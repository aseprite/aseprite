// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_grid_bounds.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/extensions.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/launcher.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/resource_finder.h"
#include "app/tx.h"
#include "app/ui/color_button.h"
#include "app/ui/main_window.h"
#include "app/ui/pref_widget.h"
#include "app/ui/rgbmap_algorithm_selector.h"
#include "app/ui/sampling_selector.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/string.h"
#include "base/version.h"
#include "doc/image.h"
#include "fmt/format.h"
#include "os/system.h"
#include "os/window.h"
#include "render/render.h"
#include "ui/ui.h"

#if ENABLE_SENTRY
#include "app/sentry_wrapper.h"
#endif

#include "options.xml.h"

namespace app {

namespace {

const char* kSectionGeneralId = "section_general";
const char* kSectionTabletId = "section_tablet";
const char* kSectionBgId = "section_bg";
const char* kSectionGridId = "section_grid";
const char* kSectionThemeId = "section_theme";
const char* kSectionExtensionsId = "section_extensions";

const char* kInfiniteSymbol = "\xE2\x88\x9E"; // Infinite symbol (UTF-8)

app::gen::ColorProfileBehavior filesWithCsMap[] = {
  app::gen::ColorProfileBehavior::DISABLE,
  app::gen::ColorProfileBehavior::EMBEDDED,
  app::gen::ColorProfileBehavior::CONVERT,
  app::gen::ColorProfileBehavior::ASSIGN,
  app::gen::ColorProfileBehavior::ASK,
};

app::gen::ColorProfileBehavior missingCsMap[] = {
  app::gen::ColorProfileBehavior::DISABLE,
  app::gen::ColorProfileBehavior::ASSIGN,
  app::gen::ColorProfileBehavior::ASK,
};

class ExtensionCategorySeparator : public SeparatorInView {
public:
  ExtensionCategorySeparator(const Extension::Category category,
                             const std::string& text)
    : SeparatorInView(text, ui::HORIZONTAL)
    , m_category(category) {
    InitTheme.connect(
      [this]{
        auto b = this->border();
        b.top(2*b.top());
        b.bottom(2*b.bottom());
        this->setBorder(b);
      });
  }
  Extension::Category category() const { return m_category; }
private:
  Extension::Category m_category;
};

} // anonymous namespace

using namespace ui;

class OptionsWindow : public app::gen::Options {

  class ColorSpaceItem : public ListItem {
  public:
    ColorSpaceItem(const os::ColorSpaceRef& cs)
      : ListItem(cs->gfxColorSpace()->name()),
        m_cs(cs) {
    }
    os::ColorSpaceRef cs() const { return m_cs; }
  private:
    os::ColorSpaceRef m_cs;
  };

  class ThemeItem : public ListItem {
  public:
    ThemeItem(const std::string& id,
              const std::string& path,
              const std::string& displayName = std::string(),
              const std::string& variant = std::string())
      : ListItem(createLabel(path, id, displayName, variant)),
        m_path(path),
        m_name(id) {
    }

    const std::string& themePath() const { return m_path; }
    const std::string& themeName() const { return m_name; }

    void openFolder() const {
      app::launcher::open_folder(m_path);
    }

    bool canSelect() const {
      return !m_name.empty();
    }

  private:
    static std::string createLabel(const std::string& path,
                                   const std::string& id,
                                   const std::string& displayName,
                                   const std::string& variant) {
      if (displayName.empty()) {
        if (id.empty())
          return fmt::format("-- {} --", path);
        else
          return id;
      }
      else if (id == displayName) {
        if (variant.empty())
          return id;
        else
          return fmt::format("{} - {}", id, variant);
      }
      else {
        if (variant.empty())
          return displayName;
        else
          return fmt::format("{} - {}", displayName, variant);
      }
    }

    std::string m_path;
    std::string m_name;
  };

  class ExtensionItem : public ListItem {
  public:
    ExtensionItem(Extension* extension)
      : ListItem(extension->displayName())
      , m_extension(extension) {
      setEnabled(extension->isEnabled());
    }

    Extension* extension() { return m_extension; }

    Extension::Category category() const {
      ASSERT(m_extension);
      return m_extension->category();
    }

    bool isEnabled() const {
      ASSERT(m_extension);
      return m_extension->isEnabled();
    }

    bool isInstalled() const {
      ASSERT(m_extension);
      return m_extension->isInstalled();
    }

    bool canBeDisabled() const {
      ASSERT(m_extension);
      return m_extension->canBeDisabled();
    }

    bool canBeUninstalled() const {
      ASSERT(m_extension);
      return m_extension->canBeUninstalled();
    }

    void enable(bool state) {
      ASSERT(m_extension);
      App::instance()->extensions().enableExtension(m_extension, state);
      setEnabled(m_extension->isEnabled());
    }

    void uninstall() {
      ASSERT(m_extension);
      ASSERT(canBeUninstalled());
      App::instance()->extensions().uninstallExtension(m_extension,
                                                       DeletePluginPref::kYes);
      m_extension = nullptr;
    }

    void openFolder() const {
      ASSERT(m_extension);
      app::launcher::open_folder(m_extension->path());
    }

  private:
    Extension* m_extension;
  };

  class ThemeVariantItem : public ButtonSet::Item {
  public:
    ThemeVariantItem(OptionsWindow* options,
                     const std::string& id,
                     const std::string& variant)
      : m_options(options)
      , m_themeId(id) {
      setText(variant);
    }
  private:
    void onClick() override {
      m_options->setUITheme(m_themeId,
                            false,  // Don't adjust scale
                            false); // Don't recreate variants
    }
    OptionsWindow* m_options;
    std::string m_themeId;
  };

public:
  OptionsWindow(Context* context, int& curSection)
    : m_context(context)
    , m_pref(Preferences::instance())
    , m_globPref(m_pref.document(nullptr))
    , m_docPref(m_pref.document(context->activeDocument()))
    , m_curPref(&m_docPref)
    , m_curSection(curSection)
    , m_restoreThisTheme(m_pref.theme.selected())
    , m_restoreScreenScaling(m_pref.general.screenScale())
    , m_restoreUIScaling(m_pref.general.uiScale())
  {
    sectionListbox()->Change.connect([this]{ onChangeSection(); });

    // Theme variants
    fillThemeVariants();

    // Default extension to save files
    fillExtensionsCombobox(defaultExtension(), m_pref.saveFile.defaultExtension());
    fillExtensionsCombobox(exportImageDefaultExtension(), m_pref.exportFile.imageDefaultExtension());
    fillExtensionsCombobox(exportAnimationDefaultExtension(), m_pref.exportFile.animationDefaultExtension());
    fillExtensionsCombobox(exportSpriteSheetDefaultExtension(), m_pref.spriteSheet.defaultExtension());

    // Number of recent items
    recentFiles()->setValue(m_pref.general.recentItems());
    clearRecentFiles()->Click.connect([this]{ onClearRecentFiles(); });

    // Template item for active display color profiles
    m_templateTextForDisplayCS = windowCs()->getItem(2)->text();
    windowCs()->deleteItem(2);

    // Color profiles
    resetColorManagement()->Click.connect([this]{ onResetColorManagement(); });
    colorManagement()->Click.connect([this]{ onColorManagement(); });
    {
      os::instance()->listColorSpaces(m_colorSpaces);
      for (auto& cs : m_colorSpaces) {
        if (cs->gfxColorSpace()->type() != gfx::ColorSpace::None)
          workingRgbCs()->addItem(new ColorSpaceItem(cs));
      }
      updateColorProfileControls(m_pref.color.manage(),
                                 m_pref.color.windowProfile(),
                                 m_pref.color.windowProfileName(),
                                 m_pref.color.workingRgbSpace(),
                                 m_pref.color.filesWithProfile(),
                                 m_pref.color.missingProfile());
    }

    // Alerts
    openSequence()->setSelectedItemIndex(int(m_pref.openFile.openSequence()));
    resetAlerts()->Click.connect([this]{ onResetAlerts(); });

    // Cursor
    paintingCursorType()->setSelectedItemIndex(int(m_pref.cursor.paintingCursorType()));
    cursorColor()->setColor(m_pref.cursor.cursorColor());

    if (cursorColor()->getColor().getType() == app::Color::MaskType) {
      cursorColorType()->setSelectedItemIndex(0);
      cursorColor()->setVisible(false);
    }
    else {
      cursorColorType()->setSelectedItemIndex(1);
      cursorColor()->setVisible(true);
    }
    cursorColorType()->Change.connect([this]{ onCursorColorType(); });

    // Brush preview
    brushPreview()->setSelectedItemIndex(
      (int)m_pref.cursor.brushPreview());

    // Guide colors
    layerEdgesColor()->setColor(m_pref.guides.layerEdgesColor());
    autoGuidesColor()->setColor(m_pref.guides.autoGuidesColor());

    // Slices default color
    defaultSliceColor()->setColor(m_pref.slices.defaultColor());

    // Others
    firstFrame()->setTextf("%d", m_globPref.timeline.firstFrame());

    if (m_pref.general.expandMenubarOnMouseover())
      expandMenubarOnMouseover()->setSelected(true);

    if (m_pref.general.dataRecovery())
      enableDataRecovery()->setSelected(true);
    enableDataRecovery()->Click.connect(
      [this](){
        const bool state = enableDataRecovery()->isSelected();
        keepEditedSpriteData()->setEnabled(state);
        keepEditedSpriteData()->setSelected(state);
        keepEditedSpriteDataFor()->setEnabled(state);
      });

    if (m_pref.general.dataRecovery() &&
        m_pref.general.keepEditedSpriteData())
      keepEditedSpriteData()->setSelected(true);
    else if (!m_pref.general.dataRecovery()) {
      keepEditedSpriteData()->setEnabled(false);
      keepEditedSpriteDataFor()->setEnabled(false);
    }

    if (m_pref.general.keepClosedSpriteOnMemory())
      keepClosedSpriteOnMemory()->setSelected(true);

    if (m_pref.general.showFullPath())
      showFullPath()->setSelected(true);

    dataRecoveryPeriod()->setSelectedItemIndex(
      dataRecoveryPeriod()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.dataRecoveryPeriod())));

    keepEditedSpriteDataFor()->setSelectedItemIndex(
      keepEditedSpriteDataFor()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.keepEditedSpriteDataFor())));

    keepClosedSpriteOnMemoryFor()->setSelectedItemIndex(
      keepClosedSpriteOnMemoryFor()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.keepClosedSpriteOnMemoryFor())));

    if (m_pref.editor.zoomFromCenterWithWheel())
      zoomFromCenterWithWheel()->setSelected(true);

    if (m_pref.editor.zoomFromCenterWithKeys())
      zoomFromCenterWithKeys()->setSelected(true);

    if (m_pref.selection.autoOpaque())
      autoOpaque()->setSelected(true);

    if (m_pref.selection.keepSelectionAfterClear())
      keepSelectionAfterClear()->setSelected(true);

    if (m_pref.selection.autoShowSelectionEdges())
      autoShowSelectionEdges()->setSelected(true);

    if (m_pref.selection.moveEdges())
      moveEdges()->setSelected(true);

    if (m_pref.selection.modifiersDisableHandles())
      modifiersDisableHandles()->setSelected(true);

    if (m_pref.selection.moveOnAddMode())
      moveOnAddMode()->setSelected(true);

    // If the platform supports native cursors...
    if ((int(os::instance()->capabilities()) &
         int(os::Capabilities::CustomMouseCursor)) != 0) {
      if (m_pref.cursor.useNativeCursor())
        nativeCursor()->setSelected(true);
      nativeCursor()->Click.connect([this]{ onNativeCursorChange(); });

      cursorScale()->setSelectedItemIndex(
        cursorScale()->findItemIndexByValue(
          base::convert_to<std::string>(m_pref.cursor.cursorScale())));
    }
    else {
      nativeCursor()->setEnabled(false);
    }

    onNativeCursorChange();

    if (m_pref.experimental.useNativeClipboard())
      nativeClipboard()->setSelected(true);

    if (m_pref.experimental.useNativeFileDialog())
      nativeFileDialog()->setSelected(true);

#ifdef _WIN32 // Show Tablet section on Windows
    {
      os::TabletAPI tabletAPI = os::instance()->tabletAPI();

      if (tabletAPI == os::TabletAPI::Wintab) {
        tabletApiWintabSystem()->setSelected(true);
        loadWintabDriver()->setSelected(true);
        loadWintabDriver2()->setSelected(true);
      }
      else if (tabletAPI == os::TabletAPI::WintabPackets) {
        tabletApiWintabDirect()->setSelected(true);
        loadWintabDriver()->setSelected(true);
        loadWintabDriver2()->setSelected(true);
      }
      else {
        tabletApiWindowsPointer()->setSelected(true);
        loadWintabDriver()->setSelected(false);
        loadWintabDriver2()->setSelected(false);
      }

      tabletApiWindowsPointer()->Click.connect([this](){ onTabletAPIChange(); });
      tabletApiWintabSystem()->Click.connect([this](){ onTabletAPIChange(); });
      tabletApiWintabDirect()->Click.connect([this](){ onTabletAPIChange(); });
      loadWintabDriver()->Click.connect(
        [this](){ onLoadWintabChange(loadWintabDriver()->isSelected()); });
      loadWintabDriver2()->Click.connect(
        [this](){ onLoadWintabChange(loadWintabDriver2()->isSelected()); });
    }
#else  // For macOS and Linux
    {
      // Hide the "section_tablet" item (which is only for Windows at the moment)
      for (auto item : sectionListbox()->children()) {
        if (static_cast<ListItem*>(item)->getValue() == kSectionTabletId) {
          item->setVisible(false);
          break;
        }
      }
      sectionTablet()->setVisible(false);
      loadWintabDriverBox()->setVisible(false);
      loadWintabDriverBox()->setVisible(false);
    }
#endif

    if (m_pref.experimental.flashLayer())
      flashLayer()->setSelected(true);

    nonactiveLayersOpacity()->setValue(m_pref.experimental.nonactiveLayersOpacity());

    rgbmapAlgorithmPlaceholder()->addChild(&m_rgbmapAlgorithmSelector);
    m_rgbmapAlgorithmSelector.setExpansive(true);
    m_rgbmapAlgorithmSelector.algorithm(m_pref.quantization.rgbmapAlgorithm());

    if (m_pref.editor.showScrollbars())
      showScrollbars()->setSelected(true);

    if (m_pref.editor.autoScroll())
      autoScroll()->setSelected(true);

    if (m_pref.editor.straightLinePreview())
      straightLinePreview()->setSelected(true);

    if (m_pref.eyedropper.discardBrush())
      discardBrush()->setSelected(true);

    // Scope
    bgScope()->addItem(Strings::options_bg_for_new_docs());
    gridScope()->addItem(Strings::options_grid_for_new_docs());
    if (context->activeDocument()) {
      bgScope()->addItem(Strings::options_bg_for_active_doc());
      bgScope()->setSelectedItemIndex(1);
      bgScope()->Change.connect([this]{ onChangeBgScope(); });

      gridScope()->addItem(Strings::options_grid_for_active_doc());
      gridScope()->setSelectedItemIndex(1);
      gridScope()->Change.connect([this]{ onChangeGridScope(); });
    }

    // Update the one/multiple window buttonset (and keep in on sync
    // with the old/experimental checkbox)
    uiWindows()->setSelectedItem(multipleWindows()->isSelected() ? 1: 0);
    uiWindows()->ItemChange.connect([this]() {
      multipleWindows()->setSelected(uiWindows()->selectedItem() == 1);
    });
    multipleWindows()->Click.connect([this](){
      uiWindows()->setSelectedItem(multipleWindows()->isSelected() ? 1: 0);
    });

    // Scaling
    selectScalingItems();

#ifdef ENABLE_DEVMODE // TODO enable this on Release when Aseprite supports
                      //      GPU-acceleration properly
    if (os::instance()->hasCapability(os::Capabilities::GpuAccelerationSwitch)) {
      gpuAcceleration()->setSelected(m_pref.general.gpuAcceleration());
    }
    else
#endif
    {
      gpuAcceleration()->setVisible(false);
    }

    // If the platform does support native menus, we show the option,
    // in other case, the option doesn't make sense for this platform.
    if (os::instance()->menus())
      showMenuBar()->setSelected(m_pref.general.showMenuBar());
    else
      showMenuBar()->setVisible(false);

    showHome()->setSelected(m_pref.general.showHome());

    // Editor sampling
    samplingPlaceholder()->addChild(
      m_samplingSelector = new SamplingSelector(
        SamplingSelector::Behavior::ChangeOnSave));

    m_samplingSelector->setEnabled(newRenderEngine()->isSelected());
    newRenderEngine()->Click.connect(
      [this]{
        m_samplingSelector->setEnabled(newRenderEngine()->isSelected());
      });

    // Right-click
    static_assert(int(app::gen::RightClickMode::PAINT_BGCOLOR) == 0, "");
    static_assert(int(app::gen::RightClickMode::PICK_FGCOLOR) == 1, "");
    static_assert(int(app::gen::RightClickMode::ERASE) == 2, "");
    static_assert(int(app::gen::RightClickMode::SCROLL) == 3, "");
    static_assert(int(app::gen::RightClickMode::RECTANGULAR_MARQUEE) == 4, "");
    static_assert(int(app::gen::RightClickMode::LASSO) == 5, "");
    static_assert(int(app::gen::RightClickMode::SELECT_LAYER_AND_MOVE) == 6, "");

    rightClickBehavior()->addItem(Strings::options_right_click_paint_bgcolor());
    rightClickBehavior()->addItem(Strings::options_right_click_pick_fgcolor());
    rightClickBehavior()->addItem(Strings::options_right_click_erase());
    rightClickBehavior()->addItem(Strings::options_right_click_scroll());
    rightClickBehavior()->addItem(Strings::options_right_click_rectangular_marquee());
    rightClickBehavior()->addItem(Strings::options_right_click_lasso());
    rightClickBehavior()->addItem(Strings::options_right_click_select_layer_and_move());
    rightClickBehavior()->setSelectedItemIndex((int)m_pref.editor.rightClickMode());

#ifndef __APPLE__ // Zoom sliding two fingers option only on macOS
    slideZoom()->setVisible(false);
#endif

    // Checkered background size
    static_assert(int(app::gen::BgType::CHECKERED_16x16) == 0, "");
    static_assert(int(app::gen::BgType::CHECKERED_1x1) == 4, "");
    static_assert(int(app::gen::BgType::CHECKERED_CUSTOM) == 5, "");
    checkeredBgSize()->addItem("16x16");
    checkeredBgSize()->addItem("8x8");
    checkeredBgSize()->addItem("4x4");
    checkeredBgSize()->addItem("2x2");
    checkeredBgSize()->addItem("1x1");
    checkeredBgSize()->addItem(Strings::options_bg_custom_size());
    checkeredBgSize()->Change.connect([this]{ onCheckeredBgSizeChange(); });

    // Reset buttons
    resetBg()->Click.connect([this]{ onResetBg(); });
    resetGrid()->Click.connect([this]{ onResetGrid(); });

    // Links
    locateFile()->Click.connect([this]{ onLocateConfigFile(); });
    if (!App::instance()->memoryDumpFilename().empty())
      locateCrashFolder()->Click.connect([this]{ onLocateCrashFolder(); });
    else
      locateCrashFolder()->setVisible(false);

    // Share crashdb
#if ENABLE_SENTRY
    shareCrashdb()->setSelected(Sentry::consentGiven());
#else
    shareCrashdb()->setVisible(false);
#endif

    // Undo preferences
    limitUndo()->Click.connect([this]{ onLimitUndoCheck(); });
    limitUndo()->setSelected(m_pref.undo.sizeLimit() != 0);
    onLimitUndoCheck();

    undoGotoModified()->setSelected(m_pref.undo.gotoModified());
    undoAllowNonlinearHistory()->setSelected(m_pref.undo.allowNonlinearHistory());

    // Theme buttons
    themeList()->Change.connect([this]{ onThemeChange(); });
    themeList()->DoubleClickItem.connect([this]{ onSelectTheme(); });
    selectTheme()->Click.connect([this]{ onSelectTheme(); });
    openThemeFolder()->Click.connect([this]{ onOpenThemeFolder(); });

    // Extensions buttons
    extensionsList()->Change.connect([this]{ onExtensionChange(); });
    addExtension()->Click.connect([this]{ onAddExtension(); });
    disableExtension()->Click.connect([this]{ onDisableExtension(); });
    uninstallExtension()->Click.connect([this]{ onUninstallExtension(); });
    openExtensionFolder()->Click.connect([this]{ onOpenExtensionFolder(); });

    // Apply button
    buttonApply()->Click.connect([this]{ onApply(); });

    onChangeBgScope();
    onChangeGridScope();
    sectionListbox()->selectIndex(m_curSection);

    // Refill languages combobox when extensions are enabled/disabled
    m_extLanguagesChanges =
      App::instance()->extensions().LanguagesChange.connect(
        [this]{ refillLanguages(); });

    // Reload themes when extensions are enabled/disabled
    m_extThemesChanges =
      App::instance()->extensions().ThemesChange.connect(
        [this]{ reloadThemes(); });
  }

  bool ok() {
    return (closer() == buttonOk());
  }

  void saveConfig() {
    // Save preferences in widgets that are bound to options automatically
    {
      Message msg(kSavePreferencesMessage);
      msg.setPropagateToChildren(true);
      sendMessage(&msg);
    }

    // Share crashdb
#if ENABLE_SENTRY
    if (shareCrashdb()->isSelected())
      Sentry::giveConsent();
    else
      Sentry::revokeConsent();
    App::instance()->mainWindow()->updateConsentCheckbox();
#endif

    // Update language
    Strings::instance()->setCurrentLanguage(
      language()->getItemText(language()->getSelectedItemIndex()));

    m_globPref.timeline.firstFrame(firstFrame()->textInt());
    m_pref.general.showFullPath(showFullPath()->isSelected());
    m_pref.saveFile.defaultExtension(getExtension(defaultExtension()));
    m_pref.exportFile.imageDefaultExtension(getExtension(exportImageDefaultExtension()));
    m_pref.exportFile.animationDefaultExtension(getExtension(exportAnimationDefaultExtension()));
    m_pref.spriteSheet.defaultExtension(getExtension(exportSpriteSheetDefaultExtension()));
    {
      const int limit = recentFiles()->getValue();
      m_pref.general.recentItems(limit);
      App::instance()->recentFiles()->setLimit(limit);
    }

    bool expandOnMouseover = expandMenubarOnMouseover()->isSelected();
    m_pref.general.expandMenubarOnMouseover(expandOnMouseover);
    ui::MenuBar::setExpandOnMouseover(expandOnMouseover);

    std::string warnings;

    double newPeriod = base::convert_to<double>(dataRecoveryPeriod()->getValue());
    if (enableDataRecovery()->isSelected() != m_pref.general.dataRecovery() ||
        newPeriod != m_pref.general.dataRecoveryPeriod()) {
      m_pref.general.dataRecovery(enableDataRecovery()->isSelected());
      m_pref.general.dataRecoveryPeriod(newPeriod);

      warnings += "<<- " + Strings::alerts_restart_by_preferences_save_recovery_data_period();
    }

    int newLifespan = base::convert_to<int>(keepEditedSpriteDataFor()->getValue());
    if (keepEditedSpriteData()->isSelected() != m_pref.general.keepEditedSpriteData() ||
        newLifespan != m_pref.general.keepEditedSpriteDataFor()) {
      m_pref.general.keepEditedSpriteData(keepEditedSpriteData()->isSelected());
      m_pref.general.keepEditedSpriteDataFor(newLifespan);

      warnings += "<<- " + Strings::alerts_restart_by_preferences_keep_edited_sprite_data_lifespan();
    }

    double newKeepClosed = base::convert_to<double>(keepClosedSpriteOnMemoryFor()->getValue());
    if (keepClosedSpriteOnMemory()->isSelected() != m_pref.general.keepClosedSpriteOnMemory() ||
        newKeepClosed != m_pref.general.keepClosedSpriteOnMemoryFor()) {
      m_pref.general.keepClosedSpriteOnMemory(keepClosedSpriteOnMemory()->isSelected());
      m_pref.general.keepClosedSpriteOnMemoryFor(newKeepClosed);

      warnings += "<<- " + Strings::alerts_restart_by_preferences_keep_closed_sprite_on_memory_for();
    }

    m_pref.editor.zoomFromCenterWithWheel(zoomFromCenterWithWheel()->isSelected());
    m_pref.editor.zoomFromCenterWithKeys(zoomFromCenterWithKeys()->isSelected());
    m_pref.editor.showScrollbars(showScrollbars()->isSelected());
    m_pref.editor.autoScroll(autoScroll()->isSelected());
    m_pref.editor.straightLinePreview(straightLinePreview()->isSelected());
    m_pref.eyedropper.discardBrush(discardBrush()->isSelected());
    m_pref.editor.rightClickMode(static_cast<app::gen::RightClickMode>(rightClickBehavior()->getSelectedItemIndex()));
    if (m_samplingSelector)
      m_samplingSelector->save();
    m_pref.cursor.paintingCursorType(static_cast<app::gen::PaintingCursorType>(paintingCursorType()->getSelectedItemIndex()));
    m_pref.cursor.cursorColor(cursorColor()->getColor());
    m_pref.cursor.brushPreview(static_cast<app::gen::BrushPreview>(brushPreview()->getSelectedItemIndex()));
    m_pref.cursor.useNativeCursor(nativeCursor()->isSelected());
    m_pref.cursor.cursorScale(base::convert_to<int>(cursorScale()->getValue()));
    m_pref.selection.autoOpaque(autoOpaque()->isSelected());
    m_pref.selection.keepSelectionAfterClear(keepSelectionAfterClear()->isSelected());
    m_pref.selection.autoShowSelectionEdges(autoShowSelectionEdges()->isSelected());
    m_pref.selection.moveEdges(moveEdges()->isSelected());
    m_pref.selection.modifiersDisableHandles(modifiersDisableHandles()->isSelected());
    m_pref.selection.moveOnAddMode(moveOnAddMode()->isSelected());
    m_pref.guides.layerEdgesColor(layerEdgesColor()->getColor());
    m_pref.guides.autoGuidesColor(autoGuidesColor()->getColor());
    m_pref.slices.defaultColor(defaultSliceColor()->getColor());

    m_pref.color.workingRgbSpace(
      workingRgbCs()->getItemText(
        workingRgbCs()->getSelectedItemIndex()));
    m_pref.color.filesWithProfile(
      filesWithCsMap[filesWithCs()->getSelectedItemIndex()]);
    m_pref.color.missingProfile(
      missingCsMap[missingCs()->getSelectedItemIndex()]);

    int winCs = windowCs()->getSelectedItemIndex();
    switch (winCs) {
      case 0:
        m_pref.color.windowProfile(gen::WindowColorProfile::MONITOR);
        break;
      case 1:
        m_pref.color.windowProfile(gen::WindowColorProfile::SRGB);
        break;
      default: {
        m_pref.color.windowProfile(gen::WindowColorProfile::SPECIFIC);

        std::string name;
        int j = 2;
        for (auto& cs : m_colorSpaces) {
          // We add ICC profiles only
          auto gfxCs = cs->gfxColorSpace();
          if (gfxCs->type() != gfx::ColorSpace::ICC)
            continue;

          if (j == winCs) {
            name = gfxCs->name();
            break;
          }
          ++j;
        }

        m_pref.color.windowProfileName(name);
        break;
      }
    }
    update_windows_color_profile_from_preferences();

    // Change sprite grid bounds
    if (m_context &&
        m_context->activeDocument() &&
        m_context->activeDocument()->sprite() &&
        m_context->activeDocument()->sprite()->gridBounds() != gridBounds()) {
      ContextWriter writer(m_context);
      Tx tx(m_context, Strings::commands_GridSettings(), ModifyDocument);
      tx(new cmd::SetGridBounds(writer.sprite(), gridBounds()));
      tx.commit();
    }

    m_curPref->show.grid(gridVisible()->isSelected());
    m_curPref->grid.bounds(gridBounds());
    m_curPref->grid.color(gridColor()->getColor());
    m_curPref->grid.opacity(gridOpacity()->getValue());
    m_curPref->grid.autoOpacity(gridAutoOpacity()->isSelected());

    m_curPref->show.pixelGrid(pixelGridVisible()->isSelected());
    m_curPref->pixelGrid.color(pixelGridColor()->getColor());
    m_curPref->pixelGrid.opacity(pixelGridOpacity()->getValue());
    m_curPref->pixelGrid.autoOpacity(pixelGridAutoOpacity()->isSelected());

    m_curPref->bg.type(app::gen::BgType(checkeredBgSize()->getSelectedItemIndex()));
    if (m_curPref->bg.type() == app::gen::BgType::CHECKERED_CUSTOM) {
      m_curPref->bg.size(gfx::Size(
        checkeredBgCustomW()->textInt(),
        checkeredBgCustomH()->textInt()));
    }
    m_curPref->bg.zoom(checkeredBgZoom()->isSelected());
    m_curPref->bg.color1(checkeredBgColor1()->getColor());
    m_curPref->bg.color2(checkeredBgColor2()->getColor());

    // Alerts preferences
    m_pref.openFile.openSequence(gen::SequenceDecision(openSequence()->getSelectedItemIndex()));

    int undo_size_limit_value;
    undo_size_limit_value = undoSizeLimit()->textInt();
    undo_size_limit_value = std::clamp(undo_size_limit_value, 0, 999999);

    m_pref.undo.sizeLimit(undo_size_limit_value);
    m_pref.undo.gotoModified(undoGotoModified()->isSelected());
    m_pref.undo.allowNonlinearHistory(undoAllowNonlinearHistory()->isSelected());

    // Experimental features
    m_pref.experimental.useNativeClipboard(nativeClipboard()->isSelected());
    m_pref.experimental.useNativeFileDialog(nativeFileDialog()->isSelected());
    m_pref.experimental.flashLayer(flashLayer()->isSelected());
    m_pref.experimental.nonactiveLayersOpacity(nonactiveLayersOpacity()->getValue());
    m_pref.quantization.rgbmapAlgorithm(m_rgbmapAlgorithmSelector.algorithm());

#ifdef _WIN32
    {
      os::TabletAPI tabletAPI = os::TabletAPI::Default;
      std::string tabletStr;
      bool wintabState = false;

      if (tabletApiWindowsPointer()->isSelected()) {
        tabletAPI = os::TabletAPI::WindowsPointerInput;
        tabletStr = "pointer";
      }
      else if (tabletApiWintabSystem()->isSelected()) {
        tabletAPI = os::TabletAPI::Wintab;
        tabletStr = "wintab";
        wintabState = true;
      }
      else if (tabletApiWintabDirect()->isSelected()) {
        tabletAPI = os::TabletAPI::WintabPackets;
        tabletStr = "wintab_packets";
        wintabState = true;
      }

      m_pref.tablet.api(tabletStr);
      m_pref.experimental.loadWintabDriver(wintabState);

      manager()->display()->nativeWindow()
        ->setInterpretOneFingerGestureAsMouseMovement(
          oneFingerAsMouseMovement()->isSelected());

      os::instance()->setTabletAPI(tabletAPI);
    }
#endif

    ui::set_use_native_cursors(m_pref.cursor.useNativeCursor());
    ui::set_mouse_cursor_scale(m_pref.cursor.cursorScale());

    bool reset_screen = false;
    int newScreenScale = base::convert_to<int>(screenScale()->getValue());
    if (newScreenScale != m_pref.general.screenScale()) {
      m_pref.general.screenScale(newScreenScale);
      reset_screen = true;
    }

    int newUIScale = base::convert_to<int>(uiScale()->getValue());
    if (newUIScale != m_pref.general.uiScale()) {
      m_pref.general.uiScale(newUIScale);
      ui::set_theme(ui::get_theme(),
                    newUIScale);
      reset_screen = true;
    }

    bool newGpuAccel = gpuAcceleration()->isSelected();
    if (newGpuAccel != m_pref.general.gpuAcceleration()) {
      m_pref.general.gpuAcceleration(newGpuAccel);
      reset_screen = true;
    }

    if (os::instance()->menus() &&
        m_pref.general.showMenuBar() != showMenuBar()->isSelected()) {
      m_pref.general.showMenuBar(showMenuBar()->isSelected());
    }

    bool newShowHome = showHome()->isSelected();
    if (newShowHome != m_pref.general.showHome())
      m_pref.general.showHome(newShowHome);

    m_pref.save();

    if (!warnings.empty()) {
      ui::Alert::show(
        fmt::format(Strings::alerts_restart_by_preferences(),
                    warnings));
    }

    // Probably it's safe to switch this flag in runtime
    if (m_pref.experimental.multipleWindows() != ui::get_multiple_displays())
      ui::set_multiple_displays(m_pref.experimental.multipleWindows());

    if (reset_screen)
      updateScreenScaling();
  }

  void restoreTheme() {
    if (m_pref.theme.selected() != m_restoreThisTheme) {
      setUITheme(m_restoreThisTheme, false);

      // Restore UI & Screen Scaling
      if (m_restoreUIScaling != m_pref.general.uiScale()) {
        m_pref.general.uiScale(m_restoreUIScaling);
        ui::set_theme(ui::get_theme(), m_restoreUIScaling);
      }

      if (m_restoreScreenScaling != m_pref.general.screenScale()) {
        m_pref.general.screenScale(m_restoreScreenScaling);
        updateScreenScaling();
      }
    }
  }

  bool showDialogToInstallExtension(const std::string& filename) {
    for (Widget* item : sectionListbox()->children()) {
      if (auto listItem = dynamic_cast<const ListItem*>(item)) {
        if (listItem->getValue() == kSectionExtensionsId) {
          sectionListbox()->selectChild(item);
          break;
        }
      }
    }

    // Install?
    if (ui::Alert::show(
          fmt::format(Strings::alerts_install_extension(), filename)) != 1)
      return false;

    installExtension(filename);
    return true;
  }

private:

  void fillThemeVariants() {
    ButtonSet* list = nullptr;
    for (Extension* ext : App::instance()->extensions()) {
      if (ext->isCurrentTheme()) {
        // Number of variants
        int c = 0;
        for (auto it : ext->themes()) {
          if (!it.second.variant.empty())
            ++c;
        }

        if (c >= 2) {
          list = new ButtonSet(c);
          for (auto it : ext->themes()) {
            if (!it.second.variant.empty()) {
              auto item = list->addItem(
                new ThemeVariantItem(this, it.first, it.second.variant));

              if (it.first == Preferences::instance().theme.selected())
                list->setSelectedItem(item, false);
            }
          }
        }
        break;
      }
    }
    if (list) {
      themeVariants()->addChild(list);
    }
    if (m_themeVars) {
      themeVariants()->removeChild(m_themeVars);
      m_themeVars->deferDelete();
    }
    m_themeVars = list;
    themeVariants()->setVisible(list ? true: false);
    themeVariants()->initTheme();
  }

  void fillExtensionsCombobox(ui::ComboBox* combobox,
                              const std::string& defExt) {
    base::paths exts = get_writable_extensions();
    for (const auto& e : exts) {
      int index = combobox->addItem(e);
      if (base::utf8_icmp(e, defExt) == 0)
        combobox->setSelectedItemIndex(index);
    }
  }

  std::string getExtension(ui::ComboBox* combobox) {
    Widget* defExt = combobox->getSelectedItem();
    ASSERT(defExt);
    return (defExt ? defExt->text(): std::string());
  }

  void selectScalingItems() {
    // Screen/UI Scale
    screenScale()->setSelectedItemIndex(
      screenScale()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.screenScale())));

    uiScale()->setSelectedItemIndex(
      uiScale()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.uiScale())));
  }

  void updateScreenScaling() {
    ui::Manager* manager = ui::Manager::getDefault();
    os::instance()->setGpuAcceleration(m_pref.general.gpuAcceleration());
    manager->updateAllDisplaysWithNewScale(m_pref.general.screenScale());
  }

  void onApply() {
    saveConfig();
    m_restoreThisTheme = m_pref.theme.selected();
    m_restoreScreenScaling = m_pref.general.screenScale();
    m_restoreUIScaling = m_pref.general.uiScale();
  }

  void onNativeCursorChange() {
    bool state =
      // If the platform supports custom cursors...
      (((int(os::instance()->capabilities()) &
         int(os::Capabilities::CustomMouseCursor)) != 0) &&
       // If the native cursor option is not selec
       !nativeCursor()->isSelected());

    cursorScaleLabel()->setEnabled(state);
    cursorScale()->setEnabled(state);
  }

  void onChangeSection() {
    ListItem* item = static_cast<ListItem*>(sectionListbox()->getSelectedChild());
    if (!item)
      return;

    m_curSection = sectionListbox()->getSelectedIndex();

    // General section
    if (item->getValue() == kSectionGeneralId)
      loadLanguages();
    // Background section
    else if (item->getValue() == kSectionBgId)
      onChangeBgScope();
    // Grid section
    else if (item->getValue() == kSectionGridId)
      onChangeGridScope();
    // Load themes
    else if (item->getValue() == kSectionThemeId)
      loadThemes();
    // Load extension
    else if (item->getValue() == kSectionExtensionsId)
      loadExtensions();

    panel()->showChild(findChild(item->getValue().c_str()));
  }

  void onClearRecentFiles() {
    App::instance()->recentFiles()->clear();
  }

  void onColorManagement() {
    const bool state = colorManagement()->isSelected();
    windowCsLabel()->setEnabled(state);
    windowCs()->setEnabled(state);
    workingRgbCsLabel()->setEnabled(state);
    workingRgbCs()->setEnabled(state);
    filesWithCsLabel()->setEnabled(state);
    filesWithCs()->setEnabled(state);
    missingCsLabel()->setEnabled(state);
    missingCs()->setEnabled(state);
  }

  void onResetColorManagement() {
    updateColorProfileControls(m_pref.color.manage.defaultValue(),
                               m_pref.color.windowProfile.defaultValue(),
                               m_pref.color.windowProfileName.defaultValue(),
                               m_pref.color.workingRgbSpace.defaultValue(),
                               m_pref.color.filesWithProfile.defaultValue(),
                               m_pref.color.missingProfile.defaultValue());
  }

  void updateColorProfileControls(const bool manage,
                                  const app::gen::WindowColorProfile& windowProfile,
                                  const std::string& windowProfileName,
                                  const std::string& workingRgbSpace,
                                  const app::gen::ColorProfileBehavior& filesWithProfile,
                                  const app::gen::ColorProfileBehavior& missingProfile) {
    colorManagement()->setSelected(manage);

    // Window color profile
    {
      int i = 0;
      if (windowProfile == gen::WindowColorProfile::MONITOR)
        i = 0;
      else if (windowProfile == gen::WindowColorProfile::SRGB)
        i = 1;

      // Delete previous added items in the combobox for each display
      // (we'll re-add them below).
      while (windowCs()->getItem(2))
        windowCs()->deleteItem(2);

      int j = 2;
      for (auto& cs : m_colorSpaces) {
        // We add ICC profiles only
        auto gfxCs = cs->gfxColorSpace();
        if (gfxCs->type() != gfx::ColorSpace::ICC)
          continue;

        auto name = gfxCs->name();
        windowCs()->addItem(fmt::format(m_templateTextForDisplayCS, name));
        if (windowProfile == gen::WindowColorProfile::SPECIFIC &&
            windowProfileName == name) {
          i = j;
        }
        ++j;
      }
      windowCs()->setSelectedItemIndex(i);
    }

    // Working color profile
    for (auto child : *workingRgbCs()) {
      if (child->text() == workingRgbSpace) {
        workingRgbCs()->setSelectedItem(child);
        break;
      }
    }

    for (int i=0; i<sizeof(filesWithCsMap)/sizeof(filesWithCsMap[0]); ++i) {
      if (filesWithCsMap[i] == filesWithProfile) {
        filesWithCs()->setSelectedItemIndex(i);
        break;
      }
    }

    for (int i=0; i<sizeof(missingCsMap)/sizeof(missingCsMap[0]); ++i) {
      if (missingCsMap[i] == missingProfile) {
        missingCs()->setSelectedItemIndex(i);
        break;
      }
    }

    onColorManagement();
  }

  void onResetAlerts() {
    fileFormatDoesntSupportAlert()->resetWithDefaultValue();
    exportAnimationInSequenceAlert()->resetWithDefaultValue();
    overwriteFilesOnExportAlert()->resetWithDefaultValue();
    overwriteFilesOnExportSpriteSheetAlert()->resetWithDefaultValue();
    advancedModeAlert()->resetWithDefaultValue();
    invalidFgBgColorAlert()->resetWithDefaultValue();
    runScriptAlert()->resetWithDefaultValue();
    cssOptionsAlert()->resetWithDefaultValue();
    gifOptionsAlert()->resetWithDefaultValue();
    jpegOptionsAlert()->resetWithDefaultValue();
    svgOptionsAlert()->resetWithDefaultValue();
    tgaOptionsAlert()->resetWithDefaultValue();
  }

  void onChangeBgScope() {
    const int item = bgScope()->getSelectedItemIndex();
    switch (item) {
      case 0: m_curPref = &m_globPref; break;
      case 1: m_curPref = &m_docPref; break;
    }

    checkeredBgSize()->setSelectedItemIndex(int(m_curPref->bg.type()));
    checkeredBgZoom()->setSelected(m_curPref->bg.zoom());
    checkeredBgColor1()->setColor(m_curPref->bg.color1());
    checkeredBgColor2()->setColor(m_curPref->bg.color2());

    onCheckeredBgSizeChange();
  }

  void onCheckeredBgSizeChange() {
    if (checkeredBgSize()->getSelectedItemIndex() == int(app::gen::BgType::CHECKERED_CUSTOM)) {
      checkeredBgCustomW()->setTextf("%d", m_curPref->bg.size().w);
      checkeredBgCustomH()->setTextf("%d", m_curPref->bg.size().h);
      checkeredBgCustomW()->setVisible(true);
      checkeredBgCustomH()->setVisible(true);
    }
    else {
      checkeredBgCustomW()->setVisible(false);
      checkeredBgCustomH()->setVisible(false);
    }
    sectionBg()->layout();
  }

  void onChangeGridScope() {
    int item = gridScope()->getSelectedItemIndex();

    switch (item) {
      case 0: m_curPref = &m_globPref; break;
      case 1: m_curPref = &m_docPref; break;
    }

    gridVisible()->setSelected(m_curPref->show.grid());
    gridX()->setTextf("%d", m_curPref->grid.bounds().x);
    gridY()->setTextf("%d", m_curPref->grid.bounds().y);
    gridW()->setTextf("%d", m_curPref->grid.bounds().w);
    gridH()->setTextf("%d", m_curPref->grid.bounds().h);

    gridColor()->setColor(m_curPref->grid.color());
    gridOpacity()->setValue(m_curPref->grid.opacity());
    gridAutoOpacity()->setSelected(m_curPref->grid.autoOpacity());

    pixelGridVisible()->setSelected(m_curPref->show.pixelGrid());
    pixelGridColor()->setColor(m_curPref->pixelGrid.color());
    pixelGridOpacity()->setValue(m_curPref->pixelGrid.opacity());
    pixelGridAutoOpacity()->setSelected(m_curPref->pixelGrid.autoOpacity());
  }

  void onResetBg() {
    DocumentPreferences& pref = m_globPref;

    // Reset global preferences (use default values specified in pref.xml)
    if (m_curPref == &m_globPref) {
      checkeredBgSize()->setSelectedItemIndex(int(pref.bg.type.defaultValue()));
      checkeredBgCustomW()->setVisible(false);
      checkeredBgCustomH()->setVisible(false);
      checkeredBgZoom()->setSelected(pref.bg.zoom.defaultValue());
      checkeredBgColor1()->setColor(pref.bg.color1.defaultValue());
      checkeredBgColor2()->setColor(pref.bg.color2.defaultValue());
    }
    // Reset document preferences with global settings
    else {
      checkeredBgSize()->setSelectedItemIndex(int(pref.bg.type()));
      checkeredBgZoom()->setSelected(pref.bg.zoom());
      checkeredBgColor1()->setColor(pref.bg.color1());
      checkeredBgColor2()->setColor(pref.bg.color2());
    }
  }

  void onResetGrid() {
    DocumentPreferences& pref = m_globPref;

    // Reset global preferences (use default values specified in pref.xml)
    if (m_curPref == &m_globPref) {
      gridVisible()->setSelected(pref.show.grid.defaultValue());
      gridX()->setTextf("%d", pref.grid.bounds.defaultValue().x);
      gridY()->setTextf("%d", pref.grid.bounds.defaultValue().y);
      gridW()->setTextf("%d", pref.grid.bounds.defaultValue().w);
      gridH()->setTextf("%d", pref.grid.bounds.defaultValue().h);

      gridColor()->setColor(pref.grid.color.defaultValue());
      gridOpacity()->setValue(pref.grid.opacity.defaultValue());
      gridAutoOpacity()->setSelected(pref.grid.autoOpacity.defaultValue());

      pixelGridVisible()->setSelected(pref.show.pixelGrid.defaultValue());
      pixelGridColor()->setColor(pref.pixelGrid.color.defaultValue());
      pixelGridOpacity()->setValue(pref.pixelGrid.opacity.defaultValue());
      pixelGridAutoOpacity()->setSelected(pref.pixelGrid.autoOpacity.defaultValue());
    }
    // Reset document preferences with global settings
    else {
      gridVisible()->setSelected(pref.show.grid());
      gridX()->setTextf("%d", pref.grid.bounds().x);
      gridY()->setTextf("%d", pref.grid.bounds().y);
      gridW()->setTextf("%d", pref.grid.bounds().w);
      gridH()->setTextf("%d", pref.grid.bounds().h);

      gridColor()->setColor(pref.grid.color());
      gridOpacity()->setValue(pref.grid.opacity());
      gridAutoOpacity()->setSelected(pref.grid.autoOpacity());

      pixelGridVisible()->setSelected(pref.show.pixelGrid());
      pixelGridColor()->setColor(pref.pixelGrid.color());
      pixelGridOpacity()->setValue(pref.pixelGrid.opacity());
      pixelGridAutoOpacity()->setSelected(pref.pixelGrid.autoOpacity());
    }
  }

  void onLocateCrashFolder() {
    app::launcher::open_folder(
      base::get_file_path(App::instance()->memoryDumpFilename()));
  }

  void onLocateConfigFile() {
    app::launcher::open_folder(app::main_config_filename());
  }

  void onLimitUndoCheck() {
    if (limitUndo()->isSelected()) {
      undoSizeLimit()->setEnabled(true);
      undoSizeLimit()->setTextf("%d", m_pref.undo.sizeLimit());
    }
    else {
      undoSizeLimit()->setEnabled(false);
      undoSizeLimit()->setText(kInfiniteSymbol);
    }
  }

  void refillLanguages() {
    language()->deleteAllItems();
    loadLanguages();
  }

  void loadLanguages() {
    // Languages already loaded
    if (language()->getItemCount() > 0)
      return;

    Strings* strings = Strings::instance();
    std::string curLang = strings->currentLanguage();
    for (const std::string& lang : strings->availableLanguages()) {
      int i = language()->addItem(lang);
      if (lang == curLang)
        language()->setSelectedItemIndex(i);
    }
  }

  void reloadThemes() {
    while (auto child = themeList()->lastChild())
      delete child;

    loadThemes();
  }

  void loadThemes() {
    // Themes already loaded
    if (themeList()->getItemsCount() > 0)
      return;

    auto theme = skin::SkinTheme::get(this);
    auto userFolder = userThemeFolder();
    auto folders = themeFolders();
    std::sort(folders.begin(), folders.end());
    const auto& selectedPath = theme->path();

    bool first = true;
    for (const auto& path : folders) {
      auto files = base::list_files(path);

      // Only one empty theme folder: the user folder
      if (files.empty() && path != userFolder)
        continue;

      std::sort(files.begin(), files.end());
      for (auto& fn : files) {
        std::string fullPath =
          base::normalize_path(
            base::join_path(path, fn));
        if (!base::is_directory(fullPath))
          continue;

        if (first) {
          first = false;
          themeList()->addChild(
            new SeparatorInView(base::normalize_path(path), HORIZONTAL));
        }

        ThemeItem* item = new ThemeItem(fn, fullPath);
        themeList()->addChild(item);

        // Selected theme
        if (fullPath == selectedPath)
          themeList()->selectChild(item);
      }
    }

    // Themes from extensions
    first = true;
    for (auto ext : App::instance()->extensions()) {
      if (!ext->isEnabled())
        continue;

      if (ext->themes().empty())
        continue;

      if (first) {
        first = false;
        themeList()->addChild(
          new SeparatorInView(Strings::options_extension_themes(), HORIZONTAL));
      }

      for (auto it : ext->themes()) {
        ThemeItem* item = new ThemeItem(it.first,
                                        it.second.path,
                                        ext->displayName(),
                                        it.second.variant);
        themeList()->addChild(item);

        // Selected theme
        if (it.second.path == selectedPath)
          themeList()->selectChild(item);
      }
    }

    themeList()->layout();
  }

  void loadExtensionsByCategory(const Extension::Category category,
                                const std::string& label) {
    bool hasItems = false;
    auto sep = new ExtensionCategorySeparator(category, label);
    extensionsList()->addChild(sep);
    for (auto e : App::instance()->extensions()) {
      if (e->category() == category) {
        ExtensionItem* item = new ExtensionItem(e);
        extensionsList()->addChild(item);
        hasItems = true;
      }
    }
    sep->setVisible(hasItems);
  }

  void loadExtensions() {
    // Extensions already loaded
    if (extensionsList()->getItemsCount() > 0)
      return;

    loadExtensionsByCategory(
      Extension::Category::Keys,
      Strings::options_keys_extensions());

    loadExtensionsByCategory(
      Extension::Category::Languages,
      Strings::options_language_extensions());

    loadExtensionsByCategory(
      Extension::Category::Themes,
      Strings::options_theme_extensions());

#ifdef ENABLE_SCRIPTING
    loadExtensionsByCategory(
      Extension::Category::Scripts,
      Strings::options_script_extensions());
#endif

    loadExtensionsByCategory(
      Extension::Category::Palettes,
      Strings::options_palette_extensions());

    loadExtensionsByCategory(
      Extension::Category::DitheringMatrices,
      Strings::options_dithering_matrix_extensions());

    loadExtensionsByCategory(
      Extension::Category::Multiple,
      Strings::options_multiple_extensions());

    onExtensionChange();
    extensionsList()->layout();
  }

  void onThemeChange() {
    ThemeItem* item = dynamic_cast<ThemeItem*>(themeList()->getSelectedChild());
    selectTheme()->setEnabled(item && item->canSelect());
    openThemeFolder()->setEnabled(item != nullptr);
  }

  void onSelectTheme() {
    ThemeItem* item = dynamic_cast<ThemeItem*>(themeList()->getSelectedChild());
    if (item)
      setUITheme(item->themeName(), true);
  }

  void setUITheme(const std::string& themeName,
                  const bool updateScaling,
                  const bool recreateVariantsFields = true) {
    try {
      if (themeName != m_pref.theme.selected()) {
        auto theme = skin::SkinTheme::get(this);

        // Change theme name from preferences
        m_pref.theme.selected(themeName);

        // Change the UI theme
        ui::set_theme(theme, m_pref.general.uiScale());

        // Ask for new scaling
        const int newUIScale = theme->preferredUIScaling();
        const int newScreenScale = theme->preferredScreenScaling();

        if (updateScaling &&
            ((newUIScale > 0 && m_pref.general.uiScale() != newUIScale) ||
             (newScreenScale > 0 && m_pref.general.screenScale() != newScreenScale))) {
          // Ask if the user want to adjust the Screen/UI Scaling
          const int result =
            ui::Alert::show(
              fmt::format(
                Strings::alerts_update_screen_ui_scaling_with_theme_values(),
                themeName,
                100 * m_pref.general.screenScale(),
                100 * (newScreenScale > 0 ? newScreenScale: m_pref.general.screenScale()),
                100 * m_pref.general.uiScale(),
                100 * (newUIScale > 0 ? newUIScale: m_pref.general.uiScale())));

          if (result == 1) {
            // Preferred UI Scaling factor
            if (newUIScale > 0 &&
                newUIScale != m_pref.general.uiScale()) {
              m_pref.general.uiScale(newUIScale);
              ui::set_theme(theme, m_pref.general.uiScale());
            }

            // Preferred Screen Scaling
            if (newScreenScale > 0 &&
                newScreenScale != m_pref.general.screenScale()) {
              m_pref.general.screenScale(newScreenScale);
              updateScreenScaling();
            }

            selectScalingItems();
          }
        }

        if (recreateVariantsFields)
          fillThemeVariants();
      }
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
  }

  void onOpenThemeFolder() {
    ThemeItem* item = dynamic_cast<ThemeItem*>(themeList()->getSelectedChild());
    if (item)
      item->openFolder();
  }

  void onExtensionChange() {
    ExtensionItem* item = dynamic_cast<ExtensionItem*>(extensionsList()->getSelectedChild());
    if (item && item->isInstalled()) {
      disableExtension()->setText(item->isEnabled() ?
                                  Strings::options_disable_extension():
                                  Strings::options_enable_extension());
      disableExtension()->processMnemonicFromText();
      disableExtension()->setEnabled(item->isEnabled() ? item->canBeDisabled(): true);
      uninstallExtension()->setEnabled(item->canBeUninstalled());
      openExtensionFolder()->setEnabled(true);
    }
    else {
      disableExtension()->setEnabled(false);
      uninstallExtension()->setEnabled(false);
      openExtensionFolder()->setEnabled(false);
    }
  }

  void onAddExtension() {
    base::paths exts = { "aseprite-extension", "zip" };
    base::paths filename;
    if (!app::show_file_selector(
          Strings::options_add_extension_title(), "", exts,
          FileSelectorType::Open, filename))
      return;

    ASSERT(!filename.empty());
    installExtension(filename.front());
  }

  void installExtension(const std::string& filename) {
    try {
      Extensions& exts = App::instance()->extensions();

      // Get the extension information from the compressed
      // package.json file.
      ExtensionInfo info = exts.getCompressedExtensionInfo(filename);

      // Check if the extension already exist
      for (auto ext : exts) {
        if (base::string_to_lower(ext->name()) !=
            base::string_to_lower(info.name))
          continue;

        bool isDowngrade =
          base::Version(info.version.c_str()) <
          base::Version(ext->version().c_str());

        // Uninstall?
        if (ui::Alert::show(
              fmt::format(
                Strings::alerts_update_extension(),
                ext->name(),
                (isDowngrade ? Strings::alerts_update_extension_downgrade():
                               Strings::alerts_update_extension_upgrade()),
                ext->version(),
                info.version)) != 1)
          return;

        // Uninstall old version
        if (ext->canBeUninstalled()) {
          exts.uninstallExtension(ext, DeletePluginPref::kNo);

          ExtensionItem* item = getItemByExtension(ext);
          if (item)
            deleteExtensionItem(item);
        }
        break;
      }

      Extension* ext = exts.installCompressedExtension(filename, info);

      // Enable extension
      exts.enableExtension(ext, true);

      // Add the new extension in the listbox
      ExtensionItem* item = new ExtensionItem(ext);
      extensionsList()->addChild(item);
      updateCategoryVisibility();
      extensionsList()->sortItems(
        [](Widget* a, Widget* b){
          auto aSep = dynamic_cast<ExtensionCategorySeparator*>(a);
          auto bSep = dynamic_cast<ExtensionCategorySeparator*>(b);
          auto aItem = dynamic_cast<ExtensionItem*>(a);
          auto bItem = dynamic_cast<ExtensionItem*>(b);
          auto aCat = (aSep ? aSep->category():
                       aItem ? aItem->category(): Extension::Category::None);
          auto bCat = (bSep ? bSep->category():
                       bItem ? bItem->category(): Extension::Category::None);
          if (aCat < bCat)
            return true;
          else if (aCat == bCat) {
            // There are no two separators with same category.
            ASSERT(!(aSep && bSep));

            if (aSep && !bSep)
              return true;
            else if (!aSep && bSep)
              return false;
            else
              return (base::compare_filenames(a->text(), b->text()) < 0);
          }
          else
            return false;
        });
      extensionsList()->layout();
      extensionsList()->selectChild(item);
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
  }

  void onDisableExtension() {
    ExtensionItem* item = dynamic_cast<ExtensionItem*>(extensionsList()->getSelectedChild());
    if (item) {
      item->enable(!item->isEnabled());
      onExtensionChange();
    }
  }

  void onUninstallExtension() {
    ExtensionItem* item = dynamic_cast<ExtensionItem*>(extensionsList()->getSelectedChild());
    if (!item)
      return;

    if (ui::Alert::show(
          fmt::format(
            Strings::alerts_uninstall_extension_warning(),
            item->text())) != 1)
      return;

    try {
      item->uninstall();
      deleteExtensionItem(item);
    }
    catch (const std::exception& ex) {
      Console::showException(ex);
    }
  }

  void deleteExtensionItem(ExtensionItem* item) {
    // Remove the item from the list
    extensionsList()->removeChild(item);
    updateCategoryVisibility();
    extensionsList()->layout();
    item->deferDelete();
  }

  ExtensionItem* getItemByExtension(Extension* ext) {
    for (auto child : extensionsList()->children()) {
      ExtensionItem* item = dynamic_cast<ExtensionItem*>(child);
      if (item && item->extension() == ext)
        return item;
    }
    return nullptr;
  }

  void onOpenExtensionFolder() {
    ExtensionItem* item = dynamic_cast<ExtensionItem*>(extensionsList()->getSelectedChild());
    if (item)
      item->openFolder();
  }

  void onCursorColorType() {
    switch (cursorColorType()->getSelectedItemIndex()) {
      case 0:
        cursorColor()->setColor(app::Color::fromMask());
        cursorColor()->setVisible(false);
        break;
      case 1:
        cursorColor()->setColor(app::Color::fromRgb(0, 0, 0, 255));
        cursorColor()->setVisible(true);
        break;
    }
    layout();
  }

  gfx::Rect gridBounds() const {
    return gfx::Rect(gridX()->textInt(), gridY()->textInt(),
                     gridW()->textInt(), gridH()->textInt());
  }

  static std::string userThemeFolder() {
    ResourceFinder rf;
    rf.includeDataDir(skin::SkinTheme::kThemesFolderName);

#if 0 // Don't create the user folder to store themes because now we prefer extensions
    try {
      if (!base::is_directory(rf.defaultFilename()))
        base::make_all_directories(rf.defaultFilename());
    }
    catch (...) {
      // Ignore errors
    }
#endif

    return base::normalize_path(rf.defaultFilename());
  }

  static base::paths themeFolders() {
    ResourceFinder rf;
    rf.includeDataDir(skin::SkinTheme::kThemesFolderName);

    base::paths paths;
    while (rf.next())
      paths.push_back(base::normalize_path(rf.filename()));
    return paths;
  }

  void updateCategoryVisibility() {
    bool visibleCategories[int(Extension::Category::Max)];
    for (auto& v : visibleCategories)
      v = false;
    for (auto w : extensionsList()->children()) {
      if (auto item = dynamic_cast<ExtensionItem*>(w)) {
        visibleCategories[int(item->category())] = true;
      }
    }
    for (auto w : extensionsList()->children()) {
      if (auto sep = dynamic_cast<ExtensionCategorySeparator*>(w))
        sep->setVisible(visibleCategories[int(sep->category())]);
    }
  }

#ifdef _WIN32
  void onTabletAPIChange() {
    if (tabletApiWindowsPointer()->isSelected()) {
      loadWintabDriver()->setSelected(false);
      loadWintabDriver2()->setSelected(false);
    }
    else if (tabletApiWintabSystem()->isSelected() ||
             tabletApiWintabDirect()->isSelected()) {
      loadWintabDriver()->setSelected(true);
      loadWintabDriver2()->setSelected(true);
    }
  }
  void onLoadWintabChange(bool state) {
    loadWintabDriver()->setSelected(state);
    loadWintabDriver2()->setSelected(state);
    if (state)
      tabletApiWintabSystem()->setSelected(true);
    else
      tabletApiWindowsPointer()->setSelected(true);
  }

#endif // _WIN32

  Context* m_context;
  Preferences& m_pref;
  DocumentPreferences& m_globPref;
  DocumentPreferences& m_docPref;
  DocumentPreferences* m_curPref;
  int& m_curSection;
  obs::scoped_connection m_extLanguagesChanges;
  obs::scoped_connection m_extThemesChanges;
  std::string m_restoreThisTheme;
  int m_restoreScreenScaling;
  int m_restoreUIScaling;
  std::vector<os::ColorSpaceRef> m_colorSpaces;
  std::string m_templateTextForDisplayCS;
  RgbMapAlgorithmSelector m_rgbmapAlgorithmSelector;
  ButtonSet* m_themeVars = nullptr;
  SamplingSelector* m_samplingSelector = nullptr;
};

class OptionsCommand : public Command {
public:
  OptionsCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  std::string m_installExtensionFilename;
};

OptionsCommand::OptionsCommand()
  : Command(CommandId::Options(), CmdUIOnlyFlag)
{
  Preferences& preferences = Preferences::instance();

  ui::MenuBar::setExpandOnMouseover(
    preferences.general.expandMenubarOnMouseover());
}

void OptionsCommand::onLoadParams(const Params& params)
{
  m_installExtensionFilename = params.get("installExtension");
}

void OptionsCommand::onExecute(Context* context)
{
  static int curSection = 0;

  OptionsWindow window(context, curSection);

  // As showDialogToInstallExtension() will show an ui::Alert, we need
  // to call this function after window.openWindowInForeground(), so
  // the parent window of the alert will be our OptionsWindow (and not
  // the main window).
  window.Open.connect(
    [&]() {
      if (!m_installExtensionFilename.empty()) {
        if (!window.showDialogToInstallExtension(this->m_installExtensionFilename)) {
          window.closeWindow(&window);
          return;
        }
      }
    });

  window.openWindowInForeground();
  if (window.ok())
    window.saveConfig();
  else
    window.restoreTheme();
}

Command* CommandFactory::createOptionsCommand()
{
  return new OptionsCommand;
}

} // namespace app
