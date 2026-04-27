// Aseprite
// Copyright (C) 2021-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/commands/debugger.h"
#include "app/console.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/script/debugger.h"
#include "ui/alert.h"
#include "ui/system.h"

#include "debugger.xml.h"

namespace app {

using namespace ui;

constexpr auto kDefaultAddress = "localhost";
constexpr uint16_t kDefaultPort = 58000;

class DebuggerWindow final : public gen::Debugger {
public:
  DebuggerWindow() : m_debugger(nullptr)
  {
    addressEntry()->setPlaceholder(kDefaultAddress);
    portEntry()->setPlaceholder(std::to_string(kDefaultPort));
    toggleDebugger()->Click.connect(&DebuggerWindow::toggle, this);
  }

  void toggle()
  {
    if (m_debugger) {
      if (m_debugger->isDebugging()) {
        // TODO: We could also have a signal coming from the debugger to tell us when we have an
        // active session (and some information) so we can change the UI accordingly. Having a
        // button to disconnect the debugging session from aseprite would also be good, with some
        // kind of indicator.
        Alert::show(Strings::alerts_debugger_end_session_before_disabling());
        return;
      }
      toggleDebugger()->setText(Strings::debugger_enable());
      m_debugger.reset();
    }
    else {
      toggleDebugger()->setText(Strings::debugger_disable());
      std::string address = kDefaultAddress;
      if (!addressEntry()->text().empty())
        address = addressEntry()->text();

      uint16_t port = kDefaultPort;
      const auto entryPort = portEntry()->textInt();
      if (entryPort > 0 && entryPort <= UINT16_MAX)
        port = portEntry()->textInt();

      m_debugger = std::make_unique<script::Debugger>(address, port);
      m_debugger->Error.connect([](const std::string& error) {
        execute_now_or_enqueue([error] { Console::println(error); });
      });
    }
  }

private:
  std::unique_ptr<script::Debugger> m_debugger;
};

static DebuggerWindow* g_window = nullptr;

DebuggerCommand::DebuggerCommand() : Command(CommandId::Debugger())
{
}

bool DebuggerCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

void DebuggerCommand::onExecute(Context*)
{
  if (!g_window)
    g_window = new DebuggerWindow();

  g_window->openWindow();
}

Command* CommandFactory::createDebuggerCommand()
{
  return new DebuggerCommand;
}

} // namespace app
