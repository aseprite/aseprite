/* ASEPRITE
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

#include "config.h"

#include "app.h"
#include "base/unique_ptr.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/pen.h"
#include "settings/settings.h"
#include "skin/skin_theme.h"
#include "tools/ink.h"
#include "tools/tool.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/label.h"
#include "ui/popup_window.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"
#include "ui_context.h"
#include "widgets/button_set.h"
#include "widgets/context_bar.h"

#include <allegro.h>

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
    UniquePtr<Pen> pen(new Pen(m_penType = penSettings->getType(),
                               std::min(10, penSettings->getSize()),
                               penSettings->getAngle()));
    Image* image = pen->get_image();

    if (m_bitmap)
      destroy_bitmap(m_bitmap);
    m_bitmap = create_bitmap_ex(32, image->w, image->h);
    clear(m_bitmap);
    image_to_allegro(image, m_bitmap, 0, 0, NULL);

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
    m_popupWindow = new PopupWindow(NULL, false);
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
    setSuffix("\xB0");
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
  ToleranceField() : IntEntry(0, 100) {
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
  enum { kMerge, kCopy, kShade, kSelection };

public:
  InkTypeField() {
    addItem("Merge");
    addItem("Copy");
    addItem("Shade");
    addItem("Selection");

    setSelectedItem(0);
  }

  void updateSelectedInk(Ink* ink) {
    if (ink->isPaint()) setSelectedItem(kMerge);
    else if (ink->isSelection()) setSelectedItem(kSelection);
  }

protected:
  void onChange() OVERRIDE {
    ComboBox::onChange();
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

ContextBar::ContextBar()
  : Box(JI_HORIZONTAL)
{
  border_width.b = 2*jguiscale();

  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->getColor(ThemeColor::Workspace));

  addChild(new Label("Brush:"));
  addChild(m_brushType = new BrushTypeField());
  addChild(m_brushSize = new BrushSizeField());
  addChild(m_brushAngle = new BrushAngleField(m_brushType));

  addChild(new Label("Tolerance:"));
  addChild(m_tolerance = new ToleranceField());

  addChild(new Label("Ink:"));
  addChild(m_inkType = new InkTypeField());
  addChild(m_inkOpacity = new InkOpacityField());
  // addChild(new InkChannelTargetField());
  // addChild(new InkShadeField());
  // addChild(new InkSelectionField());

  App::instance()->PenSizeAfterChange.connect(&ContextBar::onPenSizeAfterChange, this);
  App::instance()->CurrentToolChange.connect(&ContextBar::onCurrentToolChange, this);

  onCurrentToolChange();
}

ContextBar::~ContextBar()
{
}

bool ContextBar::onProcessMessage(Message* msg)
{
  return Box::onProcessMessage(msg);
}

void ContextBar::onPenSizeAfterChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* currentTool = settings->getCurrentTool();
  IToolSettings* toolSettings = settings->getToolSettings(currentTool);
  IPenSettings* penSettings = toolSettings->getPen();

  m_brushType->setPenSettings(penSettings);
  m_brushSize->setTextf("%d", penSettings->getSize());
}

void ContextBar::onCurrentToolChange()
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* currentTool = settings->getCurrentTool();
  IToolSettings* toolSettings = settings->getToolSettings(currentTool);
  IPenSettings* penSettings = toolSettings->getPen();

  m_brushType->setPenSettings(penSettings);
  m_brushSize->setTextf("%d", penSettings->getSize());
  m_brushAngle->setTextf("%d", penSettings->getAngle());

  m_tolerance->setTextf("%d", toolSettings->getTolerance());

  Ink* ink = currentTool->getInk(0);
  m_inkType->updateSelectedInk(ink);
  m_inkOpacity->setTextf("%d", toolSettings->getOpacity());
}
