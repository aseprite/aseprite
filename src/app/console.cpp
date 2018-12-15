// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdarg>
#include <cstdio>
#include <vector>

#include "base/bind.h"
#include "base/memory.h"
#include "base/string.h"
#include "ui/ui.h"

#include "app/app.h"
#include "app/console.h"
#include "app/context.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "ui/system.h"

namespace app {

using namespace ui;

class Console::ConsoleWindow : public Window {
public:
  ConsoleWindow() : Window(Window::WithTitleBar, "Console"),
                    m_textbox("", WORDWRAP),
                    m_button("Cancel") {
    m_button.Click.connect(base::Bind<void>(&ConsoleWindow::closeWindow, this, &m_button));

    // When the window is closed, we clear the text
    Close.connect(
      base::Bind<void>(
        [this] {
          m_textbox.setText(std::string());
        }));

    m_view.attachToView(&m_textbox);
    m_button.setMinSize(gfx::Size(60*ui::guiscale(), 0));

    Grid* grid = new Grid(1, false);
    grid->addChildInCell(&m_view, 1, 1, HORIZONTAL | VERTICAL);
    grid->addChildInCell(&m_button, 1, 1, CENTER);
    addChild(grid);

    m_textbox.setFocusMagnet(true);
    m_button.setFocusMagnet(true);
    m_view.setExpansive(true);
  }

  void addMessage(const std::string& msg) {
    if (!m_hasText) {
      m_hasText = true;

      remapWindow();
      setBounds(gfx::Rect(0, 0, ui::display_w()*9/10, ui::display_h()*6/10));
      centerWindow();
      invalidate();
    }

    m_textbox.setText(m_textbox.text() + msg);
  }

  bool isConsoleVisible() const {
    return (m_hasText && isVisible());
  }

private:
  bool onProcessMessage(ui::Message* msg) override {
    if (msg->type() == ui::kKeyDownMessage) {
#if defined __APPLE__
      if (msg->onlyCmdPressed())
#else
      if (msg->onlyCtrlPressed())
#endif
      {
        if (static_cast<KeyMessage*>(msg)->scancode() == kKeyC)
          set_clipboard_text(m_textbox.text());
      }
    }
    return Window::onProcessMessage(msg);
  }

  View m_view;
  TextBox m_textbox;
  Button m_button;
  bool m_hasText = false;
};

int Console::m_consoleCounter = 0;
Console::ConsoleWindow* Console::m_console = nullptr;

Console::Console(Context* ctx)
  : m_withUI(false)
{
  if (!ui::is_ui_thread())
    return;

  if (ctx)
    m_withUI = (ctx->isUIAvailable());
  else
    m_withUI =
      (App::instance() &&
       App::instance()->isGui() &&
       Manager::getDefault() &&
       Manager::getDefault()->getDisplay());

  if (!m_withUI)
    return;

  ++m_consoleCounter;
  if (m_console || m_consoleCounter > 1)
    return;

  m_console = new ConsoleWindow;
}

Console::~Console()
{
  if (!m_withUI)
    return;

  --m_consoleCounter;

  if (m_console && m_console->isConsoleVisible()) {
    m_console->manager()->attractFocus(m_console);
    m_console->openWindowInForeground();
  }

  if (m_consoleCounter == 0) {
    delete m_console;         // window
    m_console = nullptr;
  }
}

void Console::printf(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);
  std::string msg = base::string_vprintf(format, ap);
  va_end(ap);

  if (!m_withUI || !m_console) {
    fputs(msg.c_str(), stdout);
    fflush(stdout);
    return;
  }

  // Open the window
  if (!m_console->isVisible()) {
    m_console->openWindow();
    ui::Manager::getDefault()->invalidate();
  }

  // Update the textbox
  m_console->addMessage(msg);
}

// static
void Console::showException(const std::exception& e)
{
  Console console;
  if (typeid(e) == typeid(std::bad_alloc))
    console.printf("There is not enough memory to complete the action.");
  else
    console.printf("A problem has occurred.\n\nDetails:\n%s\n", e.what());
}

} // namespace app
