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
#include <cmath>

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

// Special slider to set min/max values of a sensor
class DynamicsPopup::MinMaxSlider : public Widget  {
public:
  MinMaxSlider() {
    setExpansive(true);
  }

  float minThreshold() const { return m_minThreshold; }
  float maxThreshold() const { return m_maxThreshold; }
  void setSensorValue(float v) {
    m_sensorValue = v;
    invalidate();
  }

private:
  void onInitTheme(InitThemeEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    setBorder(
      gfx::Border(
        theme->parts.miniSliderEmpty()->bitmapW()->width(),
        theme->parts.miniSliderEmpty()->bitmapN()->height(),
        theme->parts.miniSliderEmpty()->bitmapE()->width(),
        theme->parts.miniSliderEmpty()->bitmapS()->height()));

    Widget::onInitTheme(ev);
  }

  void onSizeHint(SizeHintEvent& ev) override {
    int w = 0;
    int h = 2*textHeight();

    w += border().width();
    h += border().height();

    ev.setSizeHint(w, h);
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    gfx::Rect rc = clientBounds();
    gfx::Color bgcolor = bgColor();
    g->fillRect(bgcolor, rc);

    rc.shrink(border());
    const int minX = this->minX();
    const int maxX = this->maxX();
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
        const int u = mouseMsg->position().x - origin().x;
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
    return rc.x + float(rc.w)*m_minThreshold;
  }

  int maxX() const {
    gfx::Rect rc = clientBounds();
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
{
  m_dynamics->values()->ItemChange.connect(
    [this](ButtonSet::Item* item){
      onValuesChange(item);
    });

  m_dynamics->gradientPlaceholder()->addChild(m_ditheringSel);
  m_dynamics->pressurePlaceholder()->addChild(m_pressureTweaks = new MinMaxSlider);
  m_dynamics->velocityPlaceholder()->addChild(m_velocityTweaks = new MinMaxSlider);
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
  opts.maxSize = m_dynamics->maxSize()->getValue();
  opts.maxAngle = m_dynamics->maxAngle()->getValue();
  opts.ditheringMatrix = m_ditheringSel->ditheringMatrix();

  opts.minPressureThreshold = m_pressureTweaks->minThreshold();
  opts.maxPressureThreshold = m_pressureTweaks->maxThreshold();
  opts.minVelocityThreshold = m_velocityTweaks->minThreshold();
  opts.maxVelocityThreshold = m_velocityTweaks->maxThreshold();

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

  if (needsSize && !m_dynamics->maxSize()->isVisible()) {
    m_dynamics->maxSize()->setValue(
      base::clamp(std::max(2*brush->size(), 4), 1, 64));
  }
  m_dynamics->maxSizeLabel()->setVisible(needsSize);
  m_dynamics->maxSize()->setVisible(needsSize);

  if (needsAngle && !m_dynamics->maxAngle()->isVisible()) {
    m_dynamics->maxAngle()->setValue(brush->angle());
  }
  m_dynamics->maxAngleLabel()->setVisible(needsAngle);
  m_dynamics->maxAngle()->setVisible(needsAngle);

  m_dynamics->gradientLabel()->setVisible(needsGradient);
  m_dynamics->gradientPlaceholder()->setVisible(needsGradient);

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

bool DynamicsPopup::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      m_hotRegion = gfx::Region(bounds());
      setHotRegion(m_hotRegion);
      manager()->addMessageFilter(kMouseMoveMessage, this);
      disableFlags(IGNORE_MOUSE);
      break;

    case kCloseMessage:
      m_hotRegion.clear();
      manager()->removeMessageFilter(kMouseMoveMessage, this);
      break;

    case kMouseEnterMessage: {
      auto mouseMsg = static_cast<MouseMessage*>(msg);
      m_lastPos = mouseMsg->position();
      m_velocity = gfx::Point(0, 0);
      m_lastPointerT = base::current_tick();
      break;
    }

    case kMouseMoveMessage: {
      auto mouseMsg = static_cast<MouseMessage*>(msg);

      if (mouseMsg->pointerType() == PointerType::Pen ||
          mouseMsg->pointerType() == PointerType::Eraser) {
        if (m_dynamics->pressurePlaceholder()->isVisible()) {
          m_pressureTweaks->setSensorValue(mouseMsg->pressure());
        }
      }

      if (m_dynamics->velocityPlaceholder()->isVisible()) {
        // TODO merge this with ToolLoopManager::getSpriteStrokePt() and
        //                      ToolLoopManager::adjustPointWithDynamics()
        const base::tick_t t = base::current_tick();
        const base::tick_t dt = t - m_lastPointerT;
        m_lastPointerT = t;

        float a = base::clamp(float(dt) / 50.0f, 0.0f, 1.0f);

        gfx::Point newVelocity(mouseMsg->position() - m_lastPos);
        m_velocity.x = (1.0f-a)*m_velocity.x + a*newVelocity.x;
        m_velocity.y = (1.0f-a)*m_velocity.y + a*newVelocity.y;
        m_lastPos = mouseMsg->position();

        float v = float(std::sqrt(m_velocity.x*m_velocity.x +
                                  m_velocity.y*m_velocity.y)) / 32.0f; // TODO 32 should be configurable
        v = base::clamp(v, 0.0f, 1.0f);
        m_velocityTweaks->setSensorValue(v);
      }
      break;
    }
  }
  return PopupWindow::onProcessMessage(msg);
}

} // namespace app
