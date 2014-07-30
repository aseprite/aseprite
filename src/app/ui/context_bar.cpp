/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/context_bar.h"

#include "app/app.h"
#include "app/modules/gui.h"
#include "app/settings/ink_type.h"
#include "app/settings/selection_mode.h"
#include "app/settings/settings.h"
#include "app/settings/settings_observers.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/unique_ptr.h"
#include "raster/brush.h"
#include "raster/conversion_she.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "she/scoped_surface_lock.h"
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

class ContextBar::BrushTypeField : public Button
                                 , public IButtonIcon {
public:
  BrushTypeField()
    : Button("")
    , m_popupWindow(NULL)
    , m_brushTypeButton(NULL) {
    setup_mini_look(this);
    setIconInterface(this);

    m_bitmap = she::instance()->createRgbaSurface(8, 8);
    she::ScopedSurfaceLock lock(m_bitmap);
    lock->clear();
  }

  ~BrushTypeField() {
    closePopup();
    setIconInterface(NULL);

    m_bitmap->dispose();
  }

  void setBrushSettings(IBrushSettings* brushSettings) {
    base::UniquePtr<Palette> palette(new Palette(FrameNumber(0), 2));
    palette->setEntry(0, raster::rgba(0, 0, 0, 0));
    palette->setEntry(1, raster::rgba(0, 0, 0, 255));

    base::UniquePtr<Brush> brush(
      new Brush(
        m_brushType = brushSettings->getType(),
        std::min(10, brushSettings->getSize()),
        brushSettings->getAngle()));

    Image* image = brush->image();

    if (m_bitmap)
      m_bitmap->dispose();

    m_bitmap = she::instance()->createRgbaSurface(image->width(), image->height());
    convert_image_to_surface(image, m_bitmap, 0, 0, palette);

    invalidate();
  }

  // IButtonIcon implementation
  void destroy() OVERRIDE {
    // Do nothing, BrushTypeField is added as a widget in the
    // ContextBar, so it will be destroyed together with the
    // ContextBar.
  }

  int getWidth() OVERRIDE {
    return m_bitmap->width();
  }

  int getHeight() OVERRIDE {
    return m_bitmap->height();
  }

  she::Surface* getNormalIcon() OVERRIDE {
    return m_bitmap;
  }

  she::Surface* getSelectedIcon() OVERRIDE {
    return m_bitmap;
  }

  she::Surface* getDisabledIcon() OVERRIDE {
    return m_bitmap;
  }

  int getIconAlign() OVERRIDE {
    return JI_CENTER | JI_MIDDLE;
  }

protected:
  void onClick(Event& ev) OVERRIDE {
    Button::onClick(ev);

    if (!m_popupWindow || !m_popupWindow->isVisible())
      openPopup();
    else
      closePopup();
  }

  void onPreferredSize(PreferredSizeEvent& ev) {
    ev.setPreferredSize(Size(16*jguiscale(),
                             16*jguiscale()));
  }

private:
  void openPopup() {
    Border border = Border(2, 2, 2, 3)*jguiscale();
    Rect rc = getBounds();
    rc.y += rc.h;
    rc.w *= 3;
    m_popupWindow = new PopupWindow("", PopupWindow::kCloseOnClickInOtherWindow);
    m_popupWindow->setAutoRemap(false);
    m_popupWindow->setBorder(border);
    m_popupWindow->setBounds(rc + border);

    Region rgn(m_popupWindow->getBounds().createUnion(getBounds()));
    m_popupWindow->setHotRegion(rgn);
    m_brushTypeButton = new ButtonSet(3, 1, m_brushType,
      PART_BRUSH_CIRCLE,
      PART_BRUSH_SQUARE,
      PART_BRUSH_LINE);
    m_brushTypeButton->ItemChange.connect(&BrushTypeField::onBrushTypeChange, this);
    m_brushTypeButton->setTransparent(true);
    m_brushTypeButton->setBgColor(gfx::ColorNone);

    m_popupWindow->addChild(m_brushTypeButton);
    m_popupWindow->openWindow();
  }

  void closePopup() {
    if (m_popupWindow) {
      m_popupWindow->closeWindow(NULL);
      delete m_popupWindow;
      m_popupWindow = NULL;
      m_brushTypeButton = NULL;
    }
  }

  void onBrushTypeChange() {
    m_brushType = (BrushType)m_brushTypeButton->getSelectedItem();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    IBrushSettings* brushSettings = settings->getToolSettings(currentTool)->getBrush();
    brushSettings->setType(m_brushType);

    setBrushSettings(brushSettings);
  }

  she::Surface* m_bitmap;
  BrushType m_brushType;
  PopupWindow* m_popupWindow;
  ButtonSet* m_brushTypeButton;
};

class ContextBar::BrushSizeField : public IntEntry
{
public:
  BrushSizeField() : IntEntry(1, 32) {
    setSuffix("px");
  }

private:
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->getBrush()
      ->setSize(getValue());
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
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->getBrush()
      ->setAngle(getValue());

    IToolSettings* toolSettings = settings->getToolSettings(currentTool);
    IBrushSettings* brushSettings = toolSettings->getBrush();
    m_brushType->setBrushSettings(brushSettings);
  }

private:
  BrushTypeField* m_brushType;
};

class ContextBar::ToleranceField : public IntEntry
{
public:
  ToleranceField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setTolerance(getValue());
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
      case kDefaultInk: index = 0; break;
      case kSetAlphaInk: index = 1; break;
      case kLockAlphaInk: index = 2; break;
    }

    m_lock = true;
    setSelectedItemIndex(index);
    m_lock = false;
  }

protected:
  void onChange() OVERRIDE {
    ComboBox::onChange();

    if (m_lock)
      return;

    InkType inkType = kDefaultInk;

    switch (getSelectedItemIndex()) {
      case 0: inkType = kDefaultInk; break;
      case 1: inkType = kSetAlphaInk; break;
      case 2: inkType = kLockAlphaInk; break;
    }

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)->setInkType(inkType);
  }

  void onCloseListBox() OVERRIDE {
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
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setOpacity(getValue());
  }
};

class ContextBar::SprayWidthField : public IntEntry
{
public:
  SprayWidthField() : IntEntry(1, 32) {
  }

protected:
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setSprayWidth(getValue());
  }
};

class ContextBar::SpraySpeedField : public IntEntry
{
public:
  SpraySpeedField() : IntEntry(1, 100) {
  }

protected:
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setSpraySpeed(getValue());
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
    UIContext::instance()->settings()->selection()->setMoveTransparentColor(getColor());
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
    addItem(new Item("Fast Rotation", kFastRotationAlgorithm));
    addItem(new Item("RotSprite", kRotSpriteRotationAlgorithm));
    m_lockChange = false;

    setSelectedItemIndex((int)UIContext::instance()->settings()
      ->selection()->getRotationAlgorithm());
  }

protected:
  void onChange() OVERRIDE {
    if (m_lockChange)
      return;

    UIContext::instance()->settings()->selection()
      ->setRotationAlgorithm(static_cast<Item*>(getSelectedItem())->algo());
  }

  void onCloseListBox() OVERRIDE {
    releaseFocus();
  }

private:
  class Item : public ListItem {
  public:
    Item(const std::string& text, RotationAlgorithm algo) :
      ListItem(text),
      m_algo(algo) {
    }

    RotationAlgorithm algo() const { return m_algo; }

  private:
    RotationAlgorithm m_algo;
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
  void destroy() OVERRIDE {
    // Do nothing, BrushTypeField is added as a widget in the
    // ContextBar, so it will be destroyed together with the
    // ContextBar.
  }

  int getWidth() OVERRIDE {
    return m_bitmap->width();
  }

  int getHeight() OVERRIDE {
    return m_bitmap->height();
  }

  she::Surface* getNormalIcon() OVERRIDE {
    return m_bitmap;
  }

  she::Surface* getSelectedIcon() OVERRIDE {
    return m_bitmap;
  }

  she::Surface* getDisabledIcon() OVERRIDE {
    return m_bitmap;
  }

  int getIconAlign() OVERRIDE {
    return JI_CENTER | JI_MIDDLE;
  }

protected:
  void onClick(Event& ev) OVERRIDE {
    Button::onClick(ev);

    if (!m_popupWindow || !m_popupWindow->isVisible())
      openPopup();
    else
      closePopup();
  }

  void onPreferredSize(PreferredSizeEvent& ev) {
    ev.setPreferredSize(Size(16*jguiscale(),
                             16*jguiscale()));
  }

private:
  void openPopup() {
    Border border = Border(2, 2, 2, 3)*jguiscale();
    Rect rc = getBounds();
    rc.y += rc.h;
    rc.w *= 3;
    m_popupWindow = new PopupWindow("", PopupWindow::kCloseOnClickInOtherWindow);
    m_popupWindow->setAutoRemap(false);
    m_popupWindow->setBorder(border);
    m_popupWindow->setBounds(rc + border);

    Region rgn(m_popupWindow->getBounds().createUnion(getBounds()));
    m_popupWindow->setHotRegion(rgn);
    m_freehandAlgoButton = new ButtonSet(3, 1, m_freehandAlgo,
      PART_FREEHAND_ALGO_DEFAULT,
      PART_FREEHAND_ALGO_PIXEL_PERFECT,
      PART_FREEHAND_ALGO_DOTS);
    m_freehandAlgoButton->ItemChange.connect(&FreehandAlgorithmField::onFreehandAlgoChange, this);
    m_freehandAlgoButton->setTransparent(true);
    m_freehandAlgoButton->setBgColor(gfx::ColorNone);

    m_tooltipManager->addTooltipFor(m_freehandAlgoButton->getButtonAt(0), "Normal trace", JI_TOP);
    m_tooltipManager->addTooltipFor(m_freehandAlgoButton->getButtonAt(1), "Pixel-perfect trace", JI_TOP);
    m_tooltipManager->addTooltipFor(m_freehandAlgoButton->getButtonAt(2), "Dots", JI_TOP);

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

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setFreehandAlgorithm(m_freehandAlgo);
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

  void setFreehandAlgorithm(FreehandAlgorithm algo) {
    switch (algo) {
      case kDefaultFreehandAlgorithm:
        setSelected(false);
        break;
      case kPixelPerfectFreehandAlgorithm:
        setSelected(true);
        break;
      case kDotsFreehandAlgorithm:
        // Not available
        break;
    }
  }

protected:

  void onClick(Event& ev) OVERRIDE {
    CheckBox::onClick(ev);

    ISettings* settings = UIContext::instance()->settings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setFreehandAlgorithm(isSelected() ?
        kPixelPerfectFreehandAlgorithm:
        kDefaultFreehandAlgorithm);

    releaseFocus();
  }
};

#endif

class ContextBar::SelectionModeField : public ButtonSet
{
public:
  SelectionModeField() : ButtonSet(3, 1, 0,
    PART_SELECTION_REPLACE,
    PART_SELECTION_ADD,
    PART_SELECTION_SUBTRACT) {
    setSelectedItem(
      (int)UIContext::instance()->settings()
      ->selection()->getSelectionMode());
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(getButtonAt(0), "Replace selection", JI_BOTTOM);
    tooltipManager->addTooltipFor(getButtonAt(1), "Add to selection", JI_BOTTOM);
    tooltipManager->addTooltipFor(getButtonAt(2), "Subtract from selection", JI_BOTTOM);
  }

protected:
  void onItemChange() OVERRIDE {
    ButtonSet::onItemChange();

    int item = getSelectedItem();
    UIContext::instance()->settings()->selection()
      ->setSelectionMode((SelectionMode)item);
  }
};

class ContextBar::DropPixelsField : public ButtonSet
{
public:
  DropPixelsField() : ButtonSet(2, 1, -1,
    PART_DROP_PIXELS_OK,
    PART_DROP_PIXELS_CANCEL) {
  }

  void setupTooltips(TooltipManager* tooltipManager) {
    tooltipManager->addTooltipFor(getButtonAt(0), "Drop pixels here", JI_BOTTOM);
    tooltipManager->addTooltipFor(getButtonAt(1), "Cancel drag and drop", JI_BOTTOM);
  }

  Signal1<void, ContextBarObserver::DropAction> DropPixels;

protected:
  void onItemChange() OVERRIDE {
    ButtonSet::onItemChange();

    switch (getSelectedItem()) {
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
  void onClick(Event& ev) OVERRIDE {
    CheckBox::onClick(ev);

    UIContext::instance()->settings()->setGrabAlpha(isSelected());

    releaseFocus();
  }
};

ContextBar::ContextBar()
  : Box(JI_HORIZONTAL)
  , m_toolSettings(NULL)
{
  border_width.b = 2*jguiscale();

  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->getColor(ThemeColor::Workspace));

  addChild(m_selectionOptionsBox = new HBox());
  m_selectionOptionsBox->addChild(m_dropPixels = new DropPixelsField());
  m_selectionOptionsBox->addChild(m_selectionMode = new SelectionModeField);
  m_selectionOptionsBox->addChild(m_transparentColor = new TransparentColorField);
  m_selectionOptionsBox->addChild(m_rotAlgo = new RotAlgorithmField());

  addChild(m_brushType = new BrushTypeField());
  addChild(m_brushSize = new BrushSizeField());
  addChild(m_brushAngle = new BrushAngleField(m_brushType));

  addChild(m_toleranceLabel = new Label("Tolerance:"));
  addChild(m_tolerance = new ToleranceField());

  addChild(m_inkType = new InkTypeField());

  addChild(m_opacityLabel = new Label("Opacity:"));
  addChild(m_inkOpacity = new InkOpacityField());

  addChild(m_grabAlpha = new GrabAlphaField());

  // addChild(new InkChannelTargetField());
  // addChild(new InkShadeField());
  // addChild(new InkSelectionField());

  addChild(m_sprayBox = new HBox());
  m_sprayBox->addChild(setup_mini_font(new Label("Spray:")));
  m_sprayBox->addChild(m_sprayWidth = new SprayWidthField());
  m_sprayBox->addChild(m_spraySpeed = new SpraySpeedField());

  addChild(m_freehandBox = new HBox());
#if 0                           // TODO for v1.1
  Label* freehandLabel;
  m_freehandBox->addChild(freehandLabel = new Label("Freehand:"));
  setup_mini_font(freehandLabel);
#endif
  m_freehandBox->addChild(m_freehandAlgo = new FreehandAlgorithmField());

  setup_mini_font(m_toleranceLabel);
  setup_mini_font(m_opacityLabel);

  TooltipManager* tooltipManager = new TooltipManager();
  addChild(tooltipManager);

  tooltipManager->addTooltipFor(m_brushType, "Brush Type", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_brushSize, "Brush Size (in pixels)", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_brushAngle, "Brush Angle (in degrees)", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_inkOpacity, "Opacity (Alpha value in RGBA)", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_sprayWidth, "Spray Width", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_spraySpeed, "Spray Speed", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_transparentColor, "Transparent Color", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_rotAlgo, "Rotation Algorithm", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_freehandAlgo, "Freehand trace algorithm", JI_BOTTOM);
  tooltipManager->addTooltipFor(m_grabAlpha,
    "When checked the tool picks the color from the active layer, and its alpha\n"
    "component is used to setup the opacity level of all drawing tools.\n\n"
    "When unchecked -the default behavior- the color is picked\n"
    "from the composition of all sprite layers.", JI_LEFT | JI_TOP);
  m_selectionMode->setupTooltips(tooltipManager);
  m_dropPixels->setupTooltips(tooltipManager);
  m_freehandAlgo->setupTooltips(tooltipManager);

  App::instance()->BrushSizeAfterChange.connect(&ContextBar::onBrushSizeChange, this);
  App::instance()->BrushAngleAfterChange.connect(&ContextBar::onBrushAngleChange, this);
  App::instance()->CurrentToolChange.connect(&ContextBar::onCurrentToolChange, this);
  m_dropPixels->DropPixels.connect(&ContextBar::onDropPixels, this);

  onCurrentToolChange();
}

ContextBar::~ContextBar()
{
  if (m_toolSettings)
    m_toolSettings->removeObserver(this);
}

bool ContextBar::onProcessMessage(Message* msg)
{
  return Box::onProcessMessage(msg);
}

void ContextBar::onPreferredSize(PreferredSizeEvent& ev)
{
  ev.setPreferredSize(gfx::Size(0, 18*jguiscale())); // TODO calculate height
}

void ContextBar::onSetOpacity(int newOpacity)
{
  m_inkOpacity->setTextf("%d", newOpacity);
}

void ContextBar::onBrushSizeChange()
{
  ISettings* settings = UIContext::instance()->settings();
  Tool* currentTool = settings->getCurrentTool();
  IToolSettings* toolSettings = settings->getToolSettings(currentTool);
  IBrushSettings* brushSettings = toolSettings->getBrush();

  m_brushType->setBrushSettings(brushSettings);
  m_brushSize->setTextf("%d", brushSettings->getSize());
}

void ContextBar::onBrushAngleChange()
{
  ISettings* settings = UIContext::instance()->settings();
  Tool* currentTool = settings->getCurrentTool();
  IToolSettings* toolSettings = settings->getToolSettings(currentTool);
  IBrushSettings* brushSettings = toolSettings->getBrush();

  m_brushType->setBrushSettings(brushSettings);
  m_brushAngle->setTextf("%d", brushSettings->getAngle());
}

void ContextBar::onCurrentToolChange()
{
  ISettings* settings = UIContext::instance()->settings();
  updateFromTool(settings->getCurrentTool());
}

void ContextBar::onDropPixels(ContextBarObserver::DropAction action)
{
  notifyObservers(&ContextBarObserver::onDropPixels, action);
}

void ContextBar::updateFromTool(tools::Tool* tool)
{
  ISettings* settings = UIContext::instance()->settings();
  IToolSettings* toolSettings = settings->getToolSettings(tool);
  IBrushSettings* brushSettings = toolSettings->getBrush();

  if (m_toolSettings)
    m_toolSettings->removeObserver(this);
  m_toolSettings = toolSettings;
  m_toolSettings->addObserver(this);

  m_brushType->setBrushSettings(brushSettings);
  m_brushSize->setTextf("%d", brushSettings->getSize());
  m_brushAngle->setTextf("%d", brushSettings->getAngle());

  m_tolerance->setTextf("%d", toolSettings->getTolerance());

  m_inkType->setInkType(toolSettings->getInkType());
  m_inkOpacity->setTextf("%d", toolSettings->getOpacity());

  m_grabAlpha->setSelected(settings->getGrabAlpha());
  m_freehandAlgo->setFreehandAlgorithm(toolSettings->getFreehandAlgorithm());

  m_sprayWidth->setValue(toolSettings->getSprayWidth());
  m_spraySpeed->setValue(toolSettings->getSpraySpeed());

  // True if the current tool needs opacity options
  bool hasOpacity = (tool->getInk(0)->isPaint() ||
                     tool->getInk(0)->isEffect() ||
                     tool->getInk(1)->isPaint() ||
                     tool->getInk(1)->isEffect());

  // True if the current tool is eyedropper.
  bool isEyedropper =
    (tool->getInk(0)->isEyedropper() ||
     tool->getInk(1)->isEyedropper());

  // True if it makes sense to change the ink property for the current
  // tool.
  bool hasInk = hasOpacity;

  // True if the current tool needs tolerance options
  bool hasTolerance = (tool->getPointShape(0)->isFloodFill() ||
                       tool->getPointShape(1)->isFloodFill());

  // True if the current tool needs spray options
  bool hasSprayOptions = (tool->getPointShape(0)->isSpray() ||
                          tool->getPointShape(1)->isSpray());

  bool hasSelectOptions = (tool->getInk(0)->isSelection() ||
                           tool->getInk(1)->isSelection());

  bool isFreehand =
    (tool->getController(0)->isFreehand() ||
     tool->getController(1)->isFreehand());

  // Show/Hide fields
  m_brushType->setVisible(hasOpacity);
  m_brushSize->setVisible(hasOpacity);
  m_brushAngle->setVisible(hasOpacity);
  m_opacityLabel->setVisible(hasOpacity);
  m_inkType->setVisible(hasInk);
  m_inkOpacity->setVisible(hasOpacity);
  m_grabAlpha->setVisible(isEyedropper);
  m_freehandBox->setVisible(isFreehand && hasOpacity);
  m_toleranceLabel->setVisible(hasTolerance);
  m_tolerance->setVisible(hasTolerance);
  m_sprayBox->setVisible(hasSprayOptions);
  m_selectionOptionsBox->setVisible(hasSelectOptions);
  m_selectionMode->setVisible(true);
  m_dropPixels->setVisible(false);

  layout();
}

void ContextBar::updateForMovingPixels()
{
  tools::Tool* tool = App::instance()->getToolBox()->getToolById(
    tools::WellKnownTools::RectangularMarquee);
  if (tool)
    updateFromTool(tool);

  m_dropPixels->deselectItems();
  m_dropPixels->setVisible(true);
  m_selectionMode->setVisible(false);
  layout();
}

} // namespace app
