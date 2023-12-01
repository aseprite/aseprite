// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/dynamics_popup.h"

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/tools/tool_box.h"
#include "app/ui/dithering_selector.h"
#include "app/ui/skin/skin_theme.h"
#include "os/font.h"
#include "os/surface.h"
#include "ui/fit_bounds.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/widget.h"

#include "dynamics.xml.h"

#include <algorithm>

namespace app {

using namespace ui;
using namespace skin;

namespace {

enum {
  NONE,
  PRESSURE_HEADER,
  VELOCITY_HEADER,
  SIZE_HEADER,
  SIZE_WITH_PRESSURE,
  SIZE_WITH_VELOCITY,
  ANGLE_HEADER,
  ANGLE_WITH_PRESSURE,
  ANGLE_WITH_VELOCITY,
  GRADIENT_HEADER,
  GRADIENT_WITH_PRESSURE,
  GRADIENT_WITH_VELOCITY,
};

} // anonymous namespace

// Special slider to set the min/max threshold values of a sensor
class DynamicsPopup::ThresholdSlider : public Widget  {
public:
  ThresholdSlider() {
    setExpansive(true);
    initTheme();
  }

  float minThreshold() const { return m_minThreshold; }
  float maxThreshold() const { return m_maxThreshold; }
  void minThreshold(float min) { m_minThreshold = min; }
  void maxThreshold(float max) { m_maxThreshold = max; }
  void setSensorValue(float v) {
    m_sensorValue = v;
    invalidate();
  }

private:
  void onInitTheme(InitThemeEvent& ev) override {
    Widget::onInitTheme(ev);
    auto theme = SkinTheme::get(this);
    setBorder(
      gfx::Border(
        theme->parts.miniSliderEmpty()->bitmapW()->width(),
        theme->parts.miniSliderEmpty()->bitmapN()->height(),
        theme->parts.miniSliderEmpty()->bitmapE()->width(),
        theme->parts.miniSliderEmpty()->bitmapS()->height()));
  }

  void onSizeHint(SizeHintEvent& ev) override {
    ev.setSizeHint(
      border().width(),
      textHeight()+2*guiscale() + border().height());
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    auto theme = SkinTheme::get(this);
    gfx::Rect rc = clientBounds();
    gfx::Color bgcolor = bgColor();
    g->fillRect(bgcolor, rc);

    rc.shrink(border());
    rc = clientBounds();

    // Draw customized background
    const skin::SkinPartPtr& nw = theme->parts.miniSliderEmpty();
    os::Surface* thumb =
      (hasFocus() ? theme->parts.miniSliderThumbFocused()->bitmap(0):
                    theme->parts.miniSliderThumb()->bitmap(0));

    // Draw background
    g->fillRect(bgcolor, rc);

    // Draw thumb
    int thumb_y = rc.y;
    rc.shrink(gfx::Border(0, thumb->height(), 0, 0));

    // Draw borders
    if (rc.h > 4*guiscale()) {
      rc.shrink(gfx::Border(3, 0, 3, 1) * guiscale());
      theme->drawRect(g, rc, nw.get());
    }

    const int minX = this->minX();
    const int maxX = this->maxX();
    const int sensorW = float(rc.w)*m_sensorValue;

    // Draw background
    if (m_minThreshold > 0.0f) {
      theme->drawRect(
        g, gfx::Rect(rc.x, rc.y, minX-rc.x, rc.h),
        theme->parts.miniSliderFull().get());
    }
    if (m_maxThreshold < 1.0f) {
      theme->drawRect(
        g, gfx::Rect(maxX, rc.y, rc.x2()-maxX, rc.h),
        theme->parts.miniSliderFull().get());
    }

    g->fillRect(theme->colors.sliderEmptyText(),
                gfx::Rect(rc.x, rc.y+rc.h/2-rc.h/8, sensorW, rc.h/4));

    g->drawRgbaSurface(thumb, minX-thumb->width()/2, thumb_y);
    g->drawRgbaSurface(thumb, maxX-thumb->width()/2, thumb_y);
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kMouseDownMessage: {
        auto mouseMsg = static_cast<MouseMessage*>(msg);
        const int u = mouseMsg->position().x - (origin().x+border().left()+3*guiscale());
        const int minX = this->minX();
        const int maxX = this->maxX();
        if (ABS(u-minX) <
            ABS(u-maxX))
          capture = Capture::Min;
        else
          capture = Capture::Max;
        captureMouse();
        break;
      }

      case kMouseUpMessage:
        if (hasCapture())
          releaseMouse();
        break;

      case kMouseMoveMessage: {
        if (!hasCapture())
          break;

        auto mouseMsg = static_cast<MouseMessage*>(msg);
        const gfx::Rect rc = bounds();
        float u = (mouseMsg->position().x - rc.x) / float(rc.w);
        u = std::clamp(u, 0.0f, 1.0f);
        switch (capture) {
          case Capture::Min:
            m_minThreshold = u;
            if (m_maxThreshold < u)
              m_maxThreshold = u;
            invalidate();
            break;
          case Capture::Max:
            m_maxThreshold = u;
            if (m_minThreshold > u)
              m_minThreshold = u;
            invalidate();
            break;
        }
        break;
      }
    }
    return Widget::onProcessMessage(msg);
  }

  int minX() const {
    gfx::Rect rc = clientBounds();
    rc.shrink(border());
    rc.shrink(gfx::Border(3, 0, 3, 1) * guiscale());
    return rc.x + float(rc.w)*m_minThreshold;
  }

  int maxX() const {
    gfx::Rect rc = clientBounds();
    rc.shrink(border());
    rc.shrink(gfx::Border(3, 0, 3, 1) * guiscale());
    return rc.x + float(rc.w)*m_maxThreshold;
  }

  enum Capture { Min, Max };

  float m_minThreshold = 0.1f;
  float m_sensorValue = 0.0f;
  float m_maxThreshold = 0.9f;
  Capture capture;
};

DynamicsPopup::DynamicsPopup(Delegate* delegate)
  : PopupWindow("",
                PopupWindow::ClickBehavior::CloseOnClickOutsideHotRegion,
                PopupWindow::EnterBehavior::DoNothingOnEnter)
  , m_delegate(delegate)
  , m_dynamics(new gen::Dynamics)
  , m_stabilizerFactorBackup(0)
  , m_ditheringSel(new DitheringSelector(DitheringSelector::SelectMatrix))
  , m_fromTo(tools::ColorFromTo::BgToFg)
{
  m_dynamics->stabilizer()->Click.connect(
    [this](){
      if (m_dynamics->stabilizer()->isSelected()) {
        if (m_stabilizerFactorBackup == 0) {
          // TODO default value when we enable stabilizer when it's
          // zero
          m_dynamics->stabilizerFactor()->setValue(16);
          m_stabilizerFactorBackup = 16;
        }
        else {
          m_dynamics->stabilizerFactor()->setValue(
            m_stabilizerFactorBackup);
        }
      }
      else {
        m_stabilizerFactorBackup =
          m_dynamics->stabilizerFactor()->getValue();
        m_dynamics->stabilizerFactor()->setValue(0);
      }

    });
  m_dynamics->stabilizerFactor()->Change.connect(
    [this](){
      m_stabilizerFactorBackup =
        m_dynamics->stabilizerFactor()->getValue();
      m_dynamics->stabilizer()->setSelected(
        m_stabilizerFactorBackup > 0);
    });

  m_dynamics->values()->ItemChange.connect(
    [this](ButtonSet::Item* item){
      onValuesChange(item);
    });
  m_dynamics->maxSize()->Change.connect(
    [this]{
      m_delegate->setMaxSize(m_dynamics->maxSize()->getValue());
    });
  m_dynamics->maxAngle()->Change.connect(
    [this]{
      m_delegate->setMaxAngle(m_dynamics->maxAngle()->getValue());
    });
  m_dynamics->gradientFromTo()->Click.connect(
    [this]{
      if (m_fromTo == tools::ColorFromTo::BgToFg)
        m_fromTo = tools::ColorFromTo::FgToBg;
      else
        m_fromTo = tools::ColorFromTo::BgToFg;
      updateFromToText();
    });
  m_ditheringSel->OpenListBox.connect(
    [this]{
      if (auto comboboxWindow = m_ditheringSel->getWindowWidget()) {
        m_hotRegion |= gfx::Region(comboboxWindow->boundsOnScreen());
        setHotRegion(m_hotRegion);
      }
    });
  m_dynamics->sameInAllTools()->setSelected(
    Preferences::instance().shared.shareDynamics());
  m_dynamics->sameInAllTools()->Click.connect(
    [this]{
      // if sameInAllTools is true here, this means:
      // "Transition false to true of 'Same in all tools' and the
      //  current parameters in the DynamicsPopup windows are the
      //  old one, i.e non-shared parameters."
      bool sameInAllTools = m_dynamics->sameInAllTools()->isSelected();
      // Save the old dynamics options:
      saveDynamicsPref(!sameInAllTools);
      Preferences::instance().shared.shareDynamics(sameInAllTools);
    });
  m_dynamics->gradientPlaceholder()->addChild(m_ditheringSel);
  m_dynamics->pressurePlaceholder()->addChild(m_pressureThreshold = new ThresholdSlider);
  m_dynamics->velocityPlaceholder()->addChild(m_velocityThreshold = new ThresholdSlider);
  addChild(m_dynamics);
}

void DynamicsPopup::setOptionsGridVisibility(bool state)
{
  m_dynamics->grid()->setVisible(state);
  if (isVisible())
    expandWindow(sizeHint());
}

std::string DynamicsPopup::ditheringMatrixName() const
{
  if (m_ditheringSel)
    return m_ditheringSel->getItemText(
      m_ditheringSel->getSelectedItemIndex());
  return std::string();
}

void DynamicsPopup::loadDynamicsPref(bool sameInAllTools)
{
  m_dynamics->sameInAllTools()->setSelected(sameInAllTools);
  tools::Tool* tool = nullptr;
  if (!sameInAllTools)
    tool = App::instance()->activeTool();

  auto& dynaPref = Preferences::instance().tool(tool).dynamics;
  m_dynamics->stabilizer()->setSelected(dynaPref.stabilizer());
  m_stabilizerFactorBackup = dynaPref.stabilizerFactor();
  m_dynamics->stabilizerFactor()->setValue(
    dynaPref.stabilizer() ? m_stabilizerFactorBackup : 0);
  m_dynamics->minSize()->setValue(dynaPref.minSize());
  m_dynamics->minAngle()->setValue(dynaPref.minAngle());
  m_pressureThreshold->minThreshold(dynaPref.minPressureThreshold());
  m_pressureThreshold->maxThreshold(dynaPref.maxPressureThreshold());
  m_velocityThreshold->minThreshold(dynaPref.minVelocityThreshold());
  m_velocityThreshold->maxThreshold(dynaPref.maxVelocityThreshold());
  m_fromTo = dynaPref.colorFromTo();

  setCheck(SIZE_WITH_PRESSURE,
           dynaPref.size() == tools::DynamicSensor::Pressure);
  setCheck(SIZE_WITH_VELOCITY,
           dynaPref.size() == tools::DynamicSensor::Velocity);
  setCheck(ANGLE_WITH_PRESSURE,
           dynaPref.angle() == tools::DynamicSensor::Pressure);
  setCheck(ANGLE_WITH_VELOCITY,
           dynaPref.angle() == tools::DynamicSensor::Velocity);
  setCheck(GRADIENT_WITH_PRESSURE,
           dynaPref.gradient() == tools::DynamicSensor::Pressure);
  setCheck(GRADIENT_WITH_VELOCITY,
           dynaPref.gradient() == tools::DynamicSensor::Velocity);

  if (m_ditheringSel)
    m_ditheringSel->setSelectedItemByName(dynaPref.matrixName());
}

void DynamicsPopup::saveDynamicsPref(bool sameInAllTools)
{
  tools::DynamicsOptions dyna = getDynamics();
  tools::Tool* tool = nullptr;
  if (!sameInAllTools)
    tool = App::instance()->activeTool();

  auto dynaPref = &Preferences::instance().tool(tool).dynamics;
  dynaPref->stabilizer(dyna.stabilizer);
  dynaPref->stabilizerFactor(m_stabilizerFactorBackup);
  dynaPref->minSize(dyna.minSize);
  dynaPref->minAngle(dyna.minAngle);
  dynaPref->minPressureThreshold(dyna.minPressureThreshold);
  dynaPref->maxPressureThreshold(dyna.maxPressureThreshold);
  dynaPref->minVelocityThreshold(dyna.minVelocityThreshold);
  dynaPref->maxVelocityThreshold(dyna.maxVelocityThreshold);
  dynaPref->colorFromTo(m_fromTo);

  dynaPref->size(dyna.size);
  dynaPref->angle(dyna.angle);
  dynaPref->gradient(dyna.gradient);

  if (m_ditheringSel)
    dynaPref->matrixName(m_ditheringSel->getItemText(
      m_ditheringSel->getSelectedItemIndex()));
}

tools::DynamicsOptions DynamicsPopup::getDynamics() const
{
  tools::DynamicsOptions opts;

  opts.stabilizer = m_dynamics->stabilizer()->isSelected();
  opts.stabilizerFactor = m_stabilizerFactorBackup;

  opts.size =
    (isCheck(SIZE_WITH_PRESSURE) ? tools::DynamicSensor::Pressure:
     isCheck(SIZE_WITH_VELOCITY) ? tools::DynamicSensor::Velocity:
                                   tools::DynamicSensor::Static);
  opts.angle =
    (isCheck(ANGLE_WITH_PRESSURE) ? tools::DynamicSensor::Pressure:
     isCheck(ANGLE_WITH_VELOCITY) ? tools::DynamicSensor::Velocity:
                                    tools::DynamicSensor::Static);
  opts.gradient =
    (isCheck(GRADIENT_WITH_PRESSURE) ? tools::DynamicSensor::Pressure:
     isCheck(GRADIENT_WITH_VELOCITY) ? tools::DynamicSensor::Velocity:
                                       tools::DynamicSensor::Static);
  opts.minSize = m_dynamics->minSize()->getValue();
  opts.minAngle = m_dynamics->minAngle()->getValue();
  opts.ditheringMatrix = m_ditheringSel->ditheringMatrix();
  opts.colorFromTo = m_fromTo;

  opts.minPressureThreshold = m_pressureThreshold->minThreshold();
  opts.maxPressureThreshold = m_pressureThreshold->maxThreshold();
  opts.minVelocityThreshold = m_velocityThreshold->minThreshold();
  opts.maxVelocityThreshold = m_velocityThreshold->maxThreshold();

  return opts;
}

void DynamicsPopup::setCheck(int i, bool state)
{
  auto theme = SkinTheme::get(this);
  m_dynamics->values()
    ->getItem(i)
    ->setIcon(state ? theme->parts.dropPixelsOk(): nullptr);
}

bool DynamicsPopup::isCheck(int i) const
{
  auto theme = SkinTheme::get(this);
  return (m_dynamics->values()
          ->getItem(i)
          ->icon() == theme->parts.dropPixelsOk());
}

// Update Pressure/Velocity/Gradient popup's variables visibility
// according ButtonSet checks.
// Used after a value change (onValuesChange) and when the popup is
// displayed (on ContextBar::DynamicsField::switchPopup()).
void DynamicsPopup::refreshVisibility()
{
  const bool hasPressure = (isCheck(SIZE_WITH_PRESSURE) ||
                            isCheck(ANGLE_WITH_PRESSURE) ||
                            isCheck(GRADIENT_WITH_PRESSURE));
  const bool hasVelocity = (isCheck(SIZE_WITH_VELOCITY) ||
                            isCheck(ANGLE_WITH_VELOCITY) ||
                            isCheck(GRADIENT_WITH_VELOCITY));
  const bool needsSize = (isCheck(SIZE_WITH_PRESSURE) ||
                          isCheck(SIZE_WITH_VELOCITY));
  const bool needsAngle = (isCheck(ANGLE_WITH_PRESSURE) ||
                           isCheck(ANGLE_WITH_VELOCITY));
  const bool needsGradient = (isCheck(GRADIENT_WITH_PRESSURE) ||
                              isCheck(GRADIENT_WITH_VELOCITY));
  const bool any = (needsSize || needsAngle || needsGradient);
  doc::BrushRef brush = m_delegate->getActiveBrush();

  if (needsSize) {
    int maxSize = brush->size();
    if (maxSize == 1) {
      // If brush size == 1, we put it to 4 so the user has some size
      // change by default.
      maxSize = 4;
      m_delegate->setMaxSize(maxSize);
    }
    m_dynamics->maxSize()->setValue(maxSize);
  }
  m_dynamics->sizeLabel()->setVisible(needsSize);
  m_dynamics->minSize()->setVisible(needsSize);
  m_dynamics->maxSize()->setVisible(needsSize);

  if (needsAngle) {
    m_dynamics->maxAngle()->setValue(brush->angle());
  }
  m_dynamics->angleLabel()->setVisible(needsAngle);
  m_dynamics->minAngle()->setVisible(needsAngle);
  m_dynamics->maxAngle()->setVisible(needsAngle);

  m_dynamics->gradientLabel()->setVisible(needsGradient);
  m_dynamics->gradientPlaceholder()->setVisible(needsGradient);
  m_dynamics->gradientFromTo()->setVisible(needsGradient);
  updateFromToText();

  m_dynamics->separator()->setVisible(any);
  m_dynamics->options()->setVisible(any);
  m_dynamics->separator2()->setVisible(any);
  m_dynamics->pressureLabel()->setVisible(hasPressure);
  m_dynamics->pressurePlaceholder()->setVisible(hasPressure);
  m_dynamics->velocityLabel()->setVisible(hasVelocity);
  m_dynamics->velocityPlaceholder()->setVisible(hasVelocity);

  expandWindow(sizeHint());

  m_hotRegion |= gfx::Region(boundsOnScreen());
  setHotRegion(m_hotRegion);

  // Inform to the delegate that the dynamics have changed (so the
  // delegate can update the UI to show if the dynamics are on/off).
  m_delegate->onDynamicsChange(getDynamics());
}

bool DynamicsPopup::sharedSettings() const
{
  return m_dynamics->sameInAllTools()->isSelected();
}

void DynamicsPopup::onValuesChange(ButtonSet::Item* item)
{
  auto theme = SkinTheme::get(this);
  const skin::SkinPartPtr& ok = theme->parts.dropPixelsOk();
  const int i = (item ? m_dynamics->values()->getItemIndex(item): -1);

  // Switch item off
  if (item && item->icon().get() == ok.get()) {
    item->setIcon(nullptr);
  }
  else {
    switch (i) {
      case SIZE_WITH_PRESSURE:
      case SIZE_WITH_VELOCITY:
        setCheck(SIZE_WITH_PRESSURE, i == SIZE_WITH_PRESSURE);
        setCheck(SIZE_WITH_VELOCITY, i == SIZE_WITH_VELOCITY);
        break;
      case ANGLE_WITH_PRESSURE:
      case ANGLE_WITH_VELOCITY:
        setCheck(ANGLE_WITH_PRESSURE, i == ANGLE_WITH_PRESSURE);
        setCheck(ANGLE_WITH_VELOCITY, i == ANGLE_WITH_VELOCITY);
        break;
      case GRADIENT_WITH_PRESSURE:
      case GRADIENT_WITH_VELOCITY:
        setCheck(GRADIENT_WITH_PRESSURE, i == GRADIENT_WITH_PRESSURE);
        setCheck(GRADIENT_WITH_VELOCITY, i == GRADIENT_WITH_VELOCITY);
        break;
    }
  }

  refreshVisibility();
}

void DynamicsPopup::updateFromToText()
{
  m_dynamics->gradientFromTo()->setText(
    m_fromTo == tools::ColorFromTo::BgToFg ? "BG > FG":
    m_fromTo == tools::ColorFromTo::FgToBg ? "FG > BG": "-");
}

void DynamicsPopup::updateWidgetsWithBrush()
{
  doc::BrushRef brush = m_delegate->getActiveBrush();
  m_dynamics->maxSize()->setValue(brush->size());
  m_dynamics->maxAngle()->setValue(brush->angle());
}

bool DynamicsPopup::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      m_hotRegion = gfx::Region(boundsOnScreen());
      setHotRegion(m_hotRegion);
      manager()->addMessageFilter(kMouseMoveMessage, this);
      manager()->addMessageFilter(kMouseDownMessage, this);
      disableFlags(IGNORE_MOUSE);

      updateWidgetsWithBrush();
      break;

    case kCloseMessage:
      m_hotRegion.clear();
      manager()->removeMessageFilter(kMouseMoveMessage, this);
      manager()->removeMessageFilter(kMouseDownMessage, this);
      break;

    case kMouseEnterMessage:
      m_velocity.reset();
      break;

    case kMouseMoveMessage: {
      auto mouseMsg = static_cast<MouseMessage*>(msg);

      if (mouseMsg->pointerType() == PointerType::Pen ||
          mouseMsg->pointerType() == PointerType::Eraser) {
        if (m_dynamics->pressurePlaceholder()->isVisible()) {
          m_pressureThreshold->setSensorValue(mouseMsg->pressure());
        }
      }

      if (m_dynamics->velocityPlaceholder()->isVisible()) {
        m_velocity.updateWithDisplayPoint(mouseMsg->position());

        float v = m_velocity.velocity().magnitude()
          / tools::VelocitySensor::kScreenPixelsForFullVelocity;
        v = std::clamp(v, 0.0f, 1.0f);

        m_velocityThreshold->setSensorValue(v);
      }
      break;
    }

    case kMouseDownMessage: {
      if (!msg->display())
        break;

      auto mouseMsg = static_cast<const MouseMessage*>(msg);
      auto screenPos = mouseMsg->screenPosition();
      auto picked = manager()->pickFromScreenPos(screenPos);
      if ((picked == nullptr) ||
          (picked->window() != this &&
           picked->window() != m_ditheringSel->getWindowWidget())) {
        closeWindow(nullptr);
      }
      break;
    }

  }
  return PopupWindow::onProcessMessage(msg);
}

} // namespace app
