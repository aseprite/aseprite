// Aseprite UI Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/slider.h"

#include "os/font.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace ui {

int Slider::slider_press_x;
int Slider::slider_press_value;
bool Slider::slider_press_left;

Slider::Slider(int min, int max, int value, SliderDelegate* delegate)
  : Widget(kSliderWidget)
  , m_value(value)
  , m_readOnly(false)
  , m_delegate(delegate)
{
  enforceValidRange(min, max);
  setFocusStop(true);
  initTheme();
}

void Slider::setRange(int min, int max)
{
  enforceValidRange(min, max);
  invalidate();
}

void Slider::enforceValidRange(int min, int max)
{
  // Do not allow min > max
  if (min > max)
    max = min;

  m_min = min;
  m_max = max;
  m_value = std::clamp(m_value, min, max);
}

void Slider::setValue(int value)
{
  int old_value = m_value;

  m_value = std::clamp(value, m_min, m_max);

  if (m_value != old_value)
    invalidate();

  // It DOES NOT emit CHANGE signal! to avoid recursive calls.
}

void Slider::getSliderThemeInfo(int* min, int* max, int* value) const
{
  if (min)
    *min = m_min;
  if (max)
    *max = m_max;
  if (value)
    *value = m_value;
}

void Slider::updateValue(int value)
{
  setValue(value);
}

std::string Slider::convertValueToText(int value) const
{
  if (m_delegate)
    return m_delegate->onGetTextFromValue(value);
  else {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%d", value);
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
        int value;
        getSliderThemeInfo(nullptr, nullptr, &value);
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        slider_press_x = mousePos.x;
        slider_press_value = value;
        slider_press_left = static_cast<MouseMessage*>(msg)->left();
      }

      setupSliderCursor();

      [[fallthrough]];

    case kMouseMoveMessage:
      if (hasCapture()) {
        int min, max, value, range;
        gfx::Rect rc = childrenBounds();
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->positionForDisplay(display());

        getSliderThemeInfo(&min, &max, &value);

        range = max - min + 1;

        // With left click
        if (slider_press_left) {
          value = min + range * (mousePos.x - rc.x) / rc.w;
        }
        // With right click
        else {
          int w = rc.w;
          if (rc.w == 0 || range > rc.w) {
            w = 1;
            range = 1;
          }

          value = slider_press_value + (mousePos.x - slider_press_x) * range / w;
        }

        value = std::clamp(value, min, max);
        if (getValue() != value) {
          updateValue(value);
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
        int min, max, value, oldValue;
        getSliderThemeInfo(&min, &max, &value);
        oldValue = value;

        switch (static_cast<KeyMessage*>(msg)->scancode()) {
          case kKeyLeft:     --value; break;
          case kKeyRight:    ++value; break;
          case kKeyPageDown: value -= (max - min + 1) / 4; break;
          case kKeyPageUp:   value += (max - min + 1) / 4; break;
          case kKeyHome:     value = min; break;
          case kKeyEnd:      value = max; break;
          default:           goto not_used;
        }

        value = std::clamp(value, min, max);
        if (oldValue != value) {
          updateValue(value);
          onChange();
        }

        return true;
      }
      break;

    case kMouseWheelMessage:
      if (isEnabled() && !isReadOnly()) {
        int value = m_value + static_cast<MouseMessage*>(msg)->wheelDelta().x -
                    static_cast<MouseMessage*>(msg)->wheelDelta().y;

        value = std::clamp(value, m_min, m_max);

        if (m_value != value) {
          setValue(value);
          onChange();
        }
        return true;
      }
      break;

    case kSetCursorMessage: setupSliderCursor(); return true;
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
