// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/slider2.h"

#include "app/ui/skin/skin_property.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "ui/manager.h"
#include "ui/message.h"

namespace app {

// TODO merge this code with ColorEntry in color_sliders.cpp
Slider2::Slider2Entry::Slider2Entry(ui::Slider* slider)
  : ui::Entry(4, "0")
  , m_slider(slider)
  , m_recentFocus(false)
{
}

bool Slider2::Slider2Entry::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case ui::kFocusEnterMessage:
      m_recentFocus = true;
      break;

    case ui::kKeyDownMessage:
      if (ui::Entry::onProcessMessage(msg))
        return true;

      if (hasFocus()) {
        int scancode = static_cast<ui::KeyMessage*>(msg)->scancode();

        switch (scancode) {
          // Enter just remove the focus
          case ui::kKeyEnter:
          case ui::kKeyEnterPad:
            releaseFocus();
            return true;

          case ui::kKeyDown:
          case ui::kKeyUp: {
            int value = textInt();
            if (scancode == ui::kKeyDown)
              --value;
            else
              ++value;

            setTextf("%d", base::clamp(value, minValue(), maxValue()));
            selectAllText();

            onChange();
            return true;
          }
        }

        // Process focus movement key here because if our
        // CustomizedGuiManager catches this kKeyDownMessage it
        // will process it as a shortcut to switch the Timeline.
        //
        // Note: The default ui::Manager handles focus movement
        // shortcuts only for foreground windows.
        // TODO maybe that should change
        if (hasFocus() &&
            manager()->processFocusMovementMessage(msg))
          return true;
      }
      return false;
  }

  bool result = Entry::onProcessMessage(msg);

  if (msg->type() == ui::kMouseDownMessage && m_recentFocus) {
    m_recentFocus = false;
    selectAllText();
  }

  return result;
}

Slider2::Slider2(int min, int max, int value)
  : m_slider(min, max, value)
  , m_entry(&m_slider)
{
  m_slider.setExpansive(true);
  m_slider.setSizeHint(gfx::Size(128, 0));
  skin::get_skin_property(&m_entry)->setLook(skin::MiniLook);

  m_slider.Change.connect(base::Bind<void>(&Slider2::onSliderChange, this));
  m_entry.Change.connect(base::Bind<void>(&Slider2::onEntryChange, this));

  addChild(&m_slider);
  addChild(&m_entry);
}

void Slider2::onChange()
{
  Change();
}

void Slider2::onSliderChange()
{
  m_entry.setTextf("%d", m_slider.getValue());
  if (m_entry.hasFocus())
    m_entry.selectAllText();
  onChange();
}

void Slider2::onEntryChange()
{
  int v = m_entry.textInt();
  v = base::clamp(v, m_slider.getMinValue(), m_slider.getMaxValue());
  m_slider.setValue(v);

  onChange();
}

} // namespace app
