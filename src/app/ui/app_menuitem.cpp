// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "ui/size_hint_event.h"
#include "ui/widget.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

// static
Params AppMenuItem::s_contextParams;

AppMenuItem::AppMenuItem(const char* text, Command* command, const Params& params)
 : MenuItem(text)
 , m_key(NULL)
 , m_command(command)
 , m_params(params)
{
}

// static
void AppMenuItem::setContextParams(const Params& params)
{
  s_contextParams = params;
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
          Params params = m_params;
          if (!s_contextParams.empty())
            params |= s_contextParams;

          m_command->loadParams(params);

          setEnabled(m_command->isEnabled(context));
          setSelected(m_command->isChecked(context));
        }
      }
      break;
  }

  return MenuItem::onProcessMessage(msg);
}

void AppMenuItem::onSizeHint(SizeHintEvent& ev)
{
  gfx::Size size(0, 0);

  if (hasText()) {
    size.w =
      + textWidth()
      + (inBar() ? childSpacing()/4: childSpacing())
      + border().width();

    size.h =
      + textHeight()
      + border().height();

    if (m_key && !m_key->accels().empty()) {
      size.w += Graphics::measureUITextLength(
        m_key->accels().front().toString().c_str(), font());
    }
  }

  ev.setSizeHint(size);
}

void AppMenuItem::onClick()
{
  MenuItem::onClick();

  if (m_command) {
    Params params = m_params;
    if (!s_contextParams.empty())
      params |= s_contextParams;

    m_command->loadParams(params);

    UIContext* context = UIContext::instance();
    if (m_command->isEnabled(context))
      context->executeCommand(m_command, params);
  }
}

} // namespace app
