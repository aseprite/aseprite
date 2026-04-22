// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/label.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/window.h"

namespace ui {

Label::Label(const std::string& text) : Widget(kLabelWidget), m_buddy(nullptr)
{
  enableFlags(IGNORE_MOUSE);
  setAlign(LEFT | MIDDLE);
  setText(text);
  initTheme();
}

Widget* Label::buddy()
{
  if (m_buddy || m_buddyId.empty())
    return m_buddy;

  if (parent()) {
    // This can miss some cases where the hierarchy is not direct
    setBuddy(parent()->findChild(m_buddyId.c_str()));
  }

  return m_buddy;
}

void Label::setBuddy(Widget* buddy)
{
  m_buddy = buddy;

  if (m_buddy)
    disableFlags(IGNORE_MOUSE);
}

void Label::setBuddy(const std::string& buddyId)
{
  m_buddy = nullptr;
  m_buddyId = buddyId;

  if (!m_buddyId.empty())
    disableFlags(IGNORE_MOUSE);
}

bool Label::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage:
      auto* bud = buddy();
      if (bud && bud->isVisible() && !bud->hasFocus()) {
        if (bud->isEnabled() && (bud->type() == kGridWidget || bud->type() == kBoxWidget)) {
          // Iterate through containers until we find something worth focusing
          for (auto* child : bud->children()) {
            if (child->isVisible() && child->isEnabled() && !child->hasFlags(IGNORE_MOUSE)) {
              manager()->setFocus(child, FocusMessage::Source::Buddy);
              return true;
            }
          }
        }

        manager()->setFocus(bud, FocusMessage::Source::Buddy);
        return true;
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

} // namespace ui
