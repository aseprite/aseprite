// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/context_bar.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/ink_type.h"
#include "app/tools/point_shape.h"
#include "app/tools/selection_mode.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/brush_popup.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "base/unique_ptr.h"
#include "doc/brush.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/listitem.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/popup_window.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"
#include "ui/tooltips.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace tools;

static bool g_updatingFromCode = false;

class ContextBar::BrushTypeField : public ButtonSet
                                 , public BrushPopupDelegate {
public:
  BrushTypeField(ContextBar* owner)
    : ButtonSet(1)
    , m_owner(owner)
    , m_popupWindow(this) {
    SkinPartPtr part(new SkinPart);
    part->setBitmap(
      0, BrushPopup::createSurfaceForBrush(BrushRef(nullptr)));

    addItem(part);
    m_popupWindow.BrushChange.connect(&BrushTypeField::onBrushChange, this);
  }

  ~BrushTypeField() {
    closePopup();
  }

  void updateBrush(tools::Tool* tool = nullptr) {
    SkinPartPtr part(new SkinPart);
    part->setBitmap(
      0, BrushPopup::createSurfaceForBrush(
        m_owner->activeBrush(tool)));

    getItem(0)->setIcon(part);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    m_popupWindow.setupTooltips(tooltipManager);
  }

protected:
  void onItemChange() override {
    ButtonSet::onItemChange();

    if (!m_popupWindow.isVisible())
      openPopup();
    else
      closePopup();
  }

  void onPreferredSize(PreferredSizeEvent& ev) override {
    ev.setPreferredSize(Size(16, 18)*guiscale());
  }

  // BrushPopupDelegate impl
  void onDeleteBrushSlot(int slot) override {
    m_owner->removeBrush(slot);
  }

  void onDeleteAllBrushes() override {
    m_owner->removeAllBrushes();
  }

  bool onIsBrushSlotLocked(int slot) const override {
    return m_owner->isBrushSlotLocked(slot);
  }

  void onLockBrushSlot(int slot) override {
    m_owner->lockBrushSlot(slot);
  }

  void onUnlockBrushSlot(int slot) override {
    m_owner->unlockBrushSlot(slot);
  }

private:
  // Returns a little rectangle that can be used by the popup as the
  // first brush position.
  gfx::Rect getPopupBox() {
    Rect rc = getBounds();
    rc.y += rc.h - 2*guiscale();
    rc.setSize(getPreferredSize());
    return rc;
  }

  void openPopup() {
    doc::BrushRef brush = m_owner->activeBrush();

    m_popupWindow.regenerate(getPopupBox(), m_owner->getBrushes());
    m_popupWindow.setBrush(brush.get());

    Region rgn(m_popupWindow.getBounds().createUnion(getBounds()));
    m_popupWindow.setHotRegion(rgn);

    m_popupWindow.openWindow();
  }

  void closePopup() {
    m_popupWindow.closeWindow(NULL);
    deselectItems();
  }

  void onBrushChange(const BrushRef& brush) {
    if (brush->type() == kImageBrushType)
      m_owner->setActiveBrush(brush);
    else {
      Tool* tool = App::instance()->activeTool();
      ToolPreferences::Brush& brushPref = Preferences::instance().tool(tool).brush;

      brushPref.type(static_cast<app::gen::BrushType>(brush->type()));

      m_owner->setActiveBrush(
        ContextBar::createBrushFromPreferences(&brushPref));
    }
  }

  ContextBar* m_owner;
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
    base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.size(getValue());
  }
};

class ContextBar::BrushAngleField : public IntEntry
{
public:
  BrushAngleField(BrushTypeField* brushType)
    : IntEntry(0, 180)
    , m_brushType(brushType) {
    setSuffix("\xc2\xb0");
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).brush.angle(getValue());

    m_brushType->updateBrush();
  }

private:
  BrushTypeField* m_brushType;
};

class ContextBar::BrushPatternField : public ComboBox
{
public:
  BrushPatternField() : m_lock(false) {
    addItem("Pattern aligned to source");
    addItem("Pattern aligned to destination");
    addItem("Paint brush");
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

class ContextBar::ToleranceField : public IntEntry
{
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

class ContextBar::ContiguousField : public CheckBox
{
public:
  ContiguousField() : CheckBox("Contiguous") {
    setup_mini_font(this);
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).contiguous(isSelected());

    releaseFocus();
  }
};

class ContextBar::StopAtGridField : public CheckBox
{
public:
  StopAtGridField() : CheckBox("Stop at Grid") {
    setup_mini_font(this);
  }

  void setStopAtGrid(bool state) {
    setSelected(state);
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).floodfill.stopAtGrid(
      (isSelected() ? app::gen::StopAtGrid::IF_VISIBLE:
                      app::gen::StopAtGrid::NEVER));

    releaseFocus();
  }
};

class ContextBar::InkTypeField : public ButtonSet {
public:
  InkTypeField(ContextBar* owner) : ButtonSet(1)
                                  , m_owner(owner) {
    SkinTheme* theme = SkinTheme::instance();
    addItem(theme->parts.inkSimple());
  }

  void setInkType(InkType inkType) {
    SkinTheme* theme = SkinTheme::instance();
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
  void onItemChange() override {
    ButtonSet::onItemChange();

    gfx::Rect bounds = getBounds();

    Menu menu;
    MenuItem
      simple("Simple Ink"),
      alphacompo("Alpha Compositing"),
      copycolor("Copy Color+Alpha"),
      lockalpha("Lock Alpha"),
      shading("Shading"),
      alltools("Same in all Tools");
    menu.addChild(&simple);
    menu.addChild(&alphacompo);
    menu.addChild(&copycolor);
    menu.addChild(&lockalpha);
    menu.addChild(&shading);
    menu.addChild(new MenuSeparator);
    menu.addChild(&alltools);

    Tool* tool = App::instance()->activeTool();
    switch (Preferences::instance().tool(tool).ink()) {
      case tools::InkType::SIMPLE: simple.setSelected(true); break;
      case tools::InkType::ALPHA_COMPOSITING: alphacompo.setSelected(true); break;
      case tools::InkType::COPY_COLOR: copycolor.setSelected(true); break;
      case tools::InkType::LOCK_ALPHA: lockalpha.setSelected(true); break;
      case tools::InkType::SHADING: shading.setSelected(true); break;
    }
    alltools.setSelected(Preferences::instance().shared.shareInk());

    simple.Click.connect(Bind<void>(&InkTypeField::selectInk, this, InkType::SIMPLE));
    alphacompo.Click.connect(Bind<void>(&InkTypeField::selectInk, this, InkType::ALPHA_COMPOSITING));
    copycolor.Click.connect(Bind<void>(&InkTypeField::selectInk, this, InkType::COPY_COLOR));
    lockalpha.Click.connect(Bind<void>(&InkTypeField::selectInk, this, InkType::LOCK_ALPHA));
    shading.Click.connect(Bind<void>(&InkTypeField::selectInk, this, InkType::SHADING));
    alltools.Click.connect(Bind<void>(&InkTypeField::onSameInAllTools, this));

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));

    deselectItems();
  }

  void selectInk(InkType inkType) {
    Preferences& pref = Preferences::instance();

    if (pref.shared.shareInk()) {
      for (Tool* tool : *App::instance()->getToolBox())
        pref.tool(tool).ink(inkType);
    }
    else {
      Tool* tool = App::instance()->activeTool();
      pref.tool(tool).ink(inkType);
    }

    m_owner->updateForCurrentTool();
  }

  void onSameInAllTools() {
    Preferences& pref = Preferences::instance();
    bool newState = !pref.shared.shareInk();
    pref.shared.shareInk(newState);

    if (newState) {
      Tool* activeTool = App::instance()->activeTool();
      InkType inkType = pref.tool(activeTool).ink();
      int opacity = pref.tool(activeTool).opacity();

      for (Tool* tool : *App::instance()->getToolBox()) {
        if (tool != activeTool) {
          pref.tool(tool).ink(inkType);
          pref.tool(tool).opacity(opacity);
        }
      }
    }
  }

  ContextBar* m_owner;
};

class ContextBar::InkShadesField : public Widget {
  typedef std::vector<app::Color> Colors;
public:

  InkShadesField() : Widget(kGenericWidget) {
    setText("Select colors in the palette");
  }

  doc::Remap* createShadesRemap(bool left) {
    base::UniquePtr<doc::Remap> remap;
    Colors colors = getColors();

    if (colors.size() > 0) {
      remap.reset(new doc::Remap(get_current_palette()->size()));

      for (int i=0; i<remap->size(); ++i)
        remap->map(i, i);

      if (left) {
        for (int i=1; i<int(colors.size()); ++i)
          remap->map(colors[i].getIndex(), colors[i-1].getIndex());
      }
      else {
        for (int i=0; i<int(colors.size())-1; ++i)
          remap->map(colors[i].getIndex(), colors[i+1].getIndex());
      }
    }

    return remap.release();
  }

private:

  Colors getColors() const {
    Colors colors;
    for (const auto& color : m_colors) {
      if (color.getIndex() >= 0 &&
          color.getIndex() < get_current_palette()->size())
        colors.push_back(color);
    }
    return colors;
  }

  void onChangeColorBarSelection() {
    if (!isVisible())
      return;

    doc::PalettePicks picks;
    ColorBar::instance()->getPaletteView()->getSelectedEntries(picks);

    m_colors.resize(picks.picks());

    int i = 0, j = 0;
    for (bool pick : picks) {
      if (pick)
        m_colors[j++] = app::Color::fromIndex(i);
      ++i;
    }

    getParent()->layout();
  }

  bool onProcessMessage(ui::Message* msg) {
    if (msg->type() == kOpenMessage) {
      ColorBar::instance()->ChangeSelection.connect(
        Bind<void>(&InkShadesField::onChangeColorBarSelection, this));
    }
    return Widget::onProcessMessage(msg);
  }

  void onPreferredSize(PreferredSizeEvent& ev) override {
    int size = getColors().size();

    if (size < 2)
      ev.setPreferredSize(Size(16+getTextWidth(), 18)*guiscale());
    else
      ev.setPreferredSize(Size(6+12*size, 18)*guiscale());
  }

  void onPaint(PaintEvent& ev) override {
    SkinTheme* theme = SkinTheme::instance();
    Graphics* g = ev.getGraphics();
    gfx::Rect bounds = getClientBounds();

    skin::Style::State state;
    if (hasMouseOver()) state += Style::hover();

    g->fillRect(theme->colors.workspace(), bounds);
    theme->styles.view()->paint(g, bounds, nullptr, state);

    bounds.shrink(3*guiscale());

    gfx::Rect box(bounds.x, bounds.y, 12*guiscale(), bounds.h);
    Colors colors = getColors();

    if (colors.size() >= 2) {
      for (int i=0; i<int(colors.size()); ++i) {
        if (i == int(colors.size())-1)
          box.w = bounds.x+bounds.w-box.x;

        draw_color(g, box, colors[i]);
        box.x += box.w;
      }
    }
    else {
      g->fillRect(theme->colors.editorFace(), bounds);
      g->drawAlignedUIString(getText(), theme->colors.face(), gfx::ColorNone, bounds,
                             ui::CENTER | ui::MIDDLE);
    }
  }

  std::vector<app::Color> m_colors;
};

class ContextBar::InkOpacityField : public IntEntry
{
public:
  InkOpacityField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() override {
    if (g_updatingFromCode)
      return;

    IntEntry::onValueChange();
    base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

    int newValue = getValue();
    Preferences& pref = Preferences::instance();
    if (pref.shared.shareInk()) {
      for (Tool* tool : *App::instance()->getToolBox())
        pref.tool(tool).opacity(newValue);
    }
    else {
      Tool* tool = App::instance()->activeTool();
      pref.tool(tool).opacity(newValue);
    }
  }
};

class ContextBar::SprayWidthField : public IntEntry
{
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

class ContextBar::SpraySpeedField : public IntEntry
{
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
  TransparentColorField(ContextBar* owner)
    : m_icon(1)
    , m_maskColor(app::Color::fromMask(), IMAGE_RGB)
    , m_owner(owner) {
    SkinTheme* theme = SkinTheme::instance();

    addChild(&m_icon);
    addChild(&m_maskColor);

    m_icon.addItem(theme->parts.selectionOpaque());
    gfx::Size sz = m_icon.getItem(0)->getPreferredSize();
    sz.w += 2*guiscale();
    m_icon.getItem(0)->setMinSize(sz);

    m_icon.ItemChange.connect(Bind<void>(&TransparentColorField::onPopup, this));
    m_maskColor.Change.connect(Bind<void>(&TransparentColorField::onChangeColor, this));

    Preferences::instance().selection.opaque.AfterChange.connect(
      Bind<void>(&TransparentColorField::onOpaqueChange, this));

    onOpaqueChange();
  }

private:

  void onPopup() {
    gfx::Rect bounds = getBounds();

    Menu menu;
    MenuItem
      opaque("Opaque"),
      masked("Transparent");
    menu.addChild(&opaque);
    menu.addChild(&masked);

    if (Preferences::instance().selection.opaque())
      opaque.setSelected(true);
    else
      masked.setSelected(true);

    opaque.Click.connect(Bind<void>(&TransparentColorField::setOpaque, this, true));
    masked.Click.connect(Bind<void>(&TransparentColorField::setOpaque, this, false));

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
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

    SkinTheme* theme = SkinTheme::instance();
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

  ButtonSet m_icon;
  ColorButton m_maskColor;
  ContextBar* m_owner;
};

class ContextBar::PivotField : public ButtonSet {
public:
  PivotField()
    : ButtonSet(1) {
    addItem(SkinTheme::instance()->parts.pivotCenter());

    ItemChange.connect(Bind<void>(&PivotField::onPopup, this));

    Preferences::instance().selection.pivotPosition.AfterChange.connect(
      Bind<void>(&PivotField::onPivotChange, this));

    onPivotChange();
  }

private:

  void onPopup() {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

    gfx::Rect bounds = getBounds();

    Menu menu;
    CheckBox visible("Display pivot by default");
    HBox box;
    ButtonSet buttonset(3);
    buttonset.addItem(theme->parts.pivotNorthwest());
    buttonset.addItem(theme->parts.pivotNorth());
    buttonset.addItem(theme->parts.pivotNortheast());
    buttonset.addItem(theme->parts.pivotWest());
    buttonset.addItem(theme->parts.pivotCenter());
    buttonset.addItem(theme->parts.pivotEast());
    buttonset.addItem(theme->parts.pivotSouthwest());
    buttonset.addItem(theme->parts.pivotSouth());
    buttonset.addItem(theme->parts.pivotSoutheast());
    box.addChild(&buttonset);

    menu.addChild(&visible);
    menu.addChild(new MenuSeparator);
    menu.addChild(&box);

    bool isVisible = Preferences::instance().selection.pivotVisibility();
    app::gen::PivotPosition pos = Preferences::instance().selection.pivotPosition();
    visible.setSelected(isVisible);
    buttonset.setSelectedItem(int(pos));

    visible.Click.connect(
      [&visible](Event&){
        Preferences::instance().selection.pivotVisibility(
          visible.isSelected());
      });

    buttonset.ItemChange.connect(
      [&buttonset](){
        Preferences::instance().selection.pivotPosition(
          app::gen::PivotPosition(buttonset.selectedItem()));
      });

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
  }

  void onPivotChange() {
    SkinTheme* theme = SkinTheme::instance();
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

};

class ContextBar::RotAlgorithmField : public ComboBox {
public:
  RotAlgorithmField() {
    // We use "m_lockChange" variable to avoid setting the rotation
    // algorithm when we call ComboBox::addItem() (because the first
    // addItem() generates an onChange() event).
    m_lockChange = true;
    addItem(new Item("Fast Rotation", tools::RotationAlgorithm::FAST));
    addItem(new Item("RotSprite", tools::RotationAlgorithm::ROTSPRITE));
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

#if 0 // TODO for v1.1 to avoid changing the UI

class ContextBar::FreehandAlgorithmField : public Button
                                         , public IButtonIcon
{
public:
  FreehandAlgorithmField()
    : Button("")
    , m_popupWindow(NULL)
    , m_tooltipManager(NULL) {
    setup_mini_look(this);
    setIconInterface(this);
  }

  ~FreehandAlgorithmField() {
    closePopup();
    setIconInterface(NULL);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    m_tooltipManager = tooltipManager;
  }

  void setFreehandAlgorithm(FreehandAlgorithm algo) {
    SkinTheme* theme = SkinTheme::instance();
    SkinPartPtr part = theme->parts.freehandAlgoDefault();
    m_freehandAlgo = algo;
    switch (m_freehandAlgo) {
      case kDefaultFreehandAlgorithm:
        part = theme->parts.freehandAlgoDefault();
        break;
      case kPixelPerfectFreehandAlgorithm:
        part = theme->parts.freehandAlgoPixelPerfect();
        break;
      case kDotsFreehandAlgorithm:
        part = theme->parts.freehandAlgoDots();
        break;
    }
    m_bitmap = part;
    invalidate();
  }

  // IButtonIcon implementation
  void destroy() override {
    // Do nothing, BrushTypeField is added as a widget in the
    // ContextBar, so it will be destroyed together with the
    // ContextBar.
  }

  gfx::Size getSize() override {
    return m_bitmap->getSize();
  }

  she::Surface* getNormalIcon() override {
    return m_bitmap->getBitmap(0);
  }

  she::Surface* getSelectedIcon() override {
    return m_bitmap->getBitmap(0);
  }

  she::Surface* getDisabledIcon() override {
    return m_bitmap->getBitmap(0);
  }

  int getIconAlign() override {
    return CENTER | MIDDLE;
  }

protected:
  void onClick(Event& ev) override {
    Button::onClick(ev);

    if (!m_popupWindow || !m_popupWindow->isVisible())
      openPopup();
    else
      closePopup();
  }

  void onPreferredSize(PreferredSizeEvent& ev) override {
    ev.setPreferredSize(Size(16, 18)*guiscale());
  }

private:
  void openPopup() {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

    Border border = Border(2, 2, 2, 3)*guiscale();
    Rect rc = getBounds();
    rc.y += rc.h;
    rc.w *= 3;
    m_popupWindow = new PopupWindow("", PopupWindow::kCloseOnClickInOtherWindow);
    m_popupWindow->setAutoRemap(false);
    m_popupWindow->setBorder(border);
    m_popupWindow->setBounds(rc + border);

    Region rgn(m_popupWindow->getBounds().createUnion(getBounds()));
    m_popupWindow->setHotRegion(rgn);
    m_freehandAlgoButton = new ButtonSet(3);
    m_freehandAlgoButton->addItem(theme->parts.freehandAlgoDefault());
    m_freehandAlgoButton->addItem(theme->parts.freehandAlgoPixelPerfect());
    m_freehandAlgoButton->addItem(theme->parts.freehandAlgoDots());
    m_freehandAlgoButton->setSelectedItem((int)m_freehandAlgo);
    m_freehandAlgoButton->ItemChange.connect(&FreehandAlgorithmField::onFreehandAlgoChange, this);
    m_freehandAlgoButton->setTransparent(true);
    m_freehandAlgoButton->setBgColor(gfx::ColorNone);

    m_tooltipManager->addTooltipFor(at(0), "Normal trace", TOP);
    m_tooltipManager->addTooltipFor(at(1), "Pixel-perfect trace", TOP);
    m_tooltipManager->addTooltipFor(at(2), "Dots", TOP);

    m_popupWindow->addChild(m_freehandAlgoButton);
    m_popupWindow->openWindow();
  }

  void closePopup() {
    if (m_popupWindow) {
      m_popupWindow->closeWindow(NULL);
      delete m_popupWindow;
      m_popupWindow = NULL;
      m_freehandAlgoButton = NULL;
    }
  }

  void onFreehandAlgoChange() {
    setFreehandAlgorithm(
      (FreehandAlgorithm)m_freehandAlgoButton->getSelectedItem());

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).freehandAlgorithm(m_freehandAlgo);
  }

  SkinPartPtr m_bitmap;
  FreehandAlgorithm m_freehandAlgo;
  PopupWindow* m_popupWindow;
  ButtonSet* m_freehandAlgoButton;
  TooltipManager* m_tooltipManager;
};

#else

class ContextBar::FreehandAlgorithmField : public CheckBox
{
public:
  FreehandAlgorithmField() : CheckBox("Pixel-perfect") {
    setup_mini_font(this);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    // Do nothing
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
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).freehandAlgorithm(
      isSelected() ?
        tools::FreehandAlgorithm::PIXEL_PERFECT:
        tools::FreehandAlgorithm::DEFAULT);

    releaseFocus();
  }
};

#endif

class ContextBar::SelectionModeField : public ButtonSet
{
public:
  SelectionModeField() : ButtonSet(3) {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

    addItem(theme->parts.selectionReplace());
    addItem(theme->parts.selectionAdd());
    addItem(theme->parts.selectionSubtract());

    setSelectedItem((int)Preferences::instance().selection.mode());
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(at(0), "Replace selection", BOTTOM);
    tooltipManager->addTooltipFor(at(1), "Add to selection\n(Shift)", BOTTOM);
    tooltipManager->addTooltipFor(at(2), "Subtract from selection\n(Shift+Alt)", BOTTOM);
  }

  void setSelectionMode(SelectionMode mode) {
    setSelectedItem((int)mode);
    invalidate();
  }

protected:
  void onItemChange() override {
    ButtonSet::onItemChange();

    Preferences::instance().selection.mode(
      (tools::SelectionMode)selectedItem());
  }
};

class ContextBar::DropPixelsField : public ButtonSet
{
public:
  DropPixelsField() : ButtonSet(2) {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

    addItem(theme->parts.dropPixelsOk());
    addItem(theme->parts.dropPixelsCancel());
    setOfferCapture(false);
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(at(0), "Drop pixels here", BOTTOM);
    tooltipManager->addTooltipFor(at(1), "Cancel drag and drop", BOTTOM);
  }

  Signal1<void, ContextBarObserver::DropAction> DropPixels;

protected:
  void onItemChange() override {
    ButtonSet::onItemChange();

    switch (selectedItem()) {
      case 0: DropPixels(ContextBarObserver::DropPixels); break;
      case 1: DropPixels(ContextBarObserver::CancelDrag); break;
    }
  }
};

class ContextBar::EyedropperField : public HBox
{
public:
  EyedropperField() {
    m_channel.addItem("Color+Alpha");
    m_channel.addItem("Color");
    m_channel.addItem("Alpha");
    m_channel.addItem("RGB+Alpha");
    m_channel.addItem("RGB");
    m_channel.addItem("HSB+Alpha");
    m_channel.addItem("HSB");
    m_channel.addItem("Gray+Alpha");
    m_channel.addItem("Gray");
    m_channel.addItem("Best fit Index");

    m_sample.addItem("All Layers");
    m_sample.addItem("Current Layer");

    addChild(new Label("Pick:"));
    addChild(&m_channel);
    addChild(new Label("Sample:"));
    addChild(&m_sample);

    m_channel.Change.connect(Bind<void>(&EyedropperField::onChannelChange, this));
    m_sample.Change.connect(Bind<void>(&EyedropperField::onSampleChange, this));
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

class ContextBar::AutoSelectLayerField : public CheckBox
{
public:
  AutoSelectLayerField() : CheckBox("Auto Select Layer") {
    setup_mini_font(this);
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Preferences::instance().editor.autoSelectLayer(isSelected());

    releaseFocus();
  }
};

ContextBar::ContextBar()
  : Box(HORIZONTAL)
{
  gfx::Border border = this->border();
  border.bottom(2*guiscale());
  setBorder(border);

  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->colors.workspace());

  addChild(m_selectionOptionsBox = new HBox());
  m_selectionOptionsBox->addChild(m_dropPixels = new DropPixelsField());
  m_selectionOptionsBox->addChild(m_selectionMode = new SelectionModeField);
  m_selectionOptionsBox->addChild(m_transparentColor = new TransparentColorField(this));
  m_selectionOptionsBox->addChild(m_pivot = new PivotField);
  m_selectionOptionsBox->addChild(m_rotAlgo = new RotAlgorithmField());

  addChild(m_brushType = new BrushTypeField(this));
  addChild(m_brushSize = new BrushSizeField());
  addChild(m_brushAngle = new BrushAngleField(m_brushType));
  addChild(m_brushPatternField = new BrushPatternField());

  addChild(m_toleranceLabel = new Label("Tolerance:"));
  addChild(m_tolerance = new ToleranceField());
  addChild(m_contiguous = new ContiguousField());
  addChild(m_stopAtGrid = new StopAtGridField());

  addChild(m_inkType = new InkTypeField(this));
  addChild(m_inkOpacityLabel = new Label("Opacity:"));
  addChild(m_inkOpacity = new InkOpacityField());
  addChild(m_inkShades = new InkShadesField());

  addChild(m_eyedropperField = new EyedropperField());

  addChild(m_autoSelectLayer = new AutoSelectLayerField());

  // addChild(new InkChannelTargetField());
  // addChild(new InkShadeField());
  // addChild(new InkSelectionField());

  addChild(m_sprayBox = new HBox());
  m_sprayBox->addChild(m_sprayLabel = new Label("Spray:"));
  m_sprayBox->addChild(m_sprayWidth = new SprayWidthField());
  m_sprayBox->addChild(m_spraySpeed = new SpraySpeedField());

  addChild(m_selectBoxHelp = new Label(""));

  setup_mini_font(m_sprayLabel);

  addChild(m_freehandBox = new HBox());
#if 0                           // TODO for v1.1
  m_freehandBox->addChild(m_freehandLabel = new Label("Freehand:"));
  setup_mini_font(m_freehandLabel);
#endif
  m_freehandBox->addChild(m_freehandAlgo = new FreehandAlgorithmField());

  setup_mini_font(m_toleranceLabel);
  setup_mini_font(m_inkOpacityLabel);

  TooltipManager* tooltipManager = new TooltipManager();
  addChild(tooltipManager);

  tooltipManager->addTooltipFor(m_brushType, "Brush Type", BOTTOM);
  tooltipManager->addTooltipFor(m_brushSize, "Brush Size (in pixels)", BOTTOM);
  tooltipManager->addTooltipFor(m_brushAngle, "Brush Angle (in degrees)", BOTTOM);
  tooltipManager->addTooltipFor(m_inkType, "Ink", BOTTOM);
  tooltipManager->addTooltipFor(m_inkOpacity, "Opacity (paint intensity)", BOTTOM);
  tooltipManager->addTooltipFor(m_inkShades, "Shades", BOTTOM);
  tooltipManager->addTooltipFor(m_sprayWidth, "Spray Width", BOTTOM);
  tooltipManager->addTooltipFor(m_spraySpeed, "Spray Speed", BOTTOM);
  tooltipManager->addTooltipFor(m_pivot, "Rotation Pivot", BOTTOM);
  tooltipManager->addTooltipFor(m_transparentColor, "Transparent Color", BOTTOM);
  tooltipManager->addTooltipFor(m_rotAlgo, "Rotation Algorithm", BOTTOM);
  tooltipManager->addTooltipFor(m_freehandAlgo, "Freehand trace algorithm", BOTTOM);

  m_brushType->setupTooltips(tooltipManager);
  m_selectionMode->setupTooltips(tooltipManager);
  m_dropPixels->setupTooltips(tooltipManager);
  m_freehandAlgo->setupTooltips(tooltipManager);

  Preferences::instance().toolBox.activeTool.AfterChange.connect(
    Bind<void>(&ContextBar::onCurrentToolChange, this));

  m_dropPixels->DropPixels.connect(&ContextBar::onDropPixels, this);

  setActiveBrush(createBrushFromPreferences());
}

void ContextBar::onPreferredSize(PreferredSizeEvent& ev)
{
  ev.setPreferredSize(gfx::Size(0, 18*guiscale())); // TODO calculate height
}

void ContextBar::onToolSetOpacity(const int& newOpacity)
{
  if (g_updatingFromCode)
    return;

  m_inkOpacity->setTextf("%d", newOpacity);
}

void ContextBar::onBrushSizeChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    discardActiveBrush();

  updateForCurrentTool();
}

void ContextBar::onBrushAngleChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    discardActiveBrush();
}

void ContextBar::onCurrentToolChange()
{
  if (m_activeBrush->type() != kImageBrushType)
    setActiveBrush(ContextBar::createBrushFromPreferences());
  else {
    updateForCurrentTool();
  }
}

void ContextBar::onDropPixels(ContextBarObserver::DropAction action)
{
  notifyObservers(&ContextBarObserver::onDropPixels, action);
}

void ContextBar::updateForCurrentTool()
{
  updateForTool(App::instance()->activeTool());
}

void ContextBar::updateForTool(tools::Tool* tool)
{
  base::ScopedValue<bool> lockFlag(g_updatingFromCode, true, g_updatingFromCode);

  ToolPreferences* toolPref = nullptr;
  ToolPreferences::Brush* brushPref = nullptr;
  Preferences& preferences = Preferences::instance();

  if (tool) {
    toolPref = &preferences.tool(tool);
    brushPref = &toolPref->brush;
  }

  if (toolPref) {
    m_sizeConn = brushPref->size.AfterChange.connect(Bind<void>(&ContextBar::onBrushSizeChange, this));
    m_angleConn = brushPref->angle.AfterChange.connect(Bind<void>(&ContextBar::onBrushAngleChange, this));
    m_opacityConn = toolPref->opacity.AfterChange.connect(&ContextBar::onToolSetOpacity, this);
  }

  if (tool)
    m_brushType->updateBrush(tool);

  if (brushPref) {
    m_brushSize->setTextf("%d", brushPref->size());
    m_brushAngle->setTextf("%d", brushPref->angle());
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
    m_stopAtGrid->setSelected(
      toolPref->floodfill.stopAtGrid() == app::gen::StopAtGrid::IF_VISIBLE ? true: false);

    m_inkType->setInkType(toolPref->ink());
    m_inkOpacity->setTextf("%d", toolPref->opacity());

    hasInkWithOpacity =
      ((isPaint && tools::inkHasOpacity(toolPref->ink())) ||
       (isEffect));

    hasInkShades =
      (isPaint && toolPref->ink() == InkType::SHADING);

    m_freehandAlgo->setFreehandAlgorithm(toolPref->freehandAlgorithm());

    m_sprayWidth->setValue(toolPref->spray.width());
    m_spraySpeed->setValue(toolPref->spray.speed());
  }

  m_eyedropperField->updateFromPreferences(preferences.eyedropper);
  m_autoSelectLayer->setSelected(preferences.editor.autoSelectLayer());

  // True if we have an image as brush
  bool hasImageBrush = (activeBrush()->type() == kImageBrushType);

  // True if the brush type supports angle.
  bool hasBrushWithAngle =
    (activeBrush()->size() > 1) &&
    (activeBrush()->type() == kSquareBrushType ||
     activeBrush()->type() == kLineBrushType);

  // True if the current tool is eyedropper.
  bool isEyedropper = tool &&
    (tool->getInk(0)->isEyedropper() ||
     tool->getInk(1)->isEyedropper());

  // True if the current tool is move tool.
  bool isMove = tool &&
    (tool->getInk(0)->isCelMovement() ||
     tool->getInk(1)->isCelMovement());

  // True if the current tool is floodfill
  bool isFloodfill = tool &&
    (tool->getPointShape(0)->isFloodFill() ||
     tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs tolerance options
  bool hasTolerance = tool &&
    (tool->getPointShape(0)->isFloodFill() ||
     tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs spray options
  bool hasSprayOptions = tool &&
    (tool->getPointShape(0)->isSpray() ||
     tool->getPointShape(1)->isSpray());

  bool hasSelectOptions = tool &&
    (tool->getInk(0)->isSelection() ||
     tool->getInk(1)->isSelection());

  bool isFreehand = tool &&
    (tool->getController(0)->isFreehand() ||
     tool->getController(1)->isFreehand());

  bool showOpacity =
    (supportOpacity) &&
    ((isPaint && (hasInkWithOpacity || hasImageBrush)) ||
     (isEffect));

  // Show/Hide fields
  m_brushType->setVisible(supportOpacity && (!isFloodfill || (isFloodfill && hasImageBrush)));
  m_brushSize->setVisible(supportOpacity && !isFloodfill && !hasImageBrush);
  m_brushAngle->setVisible(supportOpacity && !isFloodfill && !hasImageBrush && hasBrushWithAngle);
  m_brushPatternField->setVisible(supportOpacity && hasImageBrush);
  m_inkType->setVisible(hasInk && !hasImageBrush);
  m_inkOpacityLabel->setVisible(showOpacity);
  m_inkOpacity->setVisible(showOpacity);
  m_inkShades->setVisible(hasInkShades);
  m_eyedropperField->setVisible(isEyedropper);
  m_autoSelectLayer->setVisible(isMove);
  m_freehandBox->setVisible(isFreehand && supportOpacity);
  m_toleranceLabel->setVisible(hasTolerance);
  m_tolerance->setVisible(hasTolerance);
  m_contiguous->setVisible(hasTolerance);
  m_stopAtGrid->setVisible(hasTolerance);
  m_sprayBox->setVisible(hasSprayOptions);
  m_selectionOptionsBox->setVisible(hasSelectOptions);
  m_selectionMode->setVisible(true);
  m_pivot->setVisible(true);
  m_dropPixels->setVisible(false);
  m_selectBoxHelp->setVisible(false);

  layout();
}

void ContextBar::updateForMovingPixels()
{
  tools::Tool* tool = App::instance()->getToolBox()->getToolById(
    tools::WellKnownTools::RectangularMarquee);
  if (tool)
    updateForTool(tool);

  m_dropPixels->deselectItems();
  m_dropPixels->setVisible(true);
  m_selectionMode->setVisible(false);
  layout();
}

void ContextBar::updateForSelectingBox(const std::string& text)
{
  if (m_selectBoxHelp->isVisible() && m_selectBoxHelp->getText() == text)
    return;

  updateForTool(nullptr);
  m_selectBoxHelp->setText(text);
  m_selectBoxHelp->setVisible(true);
  layout();
}

void ContextBar::updateSelectionMode(SelectionMode mode)
{
  if (!m_selectionMode->isVisible())
    return;

  m_selectionMode->setSelectionMode(mode);
}

void ContextBar::updateAutoSelectLayer(bool state)
{
  if (!m_autoSelectLayer->isVisible())
    return;

  m_autoSelectLayer->setSelected(state);
}

int ContextBar::addBrush(const doc::BrushRef& brush)
{
  // Use an empty slot
  for (size_t i=0; i<m_brushes.size(); ++i) {
    if (!m_brushes[i].locked ||
        !m_brushes[i].brush) {
      m_brushes[i].brush = brush;
      return i+1;
    }
  }

  m_brushes.push_back(BrushSlot(brush));
  return (int)m_brushes.size(); // Returns the slot
}

void ContextBar::removeBrush(int slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_brushes.size()) {
    m_brushes[slot].brush.reset();

    // Erase empty trailing slots
    while (!m_brushes.empty() &&
           !m_brushes[m_brushes.size()-1].brush)
      m_brushes.erase(--m_brushes.end());
  }
}

void ContextBar::removeAllBrushes()
{
  while (!m_brushes.empty())
    m_brushes.erase(--m_brushes.end());
}

void ContextBar::setActiveBrushBySlot(int slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_brushes.size() &&
      m_brushes[slot].brush) {
    m_brushes[slot].locked = true;
    setActiveBrush(m_brushes[slot].brush);
  }
}

Brushes ContextBar::getBrushes()
{
  Brushes brushes;
  for (const auto& slot : m_brushes)
    brushes.push_back(slot.brush);
  return brushes;
}

void ContextBar::lockBrushSlot(int slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_brushes.size() &&
      m_brushes[slot].brush) {
    m_brushes[slot].locked = true;
  }
}

void ContextBar::unlockBrushSlot(int slot)
{
  --slot;
  if (slot >= 0 && slot < (int)m_brushes.size() &&
      m_brushes[slot].brush) {
    m_brushes[slot].locked = false;
  }
}

bool ContextBar::isBrushSlotLocked(int slot) const
{
  --slot;
  if (slot >= 0 && slot < (int)m_brushes.size() &&
      m_brushes[slot].brush) {
    return m_brushes[slot].locked;
  }
  else
    return false;
}

void ContextBar::setActiveBrush(const doc::BrushRef& brush)
{
  m_activeBrush = brush;
  BrushChange();

  updateForCurrentTool();
}

doc::BrushRef ContextBar::activeBrush(tools::Tool* tool) const
{
  if (!tool ||
      (tool->getInk(0)->isPaint() &&
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

doc::Remap* ContextBar::createShadesRemap(bool left)
{
  return m_inkShades->createShadesRemap(left);
}

} // namespace app
