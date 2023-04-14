// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/context_bar.h"

#include "app/app.h"
#include "app/app_brushes.h"
#include "app/app_menus.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/commands/quick_command.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/match_words.h"
#include "app/pref/preferences.h"
#include "app/shade.h"
#include "app/site.h"
#include "app/tools/active_tool.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/ink_type.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_loop_modifiers.h"
#include "app/ui/brush_popup.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/color_shades.h"
#include "app/ui/dithering_selector.h"
#include "app/ui/dynamics_popup.h"
#include "app/ui/editor/editor.h"
#include "app/ui/expr_entry.h"
#include "app/ui/icon_button.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/sampling_selector.h"
#include "app/ui/selection_mode_field.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/fs.h"
#include "base/pi.h"
#include "base/scoped_value.h"
#include "doc/brush.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/selected_objects.h"
#include "doc/slice.h"
#include "fmt/format.h"
#include "obs/connection.h"
#include "os/sampling.h"
#include "os/surface.h"
#include "os/system.h"
#include "render/dithering.h"
#include "render/error_diffusion.h"
#include "render/ordered_dither.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/fit_bounds.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/listbox.h"
#include "ui/listitem.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/popup_window.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/tooltips.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace tools;

static bool g_updatingFromCode = false;

class ContextBar::ZoomButtons : public ButtonSet {
public:
  ZoomButtons()
    : ButtonSet(3) {
    addItem("100%");
    addItem(Strings::context_bar_center());
    addItem(Strings::context_bar_fit_screen());
  }

private:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);

    Command* cmd = nullptr;
    Params params;

    switch (selectedItem()) {

      case 0: {
        cmd = Commands::instance()->byId(CommandId::Zoom());
        params.set("action", "set");
        params.set("percentage", "100");
        params.set("focus", "center");
        UIContext::instance()->executeCommand(cmd, params);
        break;
      }

      case 1: {
        cmd = Commands::instance()->byId(CommandId::ScrollCenter());
        break;
      }

      case 2: {
        cmd = Commands::instance()->byId(CommandId::FitScreen());
        break;
      }
    }

    if (cmd)
      UIContext::instance()->executeCommand(cmd, params);

    deselectItems();
    manager()->freeFocus();
  }
};

class ContextBar::BrushBackField : public ButtonSet {
public:
  BrushBackField()
    : ButtonSet(1) {
    addItem(Strings::context_bar_back());
  }

protected:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);

    Command* discardBrush = Commands::instance()
      ->byId(CommandId::DiscardBrush());
    UIContext::instance()->executeCommand(discardBrush);
  }
};

class ContextBar::BrushTypeField : public ButtonSet {
public:
  BrushTypeField(ContextBar* owner)
    : ButtonSet(1)
    , m_owner(owner)
    , m_brushes(App::instance()->brushes()) {
    SkinPartPtr part(new SkinPart);
    part->setBitmap(0, BrushPopup::createSurfaceForBrush(BrushRef(nullptr)));
    addItem(part, "brush_type");

    m_popupWindow.Open.connect(
      [this]{
        gfx::Region rgn(m_popupWindow.boundsOnScreen());
        rgn |= gfx::Region(this->boundsOnScreen());
        m_popupWindow.setHotRegion(rgn);
      });
  }

  ~BrushTypeField() {
    closePopup();
  }

  void updateBrush(tools::Tool* tool = nullptr) {
    BrushRef brush = m_owner->activeBrush(tool);
    SkinPartPtr part(new SkinPart);
    part->setBitmap(0, BrushPopup::createSurfaceForBrush(brush));

    const bool mono = (brush->type() != kImageBrushType);
    getItem(0)->setIcon(part);
    auto style = mono ? SkinTheme::instance()->styles.brushTypeMono() :
                        SkinTheme::instance()->styles.brushType();
    getItem(0)->setStyle(style);

  }

  void switchPopup() {
    if (!m_popupWindow.isVisible())
      openPopup();
    else
      closePopup();
  }

  void showPopupAndHighlightSlot(int slot) {
    // TODO use slot?
    openPopup();
  }

protected:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);
    switchPopup();
  }

  void onSizeHint(SizeHintEvent& ev) override {
    auto theme = SkinTheme::get(this);
    ev.setSizeHint(Size(theme->dimensions.brushTypeWidth(),
                        theme->dimensions.contextBarHeight()));
  }

  void onInitTheme(InitThemeEvent& ev) override {
    ButtonSet::onInitTheme(ev);
    m_popupWindow.initTheme();
  }

private:
  // Returns a little rectangle that can be used by the popup as the
  // first brush position.
  gfx::Point popupPosCandidate() const {
    Rect rc = bounds();
    rc.y += rc.h - 2*guiscale();
    return rc.origin();
  }

  void openPopup() {
    doc::BrushRef brush = m_owner->activeBrush();

    m_popupWindow.regenerate(display(), popupPosCandidate());
    m_popupWindow.setBrush(brush.get());
    m_popupWindow.openWindow();
  }

  void closePopup() {
    m_popupWindow.closeWindow(NULL);
    deselectItems();
  }

  ContextBar* m_owner;
  AppBrushes& m_brushes;
  BrushPopup m_popupWindow;
};

class ContextBar::BrushSizeField : public IntEntry {
public:
  BrushSizeField() : IntEntry(Brush::kMinBrushSize, Brush::kMaxBrushSize) {
    setSuffix("px");
  }

private:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue lockFlag(g_updatingFromCode, true);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.size(getValue());
  }
};

class ContextBar::BrushAngleField : public IntEntry {
public:
  BrushAngleField(BrushTypeField* brushType)
    : IntEntry(-180, 180)
    , m_brushType(brushType) {
    setSuffix("\xc2\xb0");
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue lockFlag(g_updatingFromCode, true);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.angle(getValue());

    m_brushType->updateBrush();
  }

private:
  BrushTypeField* m_brushType;
};

class ContextBar::BrushPatternField : public ComboBox {
public:
  BrushPatternField() {
    // We use "m_lock" variable to avoid setting the pattern
    // brush when we call ComboBox::addItem() (because the first
    // addItem() generates an onChange() event).
    m_lock = true;
    addItem(Strings::context_bar_pattern_aligned_to_src());
    addItem(Strings::context_bar_pattern_aligned_to_dest());
    addItem(Strings::context_bar_paint_brush());
    m_lock = false;

    setSelectedItemIndex((int)Preferences::instance().brush.pattern());
  }

  void setBrushPattern(BrushPattern type) {
    int index = 0;

    switch (type) {
      case BrushPattern::ALIGNED_TO_SRC: index = 0; break;
      case BrushPattern::ALIGNED_TO_DST: index = 1; break;
      case BrushPattern::PAINT_BRUSH: index = 2; break;
    }

    m_lock = true;
    setSelectedItemIndex(index);
    m_lock = false;
  }

protected:
  void onChange() override {
    ComboBox::onChange();

    if (m_lock)
      return;

    BrushPattern type = BrushPattern::ALIGNED_TO_SRC;

    switch (getSelectedItemIndex()) {
      case 0: type = BrushPattern::ALIGNED_TO_SRC; break;
      case 1: type = BrushPattern::ALIGNED_TO_DST; break;
      case 2: type = BrushPattern::PAINT_BRUSH; break;
    }

    Preferences::instance().brush.pattern(type);
  }

  bool m_lock;
};

class ContextBar::ToleranceField : public IntEntry {
public:
  ToleranceField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).tolerance(getValue());
  }
};

class ContextBar::ContiguousField : public CheckBox {
public:
  ContiguousField()
    : CheckBox(Strings::context_bar_contiguous()) {
    initTheme();
  }

protected:
  void onInitTheme(InitThemeEvent& ev) override {
    CheckBox::onInitTheme(ev);
    setStyle(SkinTheme::get(this)->styles.miniCheckBox());
  }

  void onClick() override {
    CheckBox::onClick();

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).contiguous(isSelected());

    releaseFocus();
  }
};

class ContextBar::PaintBucketSettingsField : public ButtonSet {
public:
  PaintBucketSettingsField() : ButtonSet(1) {
    auto theme = SkinTheme::get(this);
    addItem(theme->parts.timelineGear(), "context_bar_button");
  }

protected:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);
    const gfx::Rect bounds = this->bounds();

    Tool* tool = App::instance()->activeTool();
    auto& toolPref = Preferences::instance().tool(tool);

    Menu menu;
    MenuItem
      stopAtGrid(Strings::context_bar_stop_at_grid()),
      activeLayer(Strings::context_bar_refer_active_layer()),
      allLayers(Strings::context_bar_refer_visible_layer());
    menu.addChild(&stopAtGrid);
    menu.addChild(new MenuSeparator());
    menu.addChild(&activeLayer);
    menu.addChild(&allLayers);

    menu.addChild(new MenuSeparator);
    menu.addChild(new Label(Strings::context_bar_pixel_connectivity()));

    HBox box;
    ButtonSet buttonset(2);
    buttonset.addItem("4-Connected");
    buttonset.addItem("8-connected");
    box.addChild(&buttonset);
    menu.addChild(&box);

    stopAtGrid.setSelected(
      toolPref.floodfill.stopAtGrid() == app::gen::StopAtGrid::IF_VISIBLE);
    activeLayer.setSelected(
      toolPref.floodfill.referTo() == app::gen::FillReferTo::ACTIVE_LAYER);
    allLayers.setSelected(
      toolPref.floodfill.referTo() == app::gen::FillReferTo::ALL_LAYERS);

    int index = 0;

    switch (toolPref.floodfill.pixelConnectivity()) {
      case app::gen::PixelConnectivity::FOUR_CONNECTED: index = 0; break;
      case app::gen::PixelConnectivity::EIGHT_CONNECTED: index = 1; break;
    }

    buttonset.setSelectedItem(index);

    stopAtGrid.Click.connect(
      [&]{
        toolPref.floodfill.stopAtGrid(
          toolPref.floodfill.stopAtGrid() == app::gen::StopAtGrid::IF_VISIBLE ?
          app::gen::StopAtGrid::NEVER: app::gen::StopAtGrid::IF_VISIBLE);
      });
    activeLayer.Click.connect(
      [&]{
        toolPref.floodfill.referTo(app::gen::FillReferTo::ACTIVE_LAYER);
      });
    allLayers.Click.connect(
      [&]{
        toolPref.floodfill.referTo(app::gen::FillReferTo::ALL_LAYERS);
      });

    buttonset.ItemChange.connect(
      [&buttonset, &toolPref](ButtonSet::Item* item){
        switch (buttonset.selectedItem()) {
          case 0:
            toolPref.floodfill.pixelConnectivity(app::gen::PixelConnectivity::FOUR_CONNECTED);
            break;
          case 1:
            toolPref.floodfill.pixelConnectivity(app::gen::PixelConnectivity::EIGHT_CONNECTED);
            break;
        }
      });

    menu.initTheme();
    menu.showPopup(gfx::Point(bounds.x, bounds.y2()),
                   display());
    deselectItems();
  }

};

class ContextBar::InkTypeField : public ButtonSet {
public:
  InkTypeField(ContextBar* owner) : ButtonSet(1)
                                  , m_owner(owner) {
    auto theme = SkinTheme::get(this);
    addItem(theme->parts.inkSimple(), "ink_type");
  }

  void setInkType(InkType inkType) {
    Preferences& pref = Preferences::instance();

    if (pref.shared.shareInk()) {
      for (Tool* tool : *App::instance()->toolBox())
        pref.tool(tool).ink(inkType);
    }
    else {
      Tool* tool = App::instance()->activeTool();
      pref.tool(tool).ink(inkType);
    }

    m_owner->updateForActiveTool();
  }

  void setInkTypeIcon(InkType inkType) {
    auto theme = SkinTheme::get(this);
    SkinPartPtr part = theme->parts.inkSimple();

    switch (inkType) {
      case InkType::SIMPLE:            part = theme->parts.inkSimple(); break;
      case InkType::ALPHA_COMPOSITING: part = theme->parts.inkAlphaCompositing(); break;
      case InkType::COPY_COLOR:        part = theme->parts.inkCopyColor(); break;
      case InkType::LOCK_ALPHA:        part = theme->parts.inkLockAlpha(); break;
      case InkType::SHADING:           part = theme->parts.inkShading(); break;
    }

    getItem(0)->setIcon(part);
  }

protected:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);

    gfx::Rect bounds = this->bounds();

    AppMenus::instance()
      ->getInkPopupMenu()
      ->showPopup(gfx::Point(bounds.x, bounds.y2()),
                  display());

    deselectItems();
  }

  ContextBar* m_owner;
};

class ContextBar::InkShadesField : public HBox {
public:
  InkShadesField(ColorBar* colorBar)
    : m_button(SkinTheme::get(this)->parts.iconArrowDown())
    , m_shade(Shade(), ColorShades::DragAndDropEntries)
    , m_loaded(false) {
    addChild(&m_button);
    addChild(&m_shade);

    m_shade.setText(Strings::context_bar_select_palette_color());
    m_shade.setMinColors(2);
    m_conn = colorBar->ChangeSelection.connect(
      [this]{ onChangeColorBarSelection(); });

    m_button.setFocusStop(false);
    m_button.Click.connect([this]{ onShowMenu(); });

    initTheme();
  }

  ~InkShadesField() {
    saveShades();
  }

  void reverseShadeColors() {
    m_shade.reverseShadeColors();
  }

  doc::Remap* createShadeRemap(bool left) {
    return m_shade.createShadeRemap(left);
  }

  Shade getShade() const {
    return m_shade.getShade();
  }

  void setShade(const Shade& shade) {
    m_shade.setShade(shade);
  }

  void updateShadeFromColorBarPicks() {
    auto colorBar = ColorBar::instance();
    if (!colorBar)
      return;

    doc::PalettePicks picks;
    colorBar->getPaletteView()->getSelectedEntries(picks);
    if (picks.picks() >= 2)
      onChangeColorBarSelection();
  }

private:
  void onInitTheme(InitThemeEvent& ev) override {
    HBox::onInitTheme(ev);
    auto theme = SkinTheme::get(this);
    noBorderNoChildSpacing();
    m_shade.setStyle(theme->styles.topShadeView());
    m_button.setBgColor(theme->colors.workspace());
  }

  void onShowMenu() {
    loadShades();
    gfx::Rect bounds = m_button.bounds();

    Menu menu;
    MenuItem
      reverse(Strings::context_bar_reverse_shade()),
      save(Strings::context_bar_save_shade());
    menu.addChild(&reverse);
    menu.addChild(&save);

    bool hasShade = (m_shade.size() >= 2);
    reverse.setEnabled(hasShade);
    save.setEnabled(hasShade);
    reverse.Click.connect([this]{ reverseShadeColors(); });
    save.Click.connect([this]{ onSaveShade(); });

    if (!m_shades.empty()) {
      auto theme = SkinTheme::get(this);

      menu.addChild(new MenuSeparator);

      int i = 0;
      for (const Shade& shade : m_shades) {
        auto shadeWidget = new ColorShades(shade, ColorShades::ClickWholeShade);
        shadeWidget->setExpansive(true);
        shadeWidget->Click.connect(
          [&](ColorShades::ClickEvent&){
            m_shade.setShade(shade);
          });

        auto close = new IconButton(theme->parts.iconClose());
        close->InitTheme.connect(
          [close]{
            close->setBgColor(
              SkinTheme::get(close)->colors.menuitemNormalFace());
          });
        close->initTheme();
        close->Click.connect(
          [this, i, close]{
            m_shades.erase(m_shades.begin()+i);
            close->closeWindow();
          });

        auto item = new HBox();
        item->InitTheme.connect(
          [item]{
            item->noBorderNoChildSpacing();
          });
        item->initTheme();
        item->addChild(shadeWidget);
        item->addChild(close);
        menu.addChild(item);
        ++i;
      }
    }

    menu.showPopup(gfx::Point(bounds.x, bounds.y2()),
                   display());
    m_button.invalidate();
  }

  void onSaveShade() {
    loadShades();
    m_shades.push_back(m_shade.getShade());
  }

  void loadShades() {
    if (m_loaded)
      return;

    m_loaded = true;

    char buf[32];
    int n = get_config_int("shades", "count", 0);
    n = std::clamp(n, 0, 256);
    for (int i=0; i<n; ++i) {
      sprintf(buf, "shade%d", i);
      Shade shade = shade_from_string(get_config_string("shades", buf, ""));
      if (shade.size() >= 2)
        m_shades.push_back(shade);
    }
  }

  void saveShades() {
    if (!m_loaded)
      return;

    char buf[32];
    int n = int(m_shades.size());
    set_config_int("shades", "count", n);
    for (int i=0; i<n; ++i) {
      sprintf(buf, "shade%d", i);
      set_config_string("shades", buf, shade_to_string(m_shades[i]).c_str());
    }
  }

  void onChangeColorBarSelection() {
    if (!m_shade.isVisible())
      return;

    doc::PalettePicks picks;
    ColorBar::instance()->getPaletteView()->getSelectedEntries(picks);

    Shade newShade = m_shade.getShade();
    newShade.resize(picks.picks());

    int i = 0, j = 0;
    for (bool pick : picks) {
      if (pick)
        newShade[j++] = app::Color::fromIndex(i);
      ++i;
    }

    m_shade.setShade(newShade);

    layout();
  }

  IconButton m_button;
  ColorShades m_shade;
  std::vector<Shade> m_shades;
  bool m_loaded;
  obs::scoped_connection m_conn;
};

class ContextBar::InkOpacityField : public IntEntry {
public:
  InkOpacityField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue lockFlag(g_updatingFromCode, true);

    int newValue = getValue();
    Preferences& pref = Preferences::instance();
    if (pref.shared.shareInk()) {
      for (Tool* tool : *App::instance()->toolBox())
        pref.tool(tool).opacity(newValue);
    }
    else {
      Tool* tool = App::instance()->activeTool();
      pref.tool(tool).opacity(newValue);
    }
  }
};

class ContextBar::SprayWidthField : public IntEntry {
public:
  SprayWidthField() : IntEntry(1, 32) {
  }

protected:
  void onValueChange() override {
    IntEntry::onValueChange();
    if (g_updatingFromCode)
      return;

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).spray.width(getValue());
  }
};

class ContextBar::SpraySpeedField : public IntEntry {
public:
  SpraySpeedField() : IntEntry(1, 100) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).spray.speed(getValue());
  }
};

class ContextBar::TransparentColorField : public HBox {
public:
  TransparentColorField(ContextBar* owner,
                        TooltipManager* tooltipManager)
    : m_icon(1)
    , m_maskColor(app::Color::fromMask(), IMAGE_RGB, ColorButtonOptions())
    , m_owner(owner) {
    auto theme = SkinTheme::get(this);

    addChild(&m_icon);
    addChild(&m_maskColor);

    m_icon.addItem(theme->parts.selectionOpaque());
    gfx::Size sz = m_icon.getItem(0)->sizeHint();
    sz.w += 2*guiscale();
    m_icon.getItem(0)->setMinSize(sz);

    m_icon.ItemChange.connect([this]{ onPopup(); });
    m_maskColor.Change.connect([this]{ onChangeColor(); });

    Preferences::instance().selection.opaque.AfterChange.connect(
      [this]{ onOpaqueChange(); });

    onOpaqueChange();

    tooltipManager->addTooltipFor(
      m_icon.at(0), Strings::context_bar_transparent_color_options(), BOTTOM);
    tooltipManager->addTooltipFor(
      &m_maskColor, Strings::context_bar_transparent_color(), BOTTOM);
  }

private:

  void onPopup() {
    gfx::Rect bounds = this->bounds();

    Menu menu;
    MenuItem
      opaque(Strings::context_bar_opaque()),
      masked(Strings::context_bar_transparent()),
      automatic(Strings::context_bar_auto_adjust_layer());
    menu.addChild(&opaque);
    menu.addChild(&masked);
    menu.addChild(new MenuSeparator);
    menu.addChild(&automatic);

    if (Preferences::instance().selection.opaque())
      opaque.setSelected(true);
    else
      masked.setSelected(true);
    automatic.setSelected(Preferences::instance().selection.autoOpaque());

    opaque.Click.connect([this]{ setOpaque(true); });
    masked.Click.connect([this]{ setOpaque(false); });
    automatic.Click.connect([this]{ onAutomatic(); });

    menu.showPopup(gfx::Point(bounds.x, bounds.y2()),
                   display());
  }

  void onChangeColor() {
    Preferences::instance().selection.transparentColor(
      m_maskColor.getColor());
  }

  void setOpaque(bool opaque) {
    Preferences::instance().selection.opaque(opaque);
  }

  // When the preference is changed from outside the context bar
  void onOpaqueChange() {
    bool opaque = Preferences::instance().selection.opaque();

    auto theme = SkinTheme::get(this);
    SkinPartPtr part = (opaque ? theme->parts.selectionOpaque():
                                 theme->parts.selectionMasked());
    m_icon.getItem(0)->setIcon(part);

    m_maskColor.setVisible(!opaque);
    if (!opaque) {
      Preferences::instance().selection.transparentColor(
        m_maskColor.getColor());
    }

    if (m_owner)
      m_owner->layout();
  }

  void onAutomatic() {
    Preferences::instance().selection.autoOpaque(
      !Preferences::instance().selection.autoOpaque());
  }

  ButtonSet m_icon;
  ColorButton m_maskColor;
  ContextBar* m_owner;
};

class ContextBar::PivotField : public ButtonSet {
public:
  PivotField()
    : ButtonSet(1) {
    addItem(SkinTheme::get(this)->parts.pivotCenter(), "pivot_field");

    m_pivotConn = Preferences::instance().selection.pivotPosition.AfterChange.connect(
      [this]{ onPivotChange(); });

    onPivotChange();
  }

private:

  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);

    auto theme = SkinTheme::get(this);
    gfx::Rect bounds = this->bounds();

    Menu menu;
    CheckBox visible(Strings::context_bar_default_display_pivot());
    HBox box;
    ButtonSet buttonset(3);
    buttonset.addItem(theme->parts.pivotNorthwest(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotNorth(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotNortheast(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotWest(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotCenter(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotEast(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotSouthwest(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotSouth(), "pivot_dir");
    buttonset.addItem(theme->parts.pivotSoutheast(), "pivot_dir");
    box.addChild(&buttonset);

    menu.addChild(&visible);
    menu.addChild(new MenuSeparator);
    menu.addChild(&box);

    bool isVisible = Preferences::instance().selection.pivotVisibility();
    app::gen::PivotPosition pos = Preferences::instance().selection.pivotPosition();
    visible.setSelected(isVisible);
    buttonset.setSelectedItem(int(pos));
    buttonset.initTheme();

    visible.Click.connect(
      [&visible](){
        Preferences::instance().selection.pivotVisibility(
          visible.isSelected());
      });

    buttonset.ItemChange.connect(
      [&buttonset](ButtonSet::Item* item){
        Preferences::instance().selection.pivotPosition(
          app::gen::PivotPosition(buttonset.selectedItem()));
      });

    menu.showPopup(gfx::Point(bounds.x, bounds.y2()),
                   display());
  }

  void onPivotChange() {
    auto theme = SkinTheme::get(this);
    SkinPartPtr part;
    switch (Preferences::instance().selection.pivotPosition()) {
      case app::gen::PivotPosition::NORTHWEST: part = theme->parts.pivotNorthwest(); break;
      case app::gen::PivotPosition::NORTH:     part = theme->parts.pivotNorth(); break;
      case app::gen::PivotPosition::NORTHEAST: part = theme->parts.pivotNortheast(); break;
      case app::gen::PivotPosition::WEST:      part = theme->parts.pivotWest(); break;
      case app::gen::PivotPosition::CENTER:    part = theme->parts.pivotCenter(); break;
      case app::gen::PivotPosition::EAST:      part = theme->parts.pivotEast(); break;
      case app::gen::PivotPosition::SOUTHWEST: part = theme->parts.pivotSouthwest(); break;
      case app::gen::PivotPosition::SOUTH:     part = theme->parts.pivotSouth(); break;
      case app::gen::PivotPosition::SOUTHEAST: part = theme->parts.pivotSoutheast(); break;
    }
    if (part)
      getItem(0)->setIcon(part);
  }

  obs::scoped_connection m_pivotConn;
};

class ContextBar::RotAlgorithmField : public ComboBox {
public:
  RotAlgorithmField() {
    // We use "m_lockChange" variable to avoid setting the rotation
    // algorithm when we call ComboBox::addItem() (because the first
    // addItem() generates an onChange() event).
    m_lockChange = true;
    addItem(
      new Item(Strings::context_bar_fast_rotation(), tools::RotationAlgorithm::FAST));
    addItem(
      new Item(Strings::context_bar_rotsprite(), tools::RotationAlgorithm::ROTSPRITE));
    m_lockChange = false;

    setSelectedItemIndex((int)Preferences::instance().selection.rotationAlgorithm());
  }

protected:
  void onChange() override {
    if (m_lockChange)
      return;

    Preferences::instance().selection.rotationAlgorithm(
      static_cast<Item*>(getSelectedItem())->algo());
  }

  void onCloseListBox() override {
    releaseFocus();
  }

private:
  class Item : public ListItem {
  public:
    Item(const std::string& text, tools::RotationAlgorithm algo) :
      ListItem(text),
      m_algo(algo) {
    }

    tools::RotationAlgorithm algo() const { return m_algo; }

  private:
    tools::RotationAlgorithm m_algo;
  };

  bool m_lockChange;
};

class ContextBar::TransformationFields : public HBox {
public:
  class CustomEntry : public ExprEntry {
  public:
    CustomEntry() {
      setDecimals(1);
    }
  private:
    bool onProcessMessage(Message* msg) override {
      switch (msg->type()) {

        case kKeyDownMessage:
          if (ExprEntry::onProcessMessage(msg))
            return true;
          else if (hasFocus() && manager()->processFocusMovementMessage(msg))
            return true;
          return false;

        case kFocusLeaveMessage: {
          bool res = ExprEntry::onProcessMessage(msg);
          deselectText();
          setCaretPos(0);
          return res;
        }
      }
      return ExprEntry::onProcessMessage(msg);
    }
    void onVisible(bool visible) override {
      if (!visible && hasFocus()) {
        releaseFocus();
      }
      ExprEntry::onVisible(visible);
    }
    void onFormatExprFocusLeave(std::string& buf) override {
      buf = formatDec(onGetTextDouble());
    }
  };

  TransformationFields() {
    m_angle.setSuffix("°");
    m_skew.setSuffix("°");

    addChild(new Label("P:"));
    addChild(&m_x);
    addChild(&m_y);
    addChild(&m_w);
    addChild(&m_h);
    addChild(new Label("R:"));
    addChild(&m_angle);
    addChild(&m_skew);

    InitTheme.connect(
      [this]{
        gfx::Size sz(
          font()->textLength("8")*4 + m_x.border().width(),
          std::numeric_limits<int>::max());
        setChildSpacing(0);
        m_x.setMaxSize(sz);
        m_y.setMaxSize(sz);
        m_w.setMaxSize(sz);
        m_h.setMaxSize(sz);
        m_angle.setMaxSize(sz);
        m_skew.setMaxSize(sz);
      });
    initTheme();

    m_x.Change.connect([this]{ auto rc = bounds(); rc.x = m_x.textDouble(); onChangePos(rc); });
    m_y.Change.connect([this]{ auto rc = bounds(); rc.y = m_y.textDouble(); onChangePos(rc); });
    m_w.Change.connect([this]{ auto rc = bounds(); rc.w = m_w.textDouble(); onChangeSize(rc); });
    m_h.Change.connect([this]{ auto rc = bounds(); rc.h = m_h.textDouble(); onChangeSize(rc); });
    m_angle.Change.connect([this]{ onChangeAngle(); });
    m_skew.Change.connect([this]{ onChangeSkew(); });
  }

  void update(const Transformation& t) {
    auto rc = t.bounds();

    m_x.setText(formatDec(rc.x));
    m_y.setText(formatDec(rc.y));
    m_w.setText(formatDec(rc.w));
    m_h.setText(formatDec(rc.h));
    m_angle.setText(formatDec(180.0 * t.angle() / PI));
    m_skew.setText(formatDec(180.0 * t.skew() / PI));

    m_t = t;
  }

private:
  static std::string formatDec(const double x) {
    std::string s = fmt::format("{:0.1f}", x);
    if (s.size() > 2 &&
        s[s.size()-1] == '0' &&
        s[s.size()-2] == '.') {
      s.erase(s.size()-2, 2);
    }
    return s;
  }

  gfx::RectF bounds() const {
    return m_t.bounds();
  }
  void onChangePos(gfx::RectF newBounds) {
    // Adjust new pivot position depending on the new bounds origin
    gfx::RectF bounds = m_t.bounds();
    gfx::PointF pivot = m_t.pivot();
    if (!bounds.isEmpty()) {
      pivot.x = (pivot.x - bounds.x) / bounds.w;
      pivot.y = (pivot.y - bounds.y) / bounds.h;
      pivot.x = newBounds.x + pivot.x*newBounds.w;
      pivot.y = newBounds.y + pivot.y*newBounds.h;
      m_t.pivot(pivot);
    }
    m_t.bounds(newBounds);
    updateEditor();
  }
  void onChangeSize(gfx::RectF newBounds) {
    // Adjust bounds origin depending on the new size and the current pivot
    gfx::RectF bounds = m_t.bounds();
    gfx::PointF pivot = m_t.pivot();
    if (!bounds.isEmpty()) {
      pivot.x = (pivot.x - bounds.x) / bounds.w;
      pivot.y = (pivot.y - bounds.y) / bounds.h;
      newBounds.x -= (newBounds.w-bounds.w)*pivot.x;
      newBounds.y -= (newBounds.h-bounds.h)*pivot.y;

      m_x.setText(formatDec(newBounds.x));
      m_y.setText(formatDec(newBounds.y));
    }
    m_t.bounds(newBounds);
    updateEditor();
  }
  void onChangeAngle() {
    m_t.angle(PI * m_angle.textDouble() / 180.0);
    updateEditor();
  }
  void onChangeSkew() {
    double newSkew = PI * m_skew.textDouble() / 180.0;
    newSkew = std::clamp(newSkew, -PI*85.0/180.0, PI*85.0/180.0);
    m_t.skew(newSkew);
    updateEditor();
  }
  void updateEditor() {
    if (auto editor = Editor::activeEditor())
      editor->updateTransformation(m_t);
  }

  CustomEntry m_x, m_y, m_w, m_h;
  CustomEntry m_angle;
  CustomEntry m_skew;
  Transformation m_t;
};

class ContextBar::DynamicsField : public ButtonSet
                                , public DynamicsPopup::Delegate {
public:
  DynamicsField(ContextBar* ctxBar)
    : ButtonSet(1)
    , m_ctxBar(ctxBar) {
    addItem(SkinTheme::get(this)->parts.dynamics(), "dynamics_field");
  }

  void switchPopup() {
    if (m_popup &&
        m_popup->isVisible()) {
      m_popup->closeWindow(nullptr);
      return;
    }

    if (!m_popup) {
      m_popup.reset(new DynamicsPopup(this));
      m_popup->setOptionsGridVisibility(m_optionsGridVisibility);
      m_popup->Close.connect(
        [this](CloseEvent&){
          deselectItems();
          m_dynamics = m_popup->getDynamics();
        });
    }

    const gfx::Rect bounds = this->bounds();
    m_popup->remapWindow();
    fit_bounds(display(), m_popup.get(),
               gfx::Rect(gfx::Point(bounds.x, bounds.y2()),
                         m_popup->sizeHint()));

    m_popup->openWindow();
    m_popup->setHotRegion(gfx::Region(m_popup->boundsOnScreen()));
  }

  const tools::DynamicsOptions& getDynamics() const {
    if (m_popup && m_popup->isVisible())
      m_dynamics = m_popup->getDynamics();
    return m_dynamics;
  }

  void setOptionsGridVisibility(bool state) {
    m_optionsGridVisibility = state;
    if (m_popup)
      m_popup->setOptionsGridVisibility(state);
  }

private:
  // DynamicsPopup::Delegate impl
  doc::BrushRef getActiveBrush() override {
    return m_ctxBar->activeBrush();
  }

  void setMaxSize(int size) override {
    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.size(size);
  }

  void setMaxAngle(int angle) override {
    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.angle(angle);
  }

  // ButtonSet overrides
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);
    switchPopup();
  }

  // Widget overrides
  void onInitTheme(InitThemeEvent& ev) override {
    ButtonSet::onInitTheme(ev);
    if (m_popup)
      m_popup->initTheme();
  }

  std::unique_ptr<DynamicsPopup> m_popup;
  ContextBar* m_ctxBar;
  mutable tools::DynamicsOptions m_dynamics;
  bool m_optionsGridVisibility = true;
};

class ContextBar::FreehandAlgorithmField : public CheckBox {
public:
  FreehandAlgorithmField()
    : CheckBox(Strings::context_bar_pixel_perfect())
  {
    initTheme();
  }

  void setFreehandAlgorithm(tools::FreehandAlgorithm algo) {
    switch (algo) {
      case tools::FreehandAlgorithm::DEFAULT:
        setSelected(false);
        break;
      case tools::FreehandAlgorithm::PIXEL_PERFECT:
        setSelected(true);
        break;
      case tools::FreehandAlgorithm::DOTS:
        // Not available
        break;
    }
  }

protected:
  void onInitTheme(InitThemeEvent& ev) override {
    CheckBox::onInitTheme(ev);
    setStyle(SkinTheme::get(this)->styles.miniCheckBox());
  }

  void onClick() override {
    CheckBox::onClick();

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).freehandAlgorithm(
      isSelected() ?
        tools::FreehandAlgorithm::PIXEL_PERFECT:
        tools::FreehandAlgorithm::DEFAULT);

    releaseFocus();
  }
};

class ContextBar::SelectionModeField : public app::SelectionModeField {
public:
  SelectionModeField() { }
protected:
  void onSelectionModeChange(gen::SelectionMode mode) override {
    Preferences::instance().selection.mode(mode);
  }
};

class ContextBar::GradientTypeField : public ButtonSet {
public:
  GradientTypeField() : ButtonSet(2) {
    auto theme = SkinTheme::get(this);

    addItem(theme->parts.linearGradient(), "context_bar_button");
    addItem(theme->parts.radialGradient(), "context_bar_button");

    setSelectedItem(0);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(
      at(0), Strings::context_bar_linear_gradient(), BOTTOM);
    tooltipManager->addTooltipFor(
      at(1), Strings::context_bar_radial_gradient(), BOTTOM);
  }

  render::GradientType gradientType() const {
    return (render::GradientType)selectedItem();
  }
};

class ContextBar::DropPixelsField : public ButtonSet {
public:
  DropPixelsField() : ButtonSet(2) {
    auto theme = SkinTheme::get(this);

    addItem(theme->parts.dropPixelsOk(), "context_bar_button");
    addItem(theme->parts.dropPixelsCancel(), "context_bar_button");
    setOfferCapture(false);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    // TODO Enter and Esc should be configurable keys
    tooltipManager->addTooltipFor(at(0), Strings::context_bar_drop_pixel(), BOTTOM);
    tooltipManager->addTooltipFor(at(1), Strings::context_bar_cancel_drag(), BOTTOM);
  }

  obs::signal<void(ContextBarObserver::DropAction)> DropPixels;

protected:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);

    switch (selectedItem()) {
      case 0: DropPixels(ContextBarObserver::DropPixels); break;
      case 1: DropPixels(ContextBarObserver::CancelDrag); break;
    }
  }
};

class ContextBar::EyedropperField : public HBox {
public:
  EyedropperField() {
    m_channel.addItem("Color+Alpha");
    m_channel.addItem("Color");
    m_channel.addItem("Alpha");
    m_channel.addItem("RGB+Alpha");
    m_channel.addItem("RGB");
    m_channel.addItem("HSV+Alpha");
    m_channel.addItem("HSV");
    m_channel.addItem("HSL+Alpha");
    m_channel.addItem("HSL");
    m_channel.addItem("Gray+Alpha");
    m_channel.addItem("Gray");
    m_channel.addItem(Strings::context_bar_best_fit_index());

    m_sample.addItem(Strings::context_bar_all_layers());
    m_sample.addItem(Strings::context_bar_current_layer());
    m_sample.addItem(Strings::context_bar_first_ref_layer());

    addChild(new Label(Strings::context_bar_pick()));
    addChild(&m_channel);
    addChild(new Label(Strings::context_bar_sample()));
    addChild(&m_sample);

    m_channel.Change.connect([this]{ onChannelChange(); });
    m_sample.Change.connect([this]{ onSampleChange(); });
  }

  void updateFromPreferences(app::Preferences::Eyedropper& prefEyedropper) {
    m_channel.setSelectedItemIndex((int)prefEyedropper.channel());
    m_sample.setSelectedItemIndex((int)prefEyedropper.sample());
  }

private:
  void onChannelChange() {
    Preferences::instance().eyedropper.channel(
      (app::gen::EyedropperChannel)m_channel.getSelectedItemIndex());
  }

  void onSampleChange() {
    Preferences::instance().eyedropper.sample(
      (app::gen::EyedropperSample)m_sample.getSelectedItemIndex());
  }

  ComboBox m_channel;
  ComboBox m_sample;
};

class ContextBar::AutoSelectLayerField : public CheckBox {
public:
  AutoSelectLayerField()
    : CheckBox(Strings::context_bar_auto_select_layer())
  {
    initTheme();
  }

protected:
  void onInitTheme(InitThemeEvent& ev) override {
    CheckBox::onInitTheme(ev);
    setStyle(SkinTheme::get(this)->styles.miniCheckBox());
  }

  void onClick() override {
    CheckBox::onClick();

    auto atm = App::instance()->activeToolManager();
    if (atm->quickTool() &&
        atm->quickTool()->getInk(0)->isCelMovement()) {
      Preferences::instance().editor.autoSelectLayerQuick(isSelected());
    }
    else {
      Preferences::instance().editor.autoSelectLayer(isSelected());
    }

    releaseFocus();
  }
};

class ContextBar::SymmetryField : public ButtonSet {
public:
  SymmetryField() : ButtonSet(3) {
    setMultiMode(MultiMode::Set);
    auto theme = SkinTheme::get(this);
    addItem(theme->parts.horizontalSymmetry(), "symmetry_field");
    addItem(theme->parts.verticalSymmetry(), "symmetry_field");
    addItem("...", "symmetry_options");
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(at(0), Strings::symmetry_toggle_horizontal(), BOTTOM);
    tooltipManager->addTooltipFor(at(1), Strings::symmetry_toggle_vertical(), BOTTOM);
    tooltipManager->addTooltipFor(at(2), Strings::symmetry_show_options(), BOTTOM);
  }

  void updateWithCurrentDocument() {
    Doc* doc = UIContext::instance()->activeDocument();
    if (!doc)
      return;

    DocumentPreferences& docPref = Preferences::instance().document(doc);

    at(0)->setSelected(int(docPref.symmetry.mode()) & int(app::gen::SymmetryMode::HORIZONTAL) ? true: false);
    at(1)->setSelected(int(docPref.symmetry.mode()) & int(app::gen::SymmetryMode::VERTICAL) ? true: false);
  }

private:
  void onItemChange(Item* item) override {
    ButtonSet::onItemChange(item);

    Doc* doc = UIContext::instance()->activeDocument();
    if (!doc)
      return;

    DocumentPreferences& docPref =
      Preferences::instance().document(doc);

    int mode = 0;
    if (at(0)->isSelected()) mode |= int(app::gen::SymmetryMode::HORIZONTAL);
    if (at(1)->isSelected()) mode |= int(app::gen::SymmetryMode::VERTICAL);
    if (app::gen::SymmetryMode(mode) != docPref.symmetry.mode()) {
      docPref.symmetry.mode(app::gen::SymmetryMode(mode));
      // Redraw symmetry rules
      doc->notifyGeneralUpdate();
    }
    else if (at(2)->isSelected()) {
      auto item = at(2);

      gfx::Rect bounds = item->bounds();
      item->setSelected(false);

      Menu menu;
      MenuItem
        reset(Strings::symmetry_reset_position());
      menu.addChild(&reset);

      reset.Click.connect(
        [doc, &docPref]{
          docPref.symmetry.xAxis(doc->sprite()->width()/2.0);
          docPref.symmetry.yAxis(doc->sprite()->height()/2.0);
          // Redraw symmetry rules
          doc->notifyGeneralUpdate();
        });

      menu.showPopup(gfx::Point(bounds.x, bounds.y2()),
                     display());
    }
  }
};

class ContextBar::SliceFields : public HBox {
  class Item : public ListItem {
  public:
    Item(const doc::Slice* slice)
      : ListItem(slice->name())
      , m_slice(slice) { }
    const doc::Slice* slice() const { return m_slice; }
  private:
    const doc::Slice* m_slice;
  };

  class Combo : public ComboBox {
    SliceFields* m_sliceFields;
  public:
    Combo(SliceFields* sliceFields)
      : m_sliceFields(sliceFields) {
    }
  protected:
    void onChange() override {
      ComboBox::onChange();
      m_sliceFields->onSelectSliceFromComboBox();
    }
    void onEntryChange() override {
      ComboBox::onEntryChange();
      m_sliceFields->onComboBoxEntryChange();
    }
    void onBeforeOpenListBox() override {
      ComboBox::onBeforeOpenListBox();
      m_sliceFields->fillSlices();
    }
    void onEnterOnEditableEntry() override {
      ComboBox::onEnterOnEditableEntry();

      const Slice* slice = nullptr;
      if (auto item = dynamic_cast<Item*>(getSelectedItem())) {
        if (item->slice()->name() == getValue()) {
          slice = item->slice();
        }
      }

      auto editor = Editor::activeEditor();
      if (!slice && editor)
        slice = editor->sprite()->slices().getByName(getValue());
      if (slice)
        m_sliceFields->scrollToSlice(slice);

      closeListBox();
    }
  };

public:

  SliceFields()
    : m_doc(nullptr)
    , m_sel(2)
    , m_combobox(this)
    , m_action(2)
  {
    auto theme = SkinTheme::get(this);

    m_sel.addItem(Strings::context_bar_all());
    m_sel.addItem(Strings::context_bar_none());
    m_sel.ItemChange.connect(
      [this](ButtonSet::Item* item){
        onSelAction(m_sel.selectedItem());
      });

    m_combobox.setEditable(true);
    m_combobox.setExpansive(true);
    m_combobox.setMinSize(gfx::Size(256*guiscale(), 0));

    m_action.addItem(theme->parts.iconUserData(), "buttonset_item_icon_mono");
    m_action.addItem(theme->parts.iconClose(), "buttonset_item_icon_mono");
    m_action.ItemChange.connect(
      [this](ButtonSet::Item* item){
        onAction(m_action.selectedItem());
      });

    addChild(&m_sel);
    addChild(&m_combobox);
    addChild(&m_action);

    m_combobox.setVisible(false);
    m_action.setVisible(false);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(
      m_sel.at(0), Strings::context_bar_select_slices(), BOTTOM);
    tooltipManager->addTooltipFor(
      m_sel.at(1), Strings::context_bar_deselect_slices(), BOTTOM);
    tooltipManager->addTooltipFor(
      m_action.at(0), Strings::context_bar_slice_props(), BOTTOM);
    tooltipManager->addTooltipFor(
      m_action.at(1), Strings::context_bar_delete_slice(), BOTTOM);
  }

  void setDoc(Doc* doc) {
    m_doc = doc;
  }

  void addSlice(const doc::Slice* slice) {
    m_changeFromEntry = true;
    m_combobox.setValue(slice->name());
    updateLayout();
    m_changeFromEntry = false;
  }

  void removeSlice(const doc::Slice* slice) {
    m_combobox.setValue(std::string());
    updateLayout();
  }

  void updateSlice(const doc::Slice* slice) {
    m_combobox.setValue(slice->name());
    updateLayout();
  }

  void selectSlices(const doc::Sprite* sprite,
                    const doc::SelectedObjects& slices) {
    if (!slices.empty()) {
      auto selected = slices.frontAs<doc::Slice>();
      if (m_combobox.getValue() != selected->name())
        m_combobox.setValue(selected->name());
    }
    else {
      if (!m_combobox.getValue().empty())
        m_combobox.setValue(std::string());
    }
    updateLayout();
  }

  void closeComboBox() {
    m_combobox.closeListBox();
  }

private:
  void onVisible(bool visible) override {
    HBox::onVisible(visible);
    m_combobox.closeListBox();
  }

  void fillSlices() {
    m_combobox.deleteAllItems();
    if (m_doc && m_doc->sprite()) {
      MatchWords match(m_filter);

      std::vector<doc::Slice*> slices;
      for (auto slice : m_doc->sprite()->slices()) {
        if (match(slice->name()))
          slices.push_back(slice);
      }
      std::sort(slices.begin(), slices.end(),
                [](const doc::Slice* a, const doc::Slice* b){
                  return (base::compare_filenames(a->name(), b->name()) < 0);
                });

      for (auto slice : slices) {
        Item* item = new Item(slice);
        m_combobox.addItem(item);
      }
    }
  }

  void scrollToSlice(const Slice* slice) {
    auto editor = Editor::activeEditor();
    if (editor && slice) {
      if (const SliceKey* key = slice->getByFrame(editor->frame())) {
        editor->centerInSpritePoint(key->bounds().center());
      }
    }
  }

  void updateLayout() {
    const bool visible = (m_doc && !m_doc->sprite()->slices().empty());
    const bool relayout = (visible != m_combobox.isVisible() ||
                           visible != m_action.isVisible());

    m_combobox.setVisible(visible);
    m_action.setVisible(visible);

    if (relayout)
      parent()->layout();
  }

  void onSelAction(const int item) {
    m_sel.deselectItems();
    switch (item) {
      case 0:
        if (auto editor = Editor::activeEditor())
          editor->selectAllSlices();
        break;
      case 1:
        if (auto editor = Editor::activeEditor())
          editor->clearSlicesSelection();
        break;
    }
  }

  void onSelectSliceFromComboBox() {
    if (m_changeFromEntry)
      return;

    m_filter.clear();

    if (auto item = dynamic_cast<Item*>(m_combobox.getSelectedItem())) {
      if (auto editor = Editor::activeEditor()) {
        const doc::Slice* slice = item->slice();
        editor->clearSlicesSelection();
        editor->selectSlice(slice);
      }
    }
  }

  void onComboBoxEntryChange() {
    m_changeFromEntry = true;
    m_combobox.closeListBox();

    m_filter = m_combobox.getValue();

    m_combobox.openListBox();
    m_changeFromEntry = false;
  }

  void onAction(const int item) {
    m_action.deselectItems();

    Command* cmd = nullptr;
    Params params;

    switch (item) {
      case 0:
        cmd = Commands::instance()->byId(CommandId::SliceProperties());
        break;
      case 1:
        cmd = Commands::instance()->byId(CommandId::RemoveSlice());
        break;
    }

    if (cmd)
      UIContext::instance()->executeCommand(cmd, params);

    updateLayout();
  }

  Doc* m_doc;
  ButtonSet m_sel;
  Combo m_combobox;
  ButtonSet m_action;
  bool m_changeFromEntry;
  std::string m_filter;
};

ContextBar::ContextBar(TooltipManager* tooltipManager,
                       ColorBar* colorBar)
{
  auto& pref = Preferences::instance();

  addChild(m_selectionOptionsBox = new HBox());
  m_selectionOptionsBox->addChild(m_dropPixels = new DropPixelsField());
  m_selectionOptionsBox->addChild(m_selectionMode = new SelectionModeField);
  m_selectionOptionsBox->addChild(m_transparentColor = new TransparentColorField(this, tooltipManager));
  m_selectionOptionsBox->addChild(m_transformation = new TransformationFields);
  m_selectionOptionsBox->addChild(m_pivot = new PivotField);
  m_selectionOptionsBox->addChild(m_rotAlgo = new RotAlgorithmField());

  addChild(m_zoomButtons = new ZoomButtons);
  addChild(m_samplingSelector = new SamplingSelector);

  addChild(m_brushBack = new BrushBackField);
  addChild(m_brushType = new BrushTypeField(this));
  addChild(m_brushSize = new BrushSizeField());
  addChild(m_brushAngle = new BrushAngleField(m_brushType));
  addChild(m_brushPatternField = new BrushPatternField());

  addChild(m_toleranceLabel = new Label("Tolerance:"));
  addChild(m_tolerance = new ToleranceField());
  addChild(m_contiguous = new ContiguousField());
  addChild(m_paintBucketSettings = new PaintBucketSettingsField());
  addChild(m_gradientType = new GradientTypeField());
  addChild(m_ditheringSelector = new DitheringSelector(DitheringSelector::SelectMatrix));
  m_ditheringSelector->setUseCustomWidget(false); // Disable custom widget because the context bar is too small

  addChild(m_inkType = new InkTypeField(this));
  addChild(m_inkOpacityLabel = new Label("Opacity:"));
  addChild(m_inkOpacity = new InkOpacityField());
  addChild(m_inkShades = new InkShadesField(colorBar));

  addChild(m_eyedropperField = new EyedropperField());

  addChild(m_autoSelectLayer = new AutoSelectLayerField());

  addChild(m_sprayBox = new HBox());
  m_sprayBox->addChild(m_sprayLabel = new Label("Spray:"));
  m_sprayBox->addChild(m_sprayWidth = new SprayWidthField());
  m_sprayBox->addChild(m_spraySpeed = new SpraySpeedField());

  addChild(m_selectBoxHelp = new Label(""));
  addChild(m_freehandBox = new HBox());

  m_freehandBox->addChild(m_dynamics = new DynamicsField(this));
  m_freehandBox->addChild(m_freehandAlgo = new FreehandAlgorithmField());

  addChild(m_symmetry = new SymmetryField());
  m_symmetry->setVisible(pref.symmetryMode.enabled());

  addChild(m_sliceFields = new SliceFields);

  setupTooltips(tooltipManager);

  App::instance()->activeToolManager()->add_observer(this);
  UIContext::instance()->add_observer(this);

  m_symmModeConn = pref.symmetryMode.enabled.AfterChange.connect(
    [this]{ onSymmetryModeChange(); });
  m_fgColorConn = pref.colorBar.fgColor.AfterChange.connect(
    [this]{ onFgOrBgColorChange(doc::Brush::ImageColor::MainColor); });
  m_bgColorConn = pref.colorBar.bgColor.AfterChange.connect(
    [this]{ onFgOrBgColorChange(doc::Brush::ImageColor::BackgroundColor); });
  m_keysConn = KeyboardShortcuts::instance()->UserChange.connect(
    [this, tooltipManager]{ setupTooltips(tooltipManager); });
  m_dropPixelsConn = m_dropPixels->DropPixels.connect(&ContextBar::onDropPixels, this);

  setActiveBrush(createBrushFromPreferences());

  initTheme();
  registerCommands();
}

ContextBar::~ContextBar()
{
  UIContext::instance()->remove_observer(this);
  App::instance()->activeToolManager()->remove_observer(this);
}

void ContextBar::onInitTheme(ui::InitThemeEvent& ev)
{
  Box::onInitTheme(ev);

  auto theme = SkinTheme::get(this);
  gfx::Border border = this->border();
  border.bottom(2*guiscale());
  setBorder(border);
  setBgColor(theme->colors.workspace());
  m_sprayLabel->setStyle(theme->styles.miniLabel());
  m_toleranceLabel->setStyle(theme->styles.miniLabel());
  m_inkOpacityLabel->setStyle(theme->styles.miniLabel());
}

void ContextBar::onSizeHint(SizeHintEvent& ev)
{
  auto theme = SkinTheme::get(this);
  ev.setSizeHint(gfx::Size(0, theme->dimensions.contextBarHeight()));
}

void ContextBar::onToolSetOpacity(const int& newOpacity)
{
  if (g_updatingFromCode)
    return;

  m_inkOpacity->setTextf("%d", newOpacity);
}

void ContextBar::onToolSetFreehandAlgorithm()
{
  Tool* tool = App::instance()->activeTool();
  if (tool) {
    m_freehandAlgo->setFreehandAlgorithm(
      Preferences::instance().tool(tool).freehandAlgorithm());
  }
}

void ContextBar::onToolSetContiguous()
{
  Tool* tool = App::instance()->activeTool();
  if (tool) {
    m_contiguous->setSelected(
      Preferences::instance().tool(tool).contiguous());
  }
}

void ContextBar::onActiveSiteChange(const Site& site)
{
  DocObserverWidget<ui::HBox>::onActiveSiteChange(site);
  if (m_sliceFields->isVisible())
    updateSliceFields(site);
}

void ContextBar::onDocChange(Doc* doc)
{
  DocObserverWidget<ui::HBox>::onDocChange(doc);
  m_sliceFields->setDoc(doc);
}

void ContextBar::onAddSlice(DocEvent& ev)
{
  if (ev.slice())
    m_sliceFields->addSlice(ev.slice());
}

void ContextBar::onRemoveSlice(DocEvent& ev)
{
  if (ev.slice())
    m_sliceFields->removeSlice(ev.slice());
}

void ContextBar::onSliceNameChange(DocEvent& ev)
{
  if (ev.slice())
    m_sliceFields->updateSlice(ev.slice());
}

void ContextBar::onBrushSizeChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    discardActiveBrush();

  updateForActiveTool();
}

void ContextBar::onBrushAngleChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    discardActiveBrush();
}

void ContextBar::onActiveToolChange(tools::Tool* tool)
{
  if (m_activeBrush->type() != kImageBrushType)
    setActiveBrush(ContextBar::createBrushFromPreferences());
  else {
    updateForTool(tool);
  }
}

void ContextBar::onSymmetryModeChange()
{
  updateForActiveTool();
}

void ContextBar::onFgOrBgColorChange(doc::Brush::ImageColor imageColor)
{
  if (!m_activeBrush)
    return;

  if (m_activeBrush->type() == kImageBrushType) {
    ASSERT(m_activeBrush->image());

    auto& pref = Preferences::instance();
    m_activeBrush->setImageColor(
      imageColor,
      color_utils::color_for_target_mask(
        (imageColor == doc::Brush::ImageColor::MainColor ?
         pref.colorBar.fgColor():
         pref.colorBar.bgColor()),
        ColorTarget(ColorTarget::TransparentLayer,
                    m_activeBrush->image()->pixelFormat(),
                    -1)));
  }
}

void ContextBar::onDropPixels(ContextBarObserver::DropAction action)
{
  notify_observers(&ContextBarObserver::onDropPixels, action);
}

void ContextBar::updateSliceFields(const Site& site)
{
  if (site.sprite())
    m_sliceFields->selectSlices(site.sprite(),
                                site.selectedSlices());
  else
    m_sliceFields->closeComboBox();
}

void ContextBar::updateForActiveTool()
{
  updateForTool(App::instance()->activeTool());
}

void ContextBar::updateForTool(tools::Tool* tool)
{
  // TODO Improve the design of the visibility of ContextBar
  // items. Actually this manual show/hide logic is a mess. There
  // should be a IContextBarUser interface, with a method to ask who
  // needs which items to be visible. E.g. different tools elements
  // (inks, controllers, etc.) and sprite editor states are the main
  // target to implement this new IContextBarUser and ask for
  // ContextBar elements.

  const bool oldUpdatingFromCode = g_updatingFromCode;
  base::ScopedValue lockFlag(g_updatingFromCode, true);

  ToolPreferences* toolPref = nullptr;
  ToolPreferences::Brush* brushPref = nullptr;
  Preferences& preferences = Preferences::instance();

  if (tool) {
    toolPref = &preferences.tool(tool);
    brushPref = &toolPref->brush;
  }

  if (toolPref) {
    m_sizeConn = brushPref->size.AfterChange.connect([this]{ onBrushSizeChange(); });
    m_angleConn = brushPref->angle.AfterChange.connect([this]{ onBrushAngleChange(); });
    m_opacityConn = toolPref->opacity.AfterChange.connect(&ContextBar::onToolSetOpacity, this);
    m_freehandAlgoConn = toolPref->freehandAlgorithm.AfterChange.connect([this]{ onToolSetFreehandAlgorithm(); });
    m_contiguousConn = toolPref->contiguous.AfterChange.connect([this]{ onToolSetContiguous(); });
  }

  if (tool)
    m_brushType->updateBrush(tool);

  if (brushPref) {
    if (!oldUpdatingFromCode) {
      m_brushSize->setTextf("%d", brushPref->size());
      m_brushAngle->setTextf("%d", brushPref->angle());
    }
  }

  m_brushPatternField->setBrushPattern(
    preferences.brush.pattern());

  // Tool ink
  bool isPaint = tool &&
    (tool->getInk(0)->isPaint() ||
     tool->getInk(1)->isPaint());
  bool isEffect = tool &&
    (tool->getInk(0)->isEffect() ||
     tool->getInk(1)->isEffect());

  // True if the current tool support opacity slider
  bool supportOpacity = (isPaint || isEffect);

  // True if it makes sense to change the ink property for the current
  // tool.
  bool hasInk = tool &&
    ((tool->getInk(0)->isPaint() && !tool->getInk(0)->isEffect()) ||
     (tool->getInk(1)->isPaint() && !tool->getInk(1)->isEffect()));

  bool hasInkWithOpacity = false;
  bool hasInkShades = false;

  if (toolPref) {
    m_tolerance->setTextf("%d", toolPref->tolerance());
    m_contiguous->setSelected(toolPref->contiguous());

    m_inkType->setInkTypeIcon(toolPref->ink());
    m_inkOpacity->setTextf("%d", toolPref->opacity());

    hasInkWithOpacity =
      ((isPaint && tools::inkHasOpacity(toolPref->ink())) ||
       (isEffect));

    hasInkShades =
      (isPaint && !isEffect && toolPref->ink() == InkType::SHADING);

    m_freehandAlgo->setFreehandAlgorithm(toolPref->freehandAlgorithm());

    m_sprayWidth->setValue(toolPref->spray.width());
    m_spraySpeed->setValue(toolPref->spray.speed());
  }

  const bool updateShade = (!m_inkShades->isVisible() && hasInkShades);

  m_eyedropperField->updateFromPreferences(preferences.eyedropper);
  m_autoSelectLayer->setSelected(preferences.editor.autoSelectLayer());

  // True if we have an image as brush
  const bool hasImageBrush = (activeBrush()->type() == kImageBrushType);

  // True if the brush type supports angle.
  const bool hasBrushWithAngle =
    (activeBrush()->size() > 1) &&
    (activeBrush()->type() == kSquareBrushType ||
     activeBrush()->type() == kLineBrushType);

  // True if the current tool is eyedropper.
  const bool isEyedropper = tool &&
    (tool->getInk(0)->isEyedropper() ||
     tool->getInk(1)->isEyedropper());

  // True if the current tool is move tool.
  const bool isMove = tool &&
    (tool->getInk(0)->isCelMovement() ||
     tool->getInk(1)->isCelMovement());

  // True if the current tool is slice tool.
  const bool isSlice = tool &&
    (tool->getInk(0)->isSlice() ||
     tool->getInk(1)->isSlice());

  // True if the current tool is floodfill
  const bool isFloodfill = tool &&
    (tool->getPointShape(0)->isFloodFill() ||
     tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs tolerance options
  const bool hasTolerance = tool &&
    (tool->getPointShape(0)->isFloodFill() ||
     tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs spray options
  const bool hasSprayOptions = tool &&
    (tool->getPointShape(0)->isSpray() ||
     tool->getPointShape(1)->isSpray());

  const bool hasSelectOptions = tool &&
    (tool->getInk(0)->isSelection() ||
     tool->getInk(1)->isSelection());

  const bool isFreehand = tool &&
    (tool->getController(0)->isFreehand() ||
     tool->getController(1)->isFreehand());

  const bool showOpacity =
    (supportOpacity) &&
    ((isPaint && (hasInkWithOpacity || hasImageBrush)) ||
     (isEffect));

  const bool withDithering = tool &&
    (tool->getInk(0)->withDitheringOptions() ||
     tool->getInk(1)->withDitheringOptions());

  // True if the brush supports dynamics
  // TODO add support for dynamics in custom brushes in the future
  const bool supportDynamics = (!hasImageBrush);

  // Show/Hide fields
  m_zoomButtons->setVisible(needZoomButtons(tool));
  m_brushBack->setVisible(supportOpacity && hasImageBrush && !withDithering);
  m_brushType->setVisible(supportOpacity && (!isFloodfill || (isFloodfill && hasImageBrush && !withDithering)));
  m_brushSize->setVisible(supportOpacity && !isFloodfill && !hasImageBrush);
  m_brushAngle->setVisible(supportOpacity && !isFloodfill && !hasImageBrush && hasBrushWithAngle);
  m_brushPatternField->setVisible(supportOpacity && hasImageBrush && !withDithering);
  m_inkType->setVisible(hasInk);
  m_inkOpacityLabel->setVisible(showOpacity);
  m_inkOpacity->setVisible(showOpacity);
  m_inkShades->setVisible(hasInkShades);
  m_eyedropperField->setVisible(isEyedropper);
  m_autoSelectLayer->setVisible(isMove);
  m_dynamics->setVisible(isFreehand && supportDynamics);
  m_dynamics->setOptionsGridVisibility(isFreehand && !hasSelectOptions);
  m_freehandBox->setVisible(isFreehand && (supportOpacity || hasSelectOptions));
  m_toleranceLabel->setVisible(hasTolerance);
  m_tolerance->setVisible(hasTolerance);
  m_contiguous->setVisible(hasTolerance);
  m_paintBucketSettings->setVisible(hasTolerance);
  m_sprayBox->setVisible(hasSprayOptions);
  m_selectionOptionsBox->setVisible(hasSelectOptions);
  m_gradientType->setVisible(withDithering);
  m_ditheringSelector->setVisible(withDithering);
  m_selectionMode->setVisible(true);
  m_pivot->setVisible(true);
  m_dropPixels->setVisible(false);
  m_transformation->setVisible(false);
  m_selectBoxHelp->setVisible(false);

  m_symmetry->setVisible(
    Preferences::instance().symmetryMode.enabled() &&
    (isPaint || isEffect || hasSelectOptions));
  m_symmetry->updateWithCurrentDocument();

  m_sliceFields->setVisible(isSlice);
  if (isSlice)
    updateSliceFields(UIContext::instance()->activeSite());

  // Update ink shades with the current selected palette entries
  if (updateShade)
    m_inkShades->updateShadeFromColorBarPicks();

  if (!updateSamplingVisibility(tool)) {
    // updateSamplingVisibility() returns false if it doesn't layout()
    // the ContextBar.
    layout();
  }
}

void ContextBar::updateForMovingPixels(const Transformation& t)
{
  tools::Tool* tool = App::instance()->toolBox()->getToolById(
    tools::WellKnownTools::RectangularMarquee);
  if (tool)
    updateForTool(tool);

  m_dropPixels->deselectItems();
  m_dropPixels->setVisible(true);
  m_selectionMode->setVisible(false);
  m_transformation->setVisible(true);
  m_transformation->update(t);
  layout();
}

void ContextBar::updateForSelectingBox(const std::string& text)
{
  if (m_selectBoxHelp->isVisible() && m_selectBoxHelp->text() == text)
    return;

  updateForTool(nullptr);
  m_selectBoxHelp->setText(text);
  m_selectBoxHelp->setVisible(true);
  layout();
}

void ContextBar::updateToolLoopModifiersIndicators(tools::ToolLoopModifiers modifiers)
{
  gen::SelectionMode mode = gen::SelectionMode::DEFAULT;
  if (int(modifiers) & int(tools::ToolLoopModifiers::kAddSelection))
    mode = gen::SelectionMode::ADD;
  else if (int(modifiers) & int(tools::ToolLoopModifiers::kSubtractSelection))
    mode = gen::SelectionMode::SUBTRACT;
  else if (int(modifiers) & int(tools::ToolLoopModifiers::kIntersectSelection))
    mode = gen::SelectionMode::INTERSECT;

  m_selectionMode->setSelectionMode(mode);
}

bool ContextBar::updateSamplingVisibility(tools::Tool* tool)
{
  if (!tool)
    tool = App::instance()->activeTool();

  auto editor = Editor::activeEditor();
  const bool newVisibility =
    needZoomButtons(tool) &&
    editor &&
    (editor->projection().scaleX() < 1.0 ||
     editor->projection().scaleY() < 1.0) &&
    editor->isUsingNewRenderEngine();

  if (newVisibility == m_samplingSelector->hasFlags(HIDDEN)) {
    m_samplingSelector->setVisible(newVisibility);
    layout();
    return true;
  }
  return false;
}

void ContextBar::updateAutoSelectLayer(bool state)
{
  m_autoSelectLayer->setSelected(state);
}

bool ContextBar::isAutoSelectLayer() const
{
  return m_autoSelectLayer->isSelected();
}

void ContextBar::setActiveBrushBySlot(tools::Tool* tool, int slot)
{
  ASSERT(tool);
  if (!tool)
    return;

  AppBrushes& brushes = App::instance()->brushes();
  BrushSlot brush = brushes.getBrushSlot(slot);
  if (!brush.isEmpty()) {
    brushes.lockBrushSlot(slot);

    Preferences& pref = Preferences::instance();
    ToolPreferences& toolPref = pref.tool(tool);
    ToolPreferences::Brush& brushPref = toolPref.brush;

    if (brush.brush()) {
      if (brush.brush()->type() == doc::kImageBrushType) {
        // Reset the colors of the image when we select the brush from
        // the slot.
        if (brush.hasFlag(BrushSlot::Flags::ImageColor))
          brush.brush()->resetImageColors();

        setActiveBrush(brush.brush());
      }
      else {
        if (brush.hasFlag(BrushSlot::Flags::BrushType))
          brushPref.type(static_cast<app::gen::BrushType>(brush.brush()->type()));

        if (brush.hasFlag(BrushSlot::Flags::BrushSize))
          brushPref.size(brush.brush()->size());

        if (brush.hasFlag(BrushSlot::Flags::BrushAngle))
          brushPref.angle(brush.brush()->angle());

        setActiveBrush(ContextBar::createBrushFromPreferences());
      }
    }

    if (brush.hasFlag(BrushSlot::Flags::FgColor))
      pref.colorBar.fgColor(brush.fgColor());

    if (brush.hasFlag(BrushSlot::Flags::BgColor))
      pref.colorBar.bgColor(brush.bgColor());

    // If the image/stamp brush doesn't have the "ImageColor" flag, it
    // means that we have to change the image color to the current
    // "foreground color".
    if (brush.brush() &&
        brush.brush()->type() == doc::kImageBrushType &&
        !brush.hasFlag(BrushSlot::Flags::ImageColor)) {
      auto pixelFormat = brush.brush()->image()->pixelFormat();

      brush.brush()->setImageColor(
        Brush::ImageColor::MainColor,
        color_utils::color_for_target_mask(
          pref.colorBar.fgColor(),
          ColorTarget(ColorTarget::TransparentLayer,
                      pixelFormat,
                      -1)));

      brush.brush()->setImageColor(
        Brush::ImageColor::BackgroundColor,
        color_utils::color_for_target_mask(
          pref.colorBar.bgColor(),
          ColorTarget(ColorTarget::TransparentLayer,
                      pixelFormat,
                      -1)));
    }

    if (brush.hasFlag(BrushSlot::Flags::InkType))
      setInkType(brush.inkType());

    if (brush.hasFlag(BrushSlot::Flags::InkOpacity))
      toolPref.opacity(brush.inkOpacity());

    if (brush.hasFlag(BrushSlot::Flags::Shade))
      m_inkShades->setShade(brush.shade());

    if (brush.hasFlag(BrushSlot::Flags::PixelPerfect))
      toolPref.freehandAlgorithm(
        (brush.pixelPerfect() ?
         tools::FreehandAlgorithm::PIXEL_PERFECT:
         tools::FreehandAlgorithm::REGULAR));
  }
  else {
    updateForTool(tool);
    m_brushType->showPopupAndHighlightSlot(slot);
  }
}

void ContextBar::setActiveBrush(const doc::BrushRef& brush)
{
  if (brush->type() == kImageBrushType)
    m_activeBrush = brush;
  else {
    Tool* tool = App::instance()->activeTool();
    auto& brushPref = Preferences::instance().tool(tool).brush;
    auto newBrushType = static_cast<app::gen::BrushType>(brush->type());
    if (brushPref.type() != newBrushType)
      brushPref.type(newBrushType);

    m_activeBrush = brush;
  }

  BrushChange();

  updateForActiveTool();
}

doc::BrushRef ContextBar::activeBrush(tools::Tool* tool,
                                      tools::Ink* ink) const
{
  if (ink == nullptr)
    ink = (tool ? tool->getInk(0): nullptr);

  // Selection tools use a brush with size = 1 (always)
  if (ink && ink->isSelection()) {
    doc::BrushRef brush;
    brush.reset(new Brush(kCircleBrushType, 1, 0));
    return brush;
  }

  if ((tool == nullptr) ||
      (tool == App::instance()->activeTool()) ||
      (ink && ink->isPaint() &&
       m_activeBrush->type() == kImageBrushType)) {
    m_activeBrush->setPattern(Preferences::instance().brush.pattern());
    return m_activeBrush;
  }

  return ContextBar::createBrushFromPreferences(
    &Preferences::instance().tool(tool).brush);
}

void ContextBar::discardActiveBrush()
{
  setActiveBrush(ContextBar::createBrushFromPreferences());
}

// static
doc::BrushRef ContextBar::createBrushFromPreferences(ToolPreferences::Brush* brushPref)
{
  if (brushPref == nullptr) {
    tools::Tool* tool = App::instance()->activeTool();
    brushPref = &Preferences::instance().tool(tool).brush;
  }

  doc::BrushRef brush;
  brush.reset(
    new Brush(
      static_cast<doc::BrushType>(brushPref->type()),
      brushPref->size(),
      brushPref->angle()));
  return brush;
}

BrushSlot ContextBar::createBrushSlotFromPreferences()
{
  tools::Tool* activeTool = App::instance()->activeTool();
  auto& pref = Preferences::instance();
  auto& saveBrush = pref.saveBrush;
  auto& toolPref = pref.tool(activeTool);

  int flags = 0;
  if (saveBrush.brushType()) flags |= int(BrushSlot::Flags::BrushType);
  if (saveBrush.brushSize()) flags |= int(BrushSlot::Flags::BrushSize);
  if (saveBrush.brushAngle()) flags |= int(BrushSlot::Flags::BrushAngle);
  if (saveBrush.fgColor()) flags |= int(BrushSlot::Flags::FgColor);
  if (saveBrush.bgColor()) flags |= int(BrushSlot::Flags::BgColor);
  if (saveBrush.imageColor()) flags |= int(BrushSlot::Flags::ImageColor);
  if (saveBrush.inkType()) flags |= int(BrushSlot::Flags::InkType);
  if (saveBrush.inkOpacity()) flags |= int(BrushSlot::Flags::InkOpacity);
  if (saveBrush.shade()) flags |= int(BrushSlot::Flags::Shade);
  if (saveBrush.pixelPerfect()) flags |= int(BrushSlot::Flags::PixelPerfect);

  return BrushSlot(
    BrushSlot::Flags(flags),
    activeBrush(activeTool),
    pref.colorBar.fgColor(),
    pref.colorBar.bgColor(),
    toolPref.ink(),
    toolPref.opacity(),
    getShade(),
    toolPref.freehandAlgorithm() == tools::FreehandAlgorithm::PIXEL_PERFECT);
}

Shade ContextBar::getShade() const
{
  return m_inkShades->getShade();
}

doc::Remap* ContextBar::createShadeRemap(bool left)
{
  return m_inkShades->createShadeRemap(left);
}

void ContextBar::reverseShadeColors()
{
  m_inkShades->reverseShadeColors();
}

void ContextBar::setInkType(tools::InkType type)
{
  m_inkType->setInkType(type);
}

render::DitheringMatrix ContextBar::ditheringMatrix()
{
  return m_ditheringSelector->ditheringMatrix();
}

render::DitheringAlgorithmBase* ContextBar::ditheringAlgorithm()
{
  static std::unique_ptr<render::DitheringAlgorithmBase> s_dither;

  switch (m_ditheringSelector->ditheringAlgorithm()) {
    case render::DitheringAlgorithm::None:
      s_dither.reset(nullptr);
      break;
    case render::DitheringAlgorithm::Ordered:
      s_dither.reset(new render::OrderedDither2(-1));
      break;
    case render::DitheringAlgorithm::Old:
      s_dither.reset(new render::OrderedDither(-1));
      break;
    case render::DitheringAlgorithm::ErrorDiffusion:
      s_dither.reset(new render::ErrorDiffusionDither(-1));
      break;
  }

  return s_dither.get();
}

render::GradientType ContextBar::gradientType()
{
  return m_gradientType->gradientType();
}

const tools::DynamicsOptions& ContextBar::getDynamics() const
{
  return m_dynamics->getDynamics();
}

void ContextBar::setupTooltips(TooltipManager* tooltipManager)
{
  tooltipManager->addTooltipFor(
    m_brushBack->at(0), Strings::context_bar_discard_brush(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_brushType->at(0), Strings::context_bar_brush_type(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_brushSize, Strings::context_bar_brush_size(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_brushAngle, Strings::context_bar_brush_angle(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_inkType->at(0), Strings::context_bar_ink(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_inkOpacity, Strings::context_bar_opacity(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_inkShades->at(0), Strings::context_bar_shades(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_sprayWidth, Strings::context_bar_spray_width(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_spraySpeed, Strings::context_bar_spray_speed(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_pivot->at(0), Strings::context_bar_rotation_pivot(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_rotAlgo, Strings::context_bar_rotation_algorithm(), BOTTOM);
  tooltipManager->addTooltipFor(
    m_dynamics->at(0), Strings::context_bar_dynamics(), BOTTOM);
  tooltipManager->addTooltipFor(m_freehandAlgo,
                                key_tooltip("Freehand trace algorithm",
                                            CommandId::PixelPerfectMode()), BOTTOM);
  tooltipManager->addTooltipFor(m_contiguous,
                                key_tooltip("Fill contiguous areas color",
                                            CommandId::ContiguousFill()), BOTTOM);
  tooltipManager->addTooltipFor(
    m_paintBucketSettings->at(0), Strings::context_bar_paint_bucket_option(), BOTTOM);

  m_selectionMode->setupTooltips(tooltipManager);
  m_gradientType->setupTooltips(tooltipManager);
  m_dropPixels->setupTooltips(tooltipManager);
  m_symmetry->setupTooltips(tooltipManager);
  m_sliceFields->setupTooltips(tooltipManager);
}

void ContextBar::registerCommands()
{
  Commands::instance()
    ->add(
      new QuickCommand(
        CommandId::ShowBrushes(),
        [this]{ this->showBrushes(); }));

  Commands::instance()
    ->add(
      new QuickCommand(
        CommandId::ShowDynamics(),
        [this]{ this->showDynamics(); }));
}

void ContextBar::showBrushes()
{
  if (m_brushType->isVisible())
    m_brushType->switchPopup();
}

void ContextBar::showDynamics()
{
  if (m_dynamics->isVisible())
    m_dynamics->switchPopup();
}

bool ContextBar::needZoomButtons(tools::Tool* tool) const
{
  return tool &&
    (tool->getInk(0)->isZoom() ||
     tool->getInk(1)->isZoom() ||
     tool->getInk(0)->isScrollMovement() ||
     tool->getInk(1)->isScrollMovement());
}

} // namespace app
