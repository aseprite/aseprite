// Aseprite UI Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/slider.h"

#include "base/clamp.h"
#include "os/font.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace ui {

static int slider_press_x;
static int slider_press_value;
static bool slider_press_left;

Slider::Slider(int min, int max, int value, SliderDelegate* delegate)
  : Widget(kSliderWidget)
  , m_min(min)
  , m_max(max)
  , m_value(base::clamp(value, min, max))
  , m_readOnly(false)
  , m_delegate(delegate)
{
  setFocusStop(true);
  initTheme();
}

void Slider::setRange(int min, int max)
{
  m_min = min;
  m_max = max;
  m_value = base::clamp(m_value, min, max);

  invalidate();
}

void Slider::setValue(int value)
{
  int old_value = m_value;

  m_value = base::clamp(value, m_min, m_max);

  if (m_value != old_value)
    invalidate();

  // It DOES NOT emit CHANGE signal! to avoid recursive calls.
}

void Slider::getSliderThemeInfo(int* min, int* max, int* value) const
{
  if (min) *min = m_min;
  if (max) *max = m_max;
  if (value) *value = m_value;
}

std::string Slider::convertValueToText(int value) const
{
  if (m_delegate)
    return m_delegate->onGetTextFromValue(value);
  else {
    char buf[128];
    std::sprintf(buf, "%d", value);
    return buf;
  }
}

int Slider::convertTextToValue(const std::string& text) const
{
  if (m_delegate)
    return m_delegate->onGetValueFromText(text);
  else {
    return std::strtol(text.c_str(), nullptr, 10);
  }
}

bool Slider::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
    case kFocusLeaveMessage:
      if (isEnabled())
        invalidate();
      break;

    case kMouseDownMessage:
      if (!isEnabled() || isReadOnly())
        return true;

      setSelected(true);
      captureMouse();

      {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        slider_press_x = mousePos.x;
        slider_press_value = m_value;
        slider_press_left = static_cast<MouseMessage*>(msg)->left();
      }

      setupSliderCursor();

      // Fall through

    case kMouseMoveMessage:
      if (hasCapture()) {
        int value, accuracy, range;
        gfx::Rect rc = childrenBounds();
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        range = m_max - m_min + 1;

        // With left click
        if (slider_press_left) {
          value = m_min + range * (mousePos.x - rc.x) / rc.w;
        }
        // With right click
        else {
          accuracy = base::clamp(rc.w / range, 1, rc.w);

          value = slider_press_value +
            (mousePos.x - slider_press_x) / accuracy;
        }

        value = base::clamp(value, m_min, m_max);
        if (m_value != value) {
          setValue(value);
          onChange();
        }

        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        setSelected(false);
        releaseMouse();
        setupSliderCursor();

        onSliderReleased();
      }
      break;

    case kMouseEnterMessage:
    case kMouseLeaveMessage:
      // TODO theme stuff
      if (isEnabled())
        invalidate();
      break;

    case kKeyDownMessage:
      if (hasFocus() && !isReadOnly()) {
        int value = m_value;

        switch (static_cast<KeyMessage*>(msg)->scancode()) {
          case kKeyLeft:     --value; break;
          case kKeyRight:    ++value; break;
          case kKeyPageDown: value -= (m_max-m_min+1)/4; break;
          case kKeyPageUp:   value += (m_max-m_min+1)/4; break;
          case kKeyHome:     value = m_min; break;
          case kKeyEnd:      value = m_max; break;
          default:
            goto not_used;
        }

        value = base::clamp(value, m_min, m_max);
        if (m_value != value) {
          setValue(value);
          onChange();
        }

        return true;
      }
      break;

    case kMouseWheelMessage:
      if (isEnabled() && !isReadOnly()) {
        int value = m_value
          + static_cast<MouseMessage*>(msg)->wheelDelta().x
          - static_cast<MouseMessage*>(msg)->wheelDelta().y;

        value = base::clamp(value, m_min, m_max);

        if (m_value != value) {
          this->setValue(value);
          onChange();
        }
        return true;
      }
      break;

    case kSetCursorMessage:
      setupSliderCursor();
      return true;
  }

not_used:;
  return Widget::onProcessMessage(msg);
}

void Slider::onPaint(PaintEvent& ev)
{
  theme()->paintSlider(ev);
}

void Slider::onChange()
{
  Change(); // Emit Change signal
}

void Slider::onSliderReleased()
{
  SliderReleased();
}

void Slider::setupSliderCursor()
{
  if (hasCapture()) {
    if (slider_press_left)
      set_mouse_cursor(kArrowCursor);
    else
      set_mouse_cursor(kSizeWECursor);
  }
  else
    set_mouse_cursor(kArrowCursor);
}

} // namespace ui
