// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/app_menuitem.h"

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/modules/gui.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui_context.h"
#include "ui/accelerator.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/widget.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace app {

using namespace ui;

AppMenuItem::AppMenuItem(const char* text, Command* command, const Params* params)
 : MenuItem(text)
 , m_key(NULL)
 , m_command(command)
 , m_params(params ? params->clone(): NULL)
{
}

AppMenuItem::~AppMenuItem()
{
  delete m_params;
}

bool AppMenuItem::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      // disable the menu (the keyboard shortcuts are processed by "manager_msg_proc")
      setEnabled(false);
      break;

    default:
      if (msg->type() == kOpenMessage ||
          msg->type() == kOpenMenuItemMessage) {
        // Update the context flags after opening the menuitem's
        // submenu to update the "enabled" flag of each command
        // correctly.
        Context* context = UIContext::instance();
        context->updateFlags();

        if (m_command) {
          if (m_params)
            m_command->loadParams(m_params);

          setEnabled(m_command->isEnabled(context));
          setSelected(m_command->isChecked(context));
        }
      }
      break;
  }

  return MenuItem::onProcessMessage(msg);
}

void AppMenuItem::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size size(0, 0);

  if (hasText()) {
    size.w =
      + this->border_width.l
      + getTextWidth()
      + (inBar() ? this->child_spacing/4: this->child_spacing)
      + this->border_width.r;

    size.h =
      + this->border_width.t
      + getTextHeight()
      + this->border_width.b;

    if (m_key && !m_key->accels().empty()) {
      size.w += Graphics::measureUIStringLength(
        m_key->accels().front().toString().c_str(), getFont());
    }
  }

  ev.setPreferredSize(size);
}

void AppMenuItem::onClick()
{
  MenuItem::onClick();

  if (m_command) {
    if (m_params)
      m_command->loadParams(m_params);

    UIContext* context = UIContext::instance();
    if (m_command->isEnabled(context))
      context->executeCommand(m_command);
  }
}

} // namespace app
