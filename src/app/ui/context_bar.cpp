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
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "base/unique_ptr.h"
#include "doc/brush.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/listitem.h"
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
    , m_bitmap(BrushPopup::createSurfaceForBrush(BrushRef(nullptr)))
    , m_popupWindow(this) {
    addItem(m_bitmap);
    m_popupWindow.BrushChange.connect(&BrushTypeField::onBrushChange, this);
  }

  ~BrushTypeField() {
    closePopup();

    m_bitmap->dispose();
  }

  void updateBrush(tools::Tool* tool = nullptr) {
    if (m_bitmap)
      m_bitmap->dispose();

    m_bitmap = BrushPopup::createSurfaceForBrush(
      m_owner->activeBrush(tool));

    getItem(0)->setIcon(m_bitmap);
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

  void onPreferredSize(PreferredSizeEvent& ev) {
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
  she::Surface* m_bitmap;
  BrushPopup m_popupWindow;
};

class ContextBar::BrushSizeField : public IntEntry
{
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

class ContextBar::InkTypeField : public ComboBox
{
public:
  InkTypeField() : m_lock(false) {
    // The same order as in InkType
    addItem("Default Ink");
#if 0
    addItem("Opaque");
#endif
    addItem("Set Alpha");
    addItem("Lock Alpha");
#if 0
    addItem("Merge");
    addItem("Shading");
    addItem("Replace");
    addItem("Erase");
    addItem("Selection");
    addItem("Blur");
    addItem("Jumble");
#endif
  }

  void setInkType(InkType inkType) {
    int index = 0;

    switch (inkType) {
      case InkType::DEFAULT: index = 0; break;
      case InkType::SET_ALPHA: index = 1; break;
      case InkType::LOCK_ALPHA: index = 2; break;
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

    InkType inkType = InkType::DEFAULT;

    switch (getSelectedItemIndex()) {
      case 0: inkType = InkType::DEFAULT; break;
      case 1: inkType = InkType::SET_ALPHA; break;
      case 2: inkType = InkType::LOCK_ALPHA; break;
    }

    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).ink(inkType);
  }

  void onCloseListBox() override {
    releaseFocus();
  }

  bool m_lock;
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
    Tool* tool = App::instance()->activeTool();
    Preferences::instance().tool(tool).opacity(newValue);
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


class ContextBar::TransparentColorField : public ColorButton
{
public:
  TransparentColorField() : ColorButton(app::Color::fromMask(), IMAGE_RGB) {
    Change.connect(Bind<void>(&TransparentColorField::onChange, this));
  }

protected:
  void onChange() {
    Preferences::instance().selection.transparentColor(getColor());
  }
};

class ContextBar::RotAlgorithmField : public ComboBox
{
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
    int part = PART_FREEHAND_ALGO_DEFAULT;
    m_freehandAlgo = algo;
    switch (m_freehandAlgo) {
      case kDefaultFreehandAlgorithm:
        part = PART_FREEHAND_ALGO_DEFAULT;
        break;
      case kPixelPerfectFreehandAlgorithm:
        part = PART_FREEHAND_ALGO_PIXEL_PERFECT;
        break;
      case kDotsFreehandAlgorithm:
        part = PART_FREEHAND_ALGO_DOTS;
        break;
    }
    m_bitmap = static_cast<SkinTheme*>(getTheme())->get_part(part);
    invalidate();
  }

  // IButtonIcon implementation
  void destroy() override {
    // Do nothing, BrushTypeField is added as a widget in the
    // ContextBar, so it will be destroyed together with the
    // ContextBar.
  }

  int getWidth() override {
    return m_bitmap->width();
  }

  int getHeight() override {
    return m_bitmap->height();
  }

  she::Surface* getNormalIcon() override {
    return m_bitmap;
  }

  she::Surface* getSelectedIcon() override {
    return m_bitmap;
  }

  she::Surface* getDisabledIcon() override {
    return m_bitmap;
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

  void onPreferredSize(PreferredSizeEvent& ev) {
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
    m_freehandAlgoButton->addItem(theme->get_part(PART_FREEHAND_ALGO_DEFAULT));
    m_freehandAlgoButton->addItem(theme->get_part(PART_FREEHAND_ALGO_PIXEL_PERFECT));
    m_freehandAlgoButton->addItem(theme->get_part(PART_FREEHAND_ALGO_DOTS));
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

  she::Surface* m_bitmap;
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

    addItem(theme->get_part(PART_SELECTION_REPLACE));
    addItem(theme->get_part(PART_SELECTION_ADD));
    addItem(theme->get_part(PART_SELECTION_SUBTRACT));

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

    addItem(theme->get_part(PART_DROP_PIXELS_OK));
    addItem(theme->get_part(PART_DROP_PIXELS_CANCEL));
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

class ContextBar::GrabAlphaField : public CheckBox
{
public:
  GrabAlphaField() : CheckBox("Grab Alpha") {
    setup_mini_font(this);
  }

protected:
  void onClick(Event& ev) override {
    CheckBox::onClick(ev);

    Preferences::instance().editor.grabAlpha(isSelected());

    releaseFocus();
  }
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
  m_selectionOptionsBox->addChild(m_transparentColor = new TransparentColorField);
  m_selectionOptionsBox->addChild(m_rotAlgo = new RotAlgorithmField());

  addChild(m_brushType = new BrushTypeField(this));
  addChild(m_brushSize = new BrushSizeField());
  addChild(m_brushAngle = new BrushAngleField(m_brushType));
  addChild(m_brushPatternField = new BrushPatternField());

  addChild(m_toleranceLabel = new Label("Tolerance:"));
  addChild(m_tolerance = new ToleranceField());
  addChild(m_contiguous = new ContiguousField());
  addChild(m_stopAtGrid = new StopAtGridField());

  addChild(m_inkType = new InkTypeField());

  addChild(m_opacityLabel = new Label("Opacity:"));
  addChild(m_inkOpacity = new InkOpacityField());

  addChild(m_grabAlpha = new GrabAlphaField());

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
  setup_mini_font(m_opacityLabel);

  TooltipManager* tooltipManager = new TooltipManager();
  addChild(tooltipManager);

  tooltipManager->addTooltipFor(m_brushType, "Brush Type", BOTTOM);
  tooltipManager->addTooltipFor(m_brushSize, "Brush Size (in pixels)", BOTTOM);
  tooltipManager->addTooltipFor(m_brushAngle, "Brush Angle (in degrees)", BOTTOM);
  tooltipManager->addTooltipFor(m_inkOpacity, "Opacity (Alpha value in RGBA)", BOTTOM);
  tooltipManager->addTooltipFor(m_sprayWidth, "Spray Width", BOTTOM);
  tooltipManager->addTooltipFor(m_spraySpeed, "Spray Speed", BOTTOM);
  tooltipManager->addTooltipFor(m_transparentColor, "Transparent Color", BOTTOM);
  tooltipManager->addTooltipFor(m_rotAlgo, "Rotation Algorithm", BOTTOM);
  tooltipManager->addTooltipFor(m_freehandAlgo, "Freehand trace algorithm", BOTTOM);
  tooltipManager->addTooltipFor(m_grabAlpha,
    "When checked the tool picks the color from the active layer, and its alpha\n"
    "component is used to setup the opacity level of all drawing tools.\n\n"
    "When unchecked -the default behavior- the color is picked\n"
    "from the composition of all sprite layers.", LEFT | TOP);

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

  if (toolPref) {
    m_tolerance->setTextf("%d", toolPref->tolerance());
    m_contiguous->setSelected(toolPref->contiguous());
    m_stopAtGrid->setSelected(
      toolPref->floodfill.stopAtGrid() == app::gen::StopAtGrid::IF_VISIBLE ? true: false);

    m_inkType->setInkType(toolPref->ink());
    m_inkOpacity->setTextf("%d", toolPref->opacity());

    m_freehandAlgo->setFreehandAlgorithm(toolPref->freehandAlgorithm());

    m_sprayWidth->setValue(toolPref->spray.width());
    m_spraySpeed->setValue(toolPref->spray.speed());
  }

  m_grabAlpha->setSelected(preferences.editor.grabAlpha());
  m_autoSelectLayer->setSelected(preferences.editor.autoSelectLayer());

  // True if the current tool needs opacity options
  bool hasOpacity = tool &&
    (tool->getInk(0)->isPaint() ||
     tool->getInk(0)->isEffect() ||
     tool->getInk(1)->isPaint() ||
     tool->getInk(1)->isEffect());

  // True if we have an image as brush
  bool hasImageBrush = (activeBrush()->type() == kImageBrushType);

  // True if the current tool is eyedropper.
  bool isEyedropper = tool &&
    (tool->getInk(0)->isEyedropper() ||
     tool->getInk(1)->isEyedropper());

  // True if the current tool is move tool.
  bool isMove = tool &&
    (tool->getInk(0)->isCelMovement() ||
     tool->getInk(1)->isCelMovement());

  // True if it makes sense to change the ink property for the current
  // tool.
  bool hasInk = hasOpacity;

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

  // Show/Hide fields
  m_brushType->setVisible(hasOpacity && (!isFloodfill || (isFloodfill && hasImageBrush)));
  m_brushSize->setVisible(hasOpacity && !isFloodfill && !hasImageBrush);
  m_brushAngle->setVisible(hasOpacity && !isFloodfill && !hasImageBrush);
  m_brushPatternField->setVisible(hasOpacity && hasImageBrush);
  m_opacityLabel->setVisible(hasOpacity);
  m_inkType->setVisible(hasInk && !hasImageBrush);
  m_inkOpacity->setVisible(hasOpacity);
  m_grabAlpha->setVisible(isEyedropper);
  m_autoSelectLayer->setVisible(isMove);
  m_freehandBox->setVisible(isFreehand && hasOpacity);
  m_toleranceLabel->setVisible(hasTolerance);
  m_tolerance->setVisible(hasTolerance);
  m_contiguous->setVisible(hasTolerance);
  m_stopAtGrid->setVisible(hasTolerance);
  m_sprayBox->setVisible(hasSprayOptions);
  m_selectionOptionsBox->setVisible(hasSelectOptions);
  m_selectionMode->setVisible(true);
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

} // namespace app
