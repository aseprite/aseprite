// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/dynamics_popup.h"

#include "app/ui/dithering_selector.h"
#include "app/ui/skin/skin_theme.h"
#include "base/clamp.h"
#include "os/font.h"
#include "os/surface.h"
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
  void setSensorValue(float v) {
    m_sensorValue = v;
    invalidate();
  }

private:
  void onInitTheme(InitThemeEvent& ev) override {
    Widget::onInitTheme(ev);
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
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
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
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
        u = base::clamp(u, 0.0f, 1.0f);
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
  , m_ditheringSel(new DitheringSelector(DitheringSelector::SelectMatrix))
  , m_fromTo(tools::ColorFromTo::BgToFg)
{
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

  m_dynamics->gradientPlaceholder()->addChild(m_ditheringSel);
  m_dynamics->pressurePlaceholder()->addChild(m_pressureThreshold = new ThresholdSlider);
  m_dynamics->velocityPlaceholder()->addChild(m_velocityThreshold = new ThresholdSlider);
  addChild(m_dynamics);

  onValuesChange(nullptr);
}

tools::DynamicsOptions DynamicsPopup::getDynamics() const
{
  tools::DynamicsOptions opts;
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
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  m_dynamics->values()
    ->getItem(i)
    ->setIcon(state ? theme->parts.dropPixelsOk(): nullptr);
}

bool DynamicsPopup::isCheck(int i) const
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  return (m_dynamics->values()
          ->getItem(i)
          ->icon() == theme->parts.dropPixelsOk());
}

void DynamicsPopup::onValuesChange(ButtonSet::Item* item)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
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

  if (needsSize && !m_dynamics->minSize()->isVisible()) {
    m_dynamics->minSize()->setValue(1);

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

  if (needsAngle && !m_dynamics->minAngle()->isVisible()) {
    m_dynamics->minAngle()->setValue(brush->angle());
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

  auto oldBounds = bounds();
  layout();
  setBounds(gfx::Rect(origin(), sizeHint()));

  m_hotRegion |= gfx::Region(bounds());
  setHotRegion(m_hotRegion);

  if (isVisible())
    manager()->invalidateRect(oldBounds);
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
      m_hotRegion = gfx::Region(bounds());
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
        m_velocity.updateWithScreenPoint(mouseMsg->position());

        float v = m_velocity.velocity().magnitude()
          / tools::VelocitySensor::kScreenPixelsForFullVelocity;
        v = base::clamp(v, 0.0f, 1.0f);

        m_velocityThreshold->setSensorValue(v);
      }
      break;
    }

    case kMouseDownMessage: {
      auto mouseMsg = static_cast<MouseMessage*>(msg);
      auto picked = manager()->pick(mouseMsg->position());
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
