// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

// static
Params AppMenuItem::s_contextParams;

AppMenuItem::AppMenuItem(const std::string& text,
                         Command* command,
                         const Params& params)
 : MenuItem(text)
 , m_key(nullptr)
 , m_command(command)
 , m_params(params)
 , m_isRecentFileItem(false)
 , m_native(nullptr)
{
}

AppMenuItem::~AppMenuItem()
{
  if (m_native) {
    // Do not call disposeNative(), the native handle will be disposed
    // when the main menu (app menu) is disposed.

    // TODO improve handling of these kind of pointer from laf-os library

    delete m_native;
  }
}

void AppMenuItem::setKey(const KeyPtr& key)
{
  m_key = key;
  syncNativeMenuItemKeyShortcut();
}

void AppMenuItem::setNative(const Native& native)
{
  if (!m_native)
    m_native = new Native(native);
  else {
    // Do not call disposeNative(), the native handle will be disposed
    // when the main menu (app menu) is disposed.

    *m_native = native;
  }
}

void AppMenuItem::disposeNative()
{
#if 0   // TODO fix this and the whole handling of native menu items from laf-os
  if (m_native->menuItem) {
    m_native->menuItem->dispose();
    m_native->menuItem = nullptr;
  }
#endif
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
      // disable the menu (the keyboard shortcuts are processed by "manager_msg_proc")
      setEnabled(false);
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

    // Load parameters to call Command::isEnabled, so we can check if
    // the command is enabled with this parameters.
    m_command->loadParams(params);

    UIContext* context = UIContext::instance();
    if (m_command->isEnabled(context)) {
      context->executeCommandFromMenuOrShortcut(m_command, params);
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

  if (m_command) {
    Params params = m_params;
    if (!s_contextParams.empty())
      params |= s_contextParams;

    m_command->loadParams(params);

    setEnabled(m_command->isEnabled(context));
    setSelected(m_command->isChecked(context));
  }
}

} // namespace app
