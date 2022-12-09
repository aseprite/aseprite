// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/app_menuitem.h"

#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/modules/gui.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui_context.h"
#include "os/menus.h"
#include "ui/accelerator.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/widget.h"

#include <cstdarg>
#include <string>

namespace app {

using namespace ui;

// static
Params AppMenuItem::s_contextParams;

AppMenuItem::AppMenuItem(const std::string& text,
                         const std::string& commandId,
                         const Params& params)
 : MenuItem(text)
 , m_key(nullptr)
 , m_commandId(commandId)
 , m_params(params)
 , m_native(nullptr)
{
}

Command* AppMenuItem::getCommand() const
{
  if (!m_commandId.empty())
    return Commands::instance()->byId(m_commandId.c_str());
  else
    return nullptr;
}

void AppMenuItem::setKey(const KeyPtr& key)
{
  m_key = key;
  syncNativeMenuItemKeyShortcut();
}

void AppMenuItem::setNative(const Native& native)
{
  if (!m_native)
    m_native.reset(new Native(native));
  else
    *m_native = native;
}

void AppMenuItem::disposeNative()
{
  m_native.reset();
}

void AppMenuItem::syncNativeMenuItemKeyShortcut()
{
  if (m_native) {
    os::Shortcut shortcut = get_os_shortcut_from_key(m_key.get());

    m_native->shortcut = shortcut;
    m_native->menuItem->setShortcut(shortcut);
    m_native->keyContext = (m_key ? m_key->keycontext(): KeyContext::Any);
  }
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
      // Don't disable items with submenus
      if (!hasSubmenu()) {
        // Disable the menu item (the keyboard shortcuts are processed
        // by "manager_msg_proc")
        setEnabled(false);
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

  if (!m_commandId.empty()) {
    Params params = m_params;
    if (!s_contextParams.empty())
      params |= s_contextParams;

    // Load parameters to call Command::isEnabled, so we can check if
    // the command is enabled with this parameters.
    if (auto command = getCommand()) {
      command->loadParams(params);

      UIContext* context = UIContext::instance();
      if (command->isEnabled(context))
        context->executeCommandFromMenuOrShortcut(command, params);

      // TODO At this point, the "this" pointer might be deleted if
      //      the command reloaded the App main menu
    }
  }
}

void AppMenuItem::onValidate()
{
  // Update the context flags after opening the menuitem's
  // submenu to update the "enabled" flag of each command
  // correctly.
  Context* context = UIContext::instance();
  context->updateFlags();

  if (auto command = getCommand()) {
    Params params = m_params;
    if (!s_contextParams.empty())
      params |= s_contextParams;

    command->loadParams(params);

    setEnabled(command->isEnabled(context));
    setSelected(command->isChecked(context));
  }
}

} // namespace app
