// Aseprite
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_selector.h"

#include "ui/message.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"

#include <cmath>

namespace app {

using namespace ui;

ColorSelector::ColorSelector()
  : Widget(kGenericWidget)
  , m_lockColor(false)
{
}

void ColorSelector::selectColor(const app::Color& color)
{
  if (m_lockColor)
    return;

  m_color = color;
  invalidate();
}

void ColorSelector::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(32*ui::guiscale(), 32*ui::guiscale()));
}

bool ColorSelector::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kMouseWheelMessage:
      if (!hasCapture()) {
        double scale = 1.0;
        if (msg->shiftPressed() ||
            msg->ctrlPressed() ||
            msg->altPressed()) {
          scale = 15.0;
        }

        double newHue = m_color.getHue()
          + scale*(+ static_cast<MouseMessage*>(msg)->wheelDelta().x
                   - static_cast<MouseMessage*>(msg)->wheelDelta().y);

        while (newHue < 0.0)
          newHue += 360.0;
        newHue = std::fmod(newHue, 360.0);

        if (newHue != m_color.getHue()) {
          app::Color newColor =
            app::Color::fromHsv(
              newHue,
              m_color.getSaturation(),
              m_color.getValue());

          ColorChange(newColor, kButtonNone);
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

} // namespace app
