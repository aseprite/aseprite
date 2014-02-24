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
#include "raster/conversion_alleg.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/pen.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/listitem.h"
#include "ui/popup_window.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"
#include "ui/tooltips.h"

#include <allegro.h>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace tools;

class ContextBar::BrushTypeField : public Button
                                 , public IButtonIcon
{
public:
  BrushTypeField()
    : Button("")
    , m_popupWindow(NULL)
    , m_brushType(NULL) {
    setup_mini_look(this);
    setIconInterface(this);

    m_bitmap = create_bitmap_ex(32, 8, 8);
    clear(m_bitmap);
  }

  ~BrushTypeField() {
    closePopup();
    setIconInterface(NULL);
    destroy_bitmap(m_bitmap);
  }

  void setPenSettings(IPenSettings* penSettings) {
    base::UniquePtr<Palette> palette(new Palette(FrameNumber(0), 2));
    palette->setEntry(0, raster::rgba(0, 0, 0, 0));
    palette->setEntry(1, raster::rgba(0, 0, 0, 255));

    base::UniquePtr<Pen> pen(new Pen(m_penType = penSettings->getType(),
                                     std::min(10, penSettings->getSize()),
                                     penSettings->getAngle()));
    Image* image = pen->get_image();

    if (m_bitmap)
      destroy_bitmap(m_bitmap);
    m_bitmap = create_bitmap_ex(32, image->getWidth(), image->getHeight());
    clear(m_bitmap);
    convert_image_to_allegro(image, m_bitmap, 0, 0, palette);

    invalidate();
  }

  // IButtonIcon implementation
  void destroy() OVERRIDE {
    // Do nothing, BrushTypeField is added as a widget in the
    // ContextBar, so it will be destroyed together with the
    // ContextBar.
  }

  int getWidth() OVERRIDE {
    return m_bitmap->w;
  }

  int getHeight() OVERRIDE {
    return m_bitmap->h;
  }

  BITMAP* getNormalIcon() OVERRIDE {
    return m_bitmap;
  }

  BITMAP* getSelectedIcon() OVERRIDE {
    return m_bitmap;
  }

  BITMAP* getDisabledIcon() OVERRIDE {
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
    Rect rc = getBounds();
    rc.y += rc.h;
    rc.w *= 3;
    m_popupWindow = new PopupWindow("", false);
    m_popupWindow->setAutoRemap(false);
    m_popupWindow->setBounds(rc);

    Region rgn(rc.createUnion(getBounds()));
    m_popupWindow->setHotRegion(rgn);
    m_brushType = new ButtonSet(3, 1, m_penType,
                                PART_BRUSH_CIRCLE,
                                PART_BRUSH_SQUARE,
                                PART_BRUSH_LINE);

    m_brushType->ItemChange.connect(&BrushTypeField::onBrushTypeChange, this);

    m_popupWindow->addChild(m_brushType);
    m_popupWindow->openWindow();
  }

  void closePopup() {
    if (m_popupWindow) {
      m_popupWindow->closeWindow(NULL);
      delete m_popupWindow;
      m_popupWindow = NULL;
      m_brushType = NULL;
    }
  }

  void onBrushTypeChange() {
    m_penType = (PenType)m_brushType->getSelectedItem();

    ISettings* settings = UIContext::instance()->getSettings();
    Tool* currentTool = settings->getCurrentTool();
    IPenSettings* penSettings = settings->getToolSettings(currentTool)->getPen();
    penSettings->setType(m_penType);

    setPenSettings(penSettings);
  }

  BITMAP* m_bitmap;
  PenType m_penType;
  PopupWindow* m_popupWindow;
  ButtonSet* m_brushType;
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

    ISettings* settings = UIContext::instance()->getSettings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->getPen()
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

    ISettings* settings = UIContext::instance()->getSettings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->getPen()
      ->setAngle(getValue());

    IToolSettings* toolSettings = settings->getToolSettings(currentTool);
    IPenSettings* penSettings = toolSettings->getPen();
    m_brushType->setPenSettings(penSettings);
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

    ISettings* settings = UIContext::instance()->getSettings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setTolerance(getValue());
  }
};

class ContextBar::InkTypeField : public ComboBox
{
public:
  InkTypeField() {
    // The same order as in InkType
    addItem("Default Ink");
#if 0
    addItem("Opaque");
#endif
    addItem("Put Alpha");
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
    setSelectedItemIndex((int)inkType);
  }

protected:
  void onChange() OVERRIDE {
    ComboBox::onChange();

    ISettings* settings = UIContext::instance()->getSettings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setInkType((InkType)getSelectedItemIndex());
  }
};

class ContextBar::InkOpacityField : public IntEntry
{
public:
  InkOpacityField() : IntEntry(0, 255) {
  }

protected:
  void onValueChange() OVERRIDE {
    IntEntry::onValueChange();

    ISettings* settings = UIContext::instance()->getSettings();
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

    ISettings* settings = UIContext::instance()->getSettings();
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

    ISettings* settings = UIContext::instance()->getSettings();
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
    addItem(new Item("Fast Rotation", kFastRotationAlgorithm));
    addItem(new Item("RotSprite", kRotSpriteRotationAlgorithm));

    setSelectedItemIndex((int)
      UIContext::instance()->settings()->selection()
      ->getRotationAlgorithm());
  }

protected:
  void onChange() OVERRIDE {
    UIContext::instance()->settings()->selection()
      ->setRotationAlgorithm(static_cast<Item*>(getSelectedItem())->algo());
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
};

class ContextBar::FreehandAlgorithmField : public CheckBox
{
public:
  FreehandAlgorithmField() : CheckBox("Pixel-perfect") {
    setup_mini_font(this);
  }

  void onClick(Event& ev) OVERRIDE {
    CheckBox::onClick(ev);

    ISettings* settings = UIContext::instance()->getSettings();
    Tool* currentTool = settings->getCurrentTool();
    settings->getToolSettings(currentTool)
      ->setFreehandAlgorithm(isSelected() ?
        kPixelPerfectFreehandAlgorithm:
        kDefaultFreehandAlgorithm);
  }
};

class ContextBar::SelectionModeField : public ButtonSet
{
public:
  SelectionModeField() : ButtonSet(3, 1, 0,
    PART_SELECTION_REPLACE,
    PART_SELECTION_ADD,
    PART_SELECTION_SUBTRACT) {
  }

  void setupTooltips(TooltipManager* tooltipManager)
  {
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

  App::instance()->PenSizeAfterChange.connect(&ContextBar::onPenSizeChange, this);
  App::instance()->PenAngleAfterChange.connect(&ContextBar::onPenAngleChange, this);
  App::instance()->CurrentToolChange.connect(&ContextBar::onCurrentToolChange, this);

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

void ContextBar::onPenSizeChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* currentTool = settings->getCurrentTool();
  IToolSettings* toolSettings = settings->getToolSettings(currentTool);
  IPenSettings* penSettings = toolSettings->getPen();

  m_brushType->setPenSettings(penSettings);
  m_brushSize->setTextf("%d", penSettings->getSize());
}

void ContextBar::onPenAngleChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* currentTool = settings->getCurrentTool();
  IToolSettings* toolSettings = settings->getToolSettings(currentTool);
  IPenSettings* penSettings = toolSettings->getPen();

  m_brushType->setPenSettings(penSettings);
  m_brushAngle->setTextf("%d", penSettings->getAngle());
}

void ContextBar::onCurrentToolChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  updateFromTool(settings->getCurrentTool());
}

void ContextBar::updateFromTool(tools::Tool* tool)
{
  ISettings* settings = UIContext::instance()->getSettings();
  IToolSettings* toolSettings = settings->getToolSettings(tool);
  IPenSettings* penSettings = toolSettings->getPen();

  if (m_toolSettings)
    m_toolSettings->removeObserver(this);
  m_toolSettings = toolSettings;
  m_toolSettings->addObserver(this);

  m_brushType->setPenSettings(penSettings);
  m_brushSize->setTextf("%d", penSettings->getSize());
  m_brushAngle->setTextf("%d", penSettings->getAngle());

  m_tolerance->setTextf("%d", toolSettings->getTolerance());

  m_inkType->setInkType(toolSettings->getInkType());
  m_inkOpacity->setTextf("%d", toolSettings->getOpacity());

  m_grabAlpha->setSelected(settings->getGrabAlpha());
  m_freehandAlgo->setSelected(toolSettings->getFreehandAlgorithm() == kPixelPerfectFreehandAlgorithm);

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

  layout();
}

} // namespace app
