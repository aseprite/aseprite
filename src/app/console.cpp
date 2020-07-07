// Aseprite
// Copyright (C) 2018-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"

#include "app/app.h"
#include "app/context.h"
#include "app/modules/gui.h"
#include "app/ui/status_bar.h"
#include "base/memory.h"
#include "base/string.h"
#include "ui/system.h"
#include "ui/ui.h"


#include <cstdarg>
#include <cstdio>
#include <memory>

namespace app {

using namespace ui;

class Console::ConsoleWindow : public Window {
public:
  ConsoleWindow() : Window(Window::WithTitleBar, "Console"),
                    m_textbox("", WORDWRAP),
                    m_button("Cancel") {
    m_button.Click.connect([this]{ closeWindow(&m_button); });

    // When the window is closed, we clear the text
    Close.connect(
      [this]{
        m_textbox.setText(std::string());
      });

    m_view.attachToView(&m_textbox);

    Grid* grid = new Grid(1, false);
    grid->addChildInCell(&m_view, 1, 1, HORIZONTAL | VERTICAL);
    grid->addChildInCell(&m_button, 1, 1, CENTER);
    addChild(grid);

    m_textbox.setFocusMagnet(true);
    m_button.setFocusMagnet(true);
    m_view.setExpansive(true);

    initTheme();
  }

  void addMessage(const std::string& msg) {
    if (!m_hasText) {
      m_hasText = true;
      centerConsole();
    }

    m_textbox.setText(m_textbox.text() + msg);
  }

  bool isConsoleVisible() const {
    return (m_hasText && isVisible());
  }

  void centerConsole() {
    initTheme();
    remapWindow();
    setBounds(gfx::Rect(0, 0, ui::display_w()*9/10, ui::display_h()*6/10));
    centerWindow();
    invalidate();
  }

private:
  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case ui::kKeyDownMessage:
#if defined __APPLE__
        if (msg->onlyCmdPressed())
#else
        if (msg->onlyCtrlPressed())
#endif
        {
          if (static_cast<KeyMessage*>(msg)->scancode() == kKeyC)
            set_clipboard_text(m_textbox.text());
        }
        break;
    }
    return Window::onProcessMessage(msg);
  }

  void onInitTheme(InitThemeEvent& ev) override {
    Window::onInitTheme(ev);

    m_button.setMinSize(gfx::Size(60*ui::guiscale(), 0));
  }

  View m_view;
  TextBox m_textbox;
  Button m_button;
  bool m_hasText = false;
};

int Console::m_consoleCounter = 0;
std::unique_ptr<Console::ConsoleWindow> Console::m_console = nullptr;

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
       Manager::getDefault()->display());

  if (!m_withUI)
    return;

  ++m_consoleCounter;
  if (m_console || m_consoleCounter > 1)
    return;

  m_console.reset(new ConsoleWindow);
}

Console::~Console()
{
  if (!m_withUI)
    return;

  --m_consoleCounter;

  if (m_console && m_console->isConsoleVisible()) {
    m_console->manager()->attractFocus(m_console.get());
    m_console->openWindowInForeground();
  }

  if (m_consoleCounter == 0)
    m_console.reset();
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
  if (!ui::is_ui_thread()) {
    LOG(ERROR, "A problem has occurred.\n\nDetails:\n%s\n", e.what());
    return;
  }

  Console console;
  if (typeid(e) == typeid(std::bad_alloc))
    console.printf("There is not enough memory to complete the action.");
  else
    console.printf("A problem has occurred.\n\nDetails:\n%s\n", e.what());
}

// static
void Console::notifyNewDisplayConfiguration()
{
  if (m_console)
    m_console->centerConsole();
}

} // namespace app
