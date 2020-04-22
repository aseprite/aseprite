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
#include "ui/message.h"

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
  opts.ditheringAlgorithm = m_ditheringSel->ditheringAlgorithm();
  opts.ditheringMatrix = m_ditheringSel->ditheringMatrix();
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
      break;
    case kCloseMessage:
      m_hotRegion.clear();
      break;
  }
  return PopupWindow::onProcessMessage(msg);
}

} // namespace app
