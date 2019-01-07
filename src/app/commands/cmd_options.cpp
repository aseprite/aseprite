// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/extensions.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/launcher.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/resource_finder.h"
#include "app/ui/color_button.h"
#include "app/ui/pref_widget.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/string.h"
#include "base/version.h"
#include "doc/image.h"
#include "fmt/format.h"
#include "render/render.h"
#include "os/display.h"
#include "os/system.h"
#include "ui/ui.h"

#include "options.xml.h"

namespace app {

static const char* kSectionGeneralId = "section_general";
static const char* kSectionBgId = "section_bg";
static const char* kSectionGridId = "section_grid";
static const char* kSectionThemeId = "section_theme";
static const char* kSectionExtensionsId = "section_extensions";

static const char* kInfiniteSymbol = "\xE2\x88\x9E"; // Infinite symbol (UTF-8)

static app::gen::ColorProfileBehavior filesWithCsMap[] = {
  app::gen::ColorProfileBehavior::DISABLE,
  app::gen::ColorProfileBehavior::EMBEDDED,
  app::gen::ColorProfileBehavior::CONVERT,
  app::gen::ColorProfileBehavior::ASSIGN,
  app::gen::ColorProfileBehavior::ASK,
};

static app::gen::ColorProfileBehavior missingCsMap[] = {
  app::gen::ColorProfileBehavior::DISABLE,
  app::gen::ColorProfileBehavior::ASSIGN,
  app::gen::ColorProfileBehavior::ASK,
};

using namespace ui;

class OptionsWindow : public app::gen::Options {

  class ColorSpaceItem : public ListItem {
  public:
    ColorSpaceItem(const os::ColorSpacePtr& cs)
      : ListItem(cs->gfxColorSpace()->name()),
        m_cs(cs) {
    }
    os::ColorSpacePtr cs() const { return m_cs; }
  private:
    os::ColorSpacePtr m_cs;
  };

  class ThemeItem : public ListItem {
  public:
    ThemeItem(const std::string& path,
              const std::string& name)
      : ListItem(name.empty() ? "-- " + path + " --": name),
        m_path(path),
        m_name(name) {
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
      App::instance()->extensions().uninstallExtension(m_extension);
      m_extension = nullptr;
    }

    void openFolder() const {
      ASSERT(m_extension);
      app::launcher::open_folder(m_extension->path());
    }

  private:
    Extension* m_extension;
  };

public:
  OptionsWindow(Context* context, int& curSection)
    : m_pref(Preferences::instance())
    , m_globPref(m_pref.document(nullptr))
    , m_docPref(m_pref.document(context->activeDocument()))
    , m_curPref(&m_docPref)
    , m_curSection(curSection)
    , m_restoreThisTheme(m_pref.theme.selected())
    , m_restoreScreenScaling(m_pref.general.screenScale())
    , m_restoreUIScaling(m_pref.general.uiScale())
  {
    sectionListbox()->Change.connect(base::Bind<void>(&OptionsWindow::onChangeSection, this));

    // Default extension to save files
    fillExtensionsCombobox(defaultExtension(), m_pref.saveFile.defaultExtension());
    fillExtensionsCombobox(exportImageDefaultExtension(), m_pref.exportFile.imageDefaultExtension());
    fillExtensionsCombobox(exportAnimationDefaultExtension(), m_pref.exportFile.animationDefaultExtension());
    fillExtensionsCombobox(exportSpriteSheetDefaultExtension(), m_pref.spriteSheet.defaultExtension());

    // Number of recent items
    recentFiles()->setValue(m_pref.general.recentItems());
    clearRecentFiles()->Click.connect(base::Bind<void>(&OptionsWindow::onClearRecentFiles, this));

    // Color profiles
    resetColorManagement()->Click.connect(base::Bind<void>(&OptionsWindow::onResetColorManagement, this));
    colorManagement()->Click.connect(base::Bind<void>(&OptionsWindow::onColorManagement, this));
    {
      os::instance()->listColorSpaces(m_colorSpaces);
      for (auto& cs : m_colorSpaces) {
        if (cs->gfxColorSpace()->type() != gfx::ColorSpace::None)
          workingRgbCs()->addItem(new ColorSpaceItem(cs));
      }
      updateColorProfileControls(m_pref.color.manage(),
                                 m_pref.color.workingRgbSpace(),
                                 m_pref.color.filesWithProfile(),
                                 m_pref.color.missingProfile());
    }

    // Alerts
    resetAlerts()->Click.connect(base::Bind<void>(&OptionsWindow::onResetAlerts, this));

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
    cursorColorType()->Change.connect(base::Bind<void>(&OptionsWindow::onCursorColorType, this));

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

    if (m_pref.general.showFullPath())
      showFullPath()->setSelected(true);

    dataRecoveryPeriod()->setSelectedItemIndex(
      dataRecoveryPeriod()->findItemIndexByValue(
        base::convert_to<std::string>(m_pref.general.dataRecoveryPeriod())));

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
         int(os::Capabilities::CustomNativeMouseCursor)) != 0) {
      if (m_pref.cursor.useNativeCursor())
        nativeCursor()->setSelected(true);
      nativeCursor()->Click.connect(base::Bind<void>(&OptionsWindow::onNativeCursorChange, this));

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

#ifndef _WIN32
    oneFingerAsMouseMovement()->setVisible(false);
    loadWintabDriverBox()->setVisible(false);
#endif

    if (m_pref.experimental.flashLayer())
      flashLayer()->setSelected(true);

    nonactiveLayersOpacity()->setValue(m_pref.experimental.nonactiveLayersOpacity());

    if (m_pref.editor.showScrollbars())
      showScrollbars()->setSelected(true);

    if (m_pref.editor.autoScroll())
      autoScroll()->setSelected(true);

    if (m_pref.editor.straightLinePreview())
      straightLinePreview()->setSelected(true);

    if (m_pref.eyedropper.discardBrush())
      discardBrush()->setSelected(true);

    // Scope
    bgScope()->addItem("Background for New Documents");
    gridScope()->addItem("Grid for New Documents");
    if (context->activeDocument()) {
      bgScope()->addItem("Background for the Active Document");
      bgScope()->setSelectedItemIndex(1);
      bgScope()->Change.connect(base::Bind<void>(&OptionsWindow::onChangeBgScope, this));

      gridScope()->addItem("Grid for the Active Document");
      gridScope()->setSelectedItemIndex(1);
      gridScope()->Change.connect(base::Bind<void>(&OptionsWindow::onChangeGridScope, this));
    }

    selectScalingItems();

    if ((int(os::instance()->capabilities()) &
         int(os::Capabilities::GpuAccelerationSwitch)) == int(os::Capabilities::GpuAccelerationSwitch)) {
      gpuAcceleration()->setSelected(m_pref.general.gpuAcceleration());
    }
    else {
      gpuAcceleration()->setVisible(false);
    }

    // If the platform does support native menus, we show the option,
    // in other case, the option doesn't make sense for this platform.
    if (os::instance()->menus())
      showMenuBar()->setSelected(m_pref.general.showMenuBar());
    else
      showMenuBar()->setVisible(false);

    showHome()->setSelected(m_pref.general.showHome());

    // Right-click

    static_assert(int(app::gen::RightClickMode::PAINT_BGCOLOR) == 0, "");
    static_assert(int(app::gen::RightClickMode::PICK_FGCOLOR) == 1, "");
    static_assert(int(app::gen::RightClickMode::ERASE) == 2, "");
    static_assert(int(app::gen::RightClickMode::SCROLL) == 3, "");
    static_assert(int(app::gen::RightClickMode::RECTANGULAR_MARQUEE) == 4, "");
    static_assert(int(app::gen::RightClickMode::LASSO) == 5, "");
    static_assert(int(app::gen::RightClickMode::SELECT_LAYER_AND_MOVE) == 6, "");

    rightClickBehavior()->addItem("Paint with background color");
    rightClickBehavior()->addItem("Pick foreground color");
    rightClickBehavior()->addItem("Erase");
    rightClickBehavior()->addItem("Scroll");
    rightClickBehavior()->addItem("Rectangular Marquee");
    rightClickBehavior()->addItem("Lasso");
    rightClickBehavior()->addItem("Select Layer & Move");
    rightClickBehavior()->setSelectedItemIndex((int)m_pref.editor.rightClickMode());

#ifndef __APPLE__ // Zoom sliding two fingers option only on macOS
    slideZoom()->setVisible(false);
#endif

    // Checked background size
    static_assert(int(app::gen::BgType::CHECKED_16x16) == 0, "");
    static_assert(int(app::gen::BgType::CHECKED_1x1) == 4, "");
    static_assert(int(app::gen::BgType::CHECKED_CUSTOM) == 5, "");
    checkedBgSize()->addItem("16x16");
    checkedBgSize()->addItem("8x8");
    checkedBgSize()->addItem("4x4");
    checkedBgSize()->addItem("2x2");
    checkedBgSize()->addItem("1x1");
    checkedBgSize()->addItem("Custom");
    checkedBgSize()->Change.connect(base::Bind<void>(&OptionsWindow::onCheckedBgSizeChange, this));

    // Reset buttons
    resetBg()->Click.connect(base::Bind<void>(&OptionsWindow::onResetBg, this));
    resetGrid()->Click.connect(base::Bind<void>(&OptionsWindow::onResetGrid, this));

    // Links
    locateFile()->Click.connect(base::Bind<void>(&OptionsWindow::onLocateConfigFile, this));
#if _WIN32
    locateCrashFolder()->Click.connect(base::Bind<void>(&OptionsWindow::onLocateCrashFolder, this));
#else
    locateCrashFolder()->setVisible(false);
#endif

    // Undo preferences
    limitUndo()->Click.connect(base::Bind<void>(&OptionsWindow::onLimitUndoCheck, this));
    limitUndo()->setSelected(m_pref.undo.sizeLimit() != 0);
    onLimitUndoCheck();

    undoGotoModified()->setSelected(m_pref.undo.gotoModified());
    undoAllowNonlinearHistory()->setSelected(m_pref.undo.allowNonlinearHistory());

    // Theme buttons
    themeList()->Change.connect(base::Bind<void>(&OptionsWindow::onThemeChange, this));
    themeList()->DoubleClickItem.connect(base::Bind<void>(&OptionsWindow::onSelectTheme, this));
    selectTheme()->Click.connect(base::Bind<void>(&OptionsWindow::onSelectTheme, this));
    openThemeFolder()->Click.connect(base::Bind<void>(&OptionsWindow::onOpenThemeFolder, this));

    // Extensions buttons
    extensionsList()->Change.connect(base::Bind<void>(&OptionsWindow::onExtensionChange, this));
    addExtension()->Click.connect(base::Bind<void>(&OptionsWindow::onAddExtension, this));
    disableExtension()->Click.connect(base::Bind<void>(&OptionsWindow::onDisableExtension, this));
    uninstallExtension()->Click.connect(base::Bind<void>(&OptionsWindow::onUninstallExtension, this));
    openExtensionFolder()->Click.connect(base::Bind<void>(&OptionsWindow::onOpenExtensionFolder, this));

    // Apply button
    buttonApply()->Click.connect(base::Bind<void>(&OptionsWindow::onApply, this));

    onChangeBgScope();
    onChangeGridScope();
    sectionListbox()->selectIndex(m_curSection);

    // Refill languages combobox when extensions are enabled/disabled
    m_extLanguagesChanges =
      App::instance()->extensions().LanguagesChange.connect(
        base::Bind<void>(&OptionsWindow::refillLanguages, this));

    // Reload themes when extensions are enabled/disabled
    m_extThemesChanges =
      App::instance()->extensions().ThemesChange.connect(
        base::Bind<void>(&OptionsWindow::reloadThemes, this));
  }

  bool ok() {
    return (closer() == buttonOk());
  }

  void saveConfig() {
    // Save preferences in widgets that are bound to options automatically
    {
      Message* msg = new Message(kSavePreferencesMessage);
      msg->setPropagateToChildren(msg);
      sendMessage(msg);
    }

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

    m_pref.editor.zoomFromCenterWithWheel(zoomFromCenterWithWheel()->isSelected());
    m_pref.editor.zoomFromCenterWithKeys(zoomFromCenterWithKeys()->isSelected());
    m_pref.editor.showScrollbars(showScrollbars()->isSelected());
    m_pref.editor.autoScroll(autoScroll()->isSelected());
    m_pref.editor.straightLinePreview(straightLinePreview()->isSelected());
    m_pref.eyedropper.discardBrush(discardBrush()->isSelected());
    m_pref.editor.rightClickMode(static_cast<app::gen::RightClickMode>(rightClickBehavior()->getSelectedItemIndex()));
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

    m_curPref->show.grid(gridVisible()->isSelected());
    m_curPref->grid.bounds(gridBounds());
    m_curPref->grid.color(gridColor()->getColor());
    m_curPref->grid.opacity(gridOpacity()->getValue());
    m_curPref->grid.autoOpacity(gridAutoOpacity()->isSelected());

    m_curPref->show.pixelGrid(pixelGridVisible()->isSelected());
    m_curPref->pixelGrid.color(pixelGridColor()->getColor());
    m_curPref->pixelGrid.opacity(pixelGridOpacity()->getValue());
    m_curPref->pixelGrid.autoOpacity(pixelGridAutoOpacity()->isSelected());

    m_curPref->bg.type(app::gen::BgType(checkedBgSize()->getSelectedItemIndex()));
    if (m_curPref->bg.type() == app::gen::BgType::CHECKED_CUSTOM) {
      m_curPref->bg.size(gfx::Size(
        checkedBgCustomW()->textInt(),
        checkedBgCustomH()->textInt()));
    }
    m_curPref->bg.zoom(checkedBgZoom()->isSelected());
    m_curPref->bg.color1(checkedBgColor1()->getColor());
    m_curPref->bg.color2(checkedBgColor2()->getColor());

    int undo_size_limit_value;
    undo_size_limit_value = undoSizeLimit()->textInt();
    undo_size_limit_value = MID(0, undo_size_limit_value, 999999);

    m_pref.undo.sizeLimit(undo_size_limit_value);
    m_pref.undo.gotoModified(undoGotoModified()->isSelected());
    m_pref.undo.allowNonlinearHistory(undoAllowNonlinearHistory()->isSelected());

    // Experimental features
    m_pref.experimental.useNativeClipboard(nativeClipboard()->isSelected());
    m_pref.experimental.useNativeFileDialog(nativeFileDialog()->isSelected());
    m_pref.experimental.flashLayer(flashLayer()->isSelected());
    m_pref.experimental.nonactiveLayersOpacity(nonactiveLayersOpacity()->getValue());

#ifdef _WIN32
    manager()->getDisplay()
      ->setInterpretOneFingerGestureAsMouseMovement(
        oneFingerAsMouseMovement()->isSelected());
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
    os::Display* display = manager->getDisplay();
    os::instance()->setGpuAcceleration(m_pref.general.gpuAcceleration());
    display->setScale(m_pref.general.screenScale());
    manager->setDisplay(display);
  }

  void onApply() {
    saveConfig();
    m_restoreThisTheme = m_pref.theme.selected();
    m_restoreScreenScaling = m_pref.general.screenScale();
    m_restoreUIScaling = m_pref.general.uiScale();
  }

  void onNativeCursorChange() {
    bool state =
      // If the platform supports native cursors...
      (((int(os::instance()->capabilities()) &
         int(os::Capabilities::CustomNativeMouseCursor)) != 0) &&
       // If the native cursor option is not selec
       !nativeCursor()->isSelected());

    cursorScaleLabel()->setEnabled(state);
    cursorScale()->setEnabled(state);
  }

  void onChangeSection() {
    ListItem* item = static_cast<ListItem*>(sectionListbox()->getSelectedChild());
    if (!item)
      return;

    panel()->showChild(findChild(item->getValue().c_str()));
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
  }

  void onClearRecentFiles() {
    App::instance()->recentFiles()->clear();
  }

  void onColorManagement() {
    const bool state = colorManagement()->isSelected();
    workingRgbCsLabel()->setEnabled(state);
    workingRgbCs()->setEnabled(state);
    filesWithCsLabel()->setEnabled(state);
    filesWithCs()->setEnabled(state);
    missingCsLabel()->setEnabled(state);
    missingCs()->setEnabled(state);
  }

  void onResetColorManagement() {
    updateColorProfileControls(m_pref.color.manage.defaultValue(),
                               m_pref.color.workingRgbSpace.defaultValue(),
                               m_pref.color.filesWithProfile.defaultValue(),
                               m_pref.color.missingProfile.defaultValue());
  }

  void updateColorProfileControls(const bool manage,
                                  const std::string& workingRgbSpace,
                                  const app::gen::ColorProfileBehavior& filesWithProfile,
                                  const app::gen::ColorProfileBehavior& missingProfile) {
    colorManagement()->setSelected(manage);

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
    gifOptionsAlert()->resetWithDefaultValue();
    jpegOptionsAlert()->resetWithDefaultValue();
    svgOptionsAlert()->resetWithDefaultValue();
    advancedModeAlert()->resetWithDefaultValue();
    invalidFgBgColorAlert()->resetWithDefaultValue();
    runScriptAlert()->resetWithDefaultValue();
  }

  void onChangeBgScope() {
    const int item = bgScope()->getSelectedItemIndex();
    switch (item) {
      case 0: m_curPref = &m_globPref; break;
      case 1: m_curPref = &m_docPref; break;
    }

    checkedBgSize()->setSelectedItemIndex(int(m_curPref->bg.type()));
    checkedBgZoom()->setSelected(m_curPref->bg.zoom());
    checkedBgColor1()->setColor(m_curPref->bg.color1());
    checkedBgColor2()->setColor(m_curPref->bg.color2());

    onCheckedBgSizeChange();
  }

  void onCheckedBgSizeChange() {
    if (checkedBgSize()->getSelectedItemIndex() == int(app::gen::BgType::CHECKED_CUSTOM)) {
      checkedBgCustomW()->setTextf("%d", m_curPref->bg.size().w);
      checkedBgCustomH()->setTextf("%d", m_curPref->bg.size().h);
      checkedBgCustomW()->setVisible(true);
      checkedBgCustomH()->setVisible(true);
    }
    else {
      checkedBgCustomW()->setVisible(false);
      checkedBgCustomH()->setVisible(false);
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
      checkedBgSize()->setSelectedItemIndex(int(pref.bg.type.defaultValue()));
      checkedBgCustomW()->setVisible(false);
      checkedBgCustomH()->setVisible(false);
      checkedBgZoom()->setSelected(pref.bg.zoom.defaultValue());
      checkedBgColor1()->setColor(pref.bg.color1.defaultValue());
      checkedBgColor2()->setColor(pref.bg.color2.defaultValue());
    }
    // Reset document preferences with global settings
    else {
      checkedBgSize()->setSelectedItemIndex(int(pref.bg.type()));
      checkedBgZoom()->setSelected(pref.bg.zoom());
      checkedBgColor1()->setColor(pref.bg.color1());
      checkedBgColor2()->setColor(pref.bg.color2());
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
    app::launcher::open_folder(base::get_file_path(app::memory_dump_filename()));
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
    language()->removeAllItems();
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
    while (themeList()->firstChild())
      delete themeList()->lastChild();

    loadThemes();
  }

  void loadThemes() {
    // Themes already loaded
    if (themeList()->getItemsCount() > 0)
      return;

    auto theme = skin::SkinTheme::instance();
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

        ThemeItem* item = new ThemeItem(fullPath, fn);
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
          new SeparatorInView("Extension Themes", HORIZONTAL));
      }

      for (auto it : ext->themes()) {
        ThemeItem* item = new ThemeItem(it.second, it.first);
        themeList()->addChild(item);

        // Selected theme
        if (it.second == selectedPath)
          themeList()->selectChild(item);
      }
    }

    themeList()->layout();
  }

  void loadExtensions() {
    // Extensions already loaded
    if (extensionsList()->getItemsCount() > 0)
      return;

    for (auto extension : App::instance()->extensions()) {
      ExtensionItem* item = new ExtensionItem(extension);
      extensionsList()->addChild(item);
    }
    extensionsList()->sortItems();

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
                  const bool updateScaling) {
    try {
      if (themeName != m_pref.theme.selected()) {
        auto theme = static_cast<skin::SkinTheme*>(ui::get_theme());

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
      disableExtension()->setText(item->isEnabled() ? "&Disable": "&Enable");
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
          "Add Extension", "", exts,
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
          exts.uninstallExtension(ext);

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
      extensionsList()->sortItems();
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
  std::vector<os::ColorSpacePtr> m_colorSpaces;
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
  window.openWindow();

  if (!m_installExtensionFilename.empty()) {
    if (!window.showDialogToInstallExtension(m_installExtensionFilename))
      return;
  }

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
