// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
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
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "base/memory.h"
#include "base/string.h"
#include "ui/system.h"
#include "ui/ui.h"

#include <cstdarg>
#include <cstdio>
#include <memory>

#define TRACE_CON(...) // TRACEARGS(__VA_ARGS__)

namespace app {

using namespace ui;

Console::ConsoleWindow* Console::m_console = nullptr;

class Console::ConsoleWindow final : public Window {
public:
  ConsoleWindow() : Window(Window::WithTitleBar, Strings::debugger_console()),
                    m_textbox("", WORDWRAP),
                    m_button(Strings::debugger_cancel()) {
    TRACE_CON("CON: ConsoleWindow this=", this);

    m_button.Click.connect([this]{ closeWindow(&m_button); });

    // When the main window is closed, we should close the console (in
    // other case the main message loop will continue running for the
    // console too).
    m_mainWindowClosedConn =
      App::instance()->mainWindow()->Close.connect(
        [this]{ closeWindow(nullptr); });

    // When the window is closed, we clear the text
    Close.connect(
      [this]{
        m_mainWindowClosedConn.disconnect();
        m_textbox.setText(std::string());

        Console::m_console->deferDelete();
        Console::m_console = nullptr;
        TRACE_CON("CON: Close signal");
      });

    m_view.attachToView(&m_textbox);

    ui::Grid* grid = new ui::Grid(1, false);
    grid->addChildInCell(&m_view, 1, 1, HORIZONTAL | VERTICAL);
    grid->addChildInCell(&m_button, 1, 1, CENTER);
    addChild(grid);

    m_textbox.setFocusMagnet(true);
    m_button.setFocusMagnet(true);
    m_view.setExpansive(true);

    initTheme();
  }

  ~ConsoleWindow() {
    TRACE_CON("CON: ~ConsoleWindow this=", this);
  }

  void addMessage(const std::string& msg) {
    if (!m_hasText) {
      m_hasText = true;
      centerConsole();
    }

    gfx::Size maxSize = m_view.getScrollableSize();
    gfx::Size visible = m_view.visibleSize();
    gfx::Point pt = m_view.viewScroll();
    const bool autoScroll = (pt.y >= maxSize.h - visible.h);

    m_textbox.setText(m_textbox.text() + msg);

    if (autoScroll) {
      maxSize = m_view.getScrollableSize();
      visible = m_view.visibleSize();
      pt.y = maxSize.h - visible.h;
      m_view.setViewScroll(pt);
    }
  }

  bool hasConsoleText() const {
    return m_hasText;
  }

  void centerConsole() {
    initTheme();

    Display* display = ui::Manager::getDefault()->display();
    const gfx::Rect displayRc = display->bounds();
    gfx::Rect rc;
    rc.w = displayRc.w*9/10;
    rc.h = displayRc.h*6/10;
    rc.x = displayRc.x + displayRc.w/2 - rc.w/2;
    rc.y = displayRc.y + displayRc.h/2 - rc.h/2;

    ui::fit_bounds(display, this, rc);
  }

private:
  // As Esc key activates the close button only on foreground windows,
  // we have to override this method to allow pressing the window
  // close button using Esc key even in this window (which runs in the
  // background).
  bool shouldProcessEscKeyToCloseWindow() const override {
    return true;
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {

      case ui::kKeyDownMessage: {
        auto scancode = static_cast<KeyMessage*>(msg)->scancode();

#if defined __APPLE__
        if (msg->onlyCmdPressed())
#else
        if (msg->onlyCtrlPressed())
#endif
        {
          if (scancode == kKeyC)
            set_clipboard_text(m_textbox.text());
        }

        // Esc to close the window.
        if (auto closeButton = this->closeButton()) {
          bool p = msg->propagateToParent();
          msg->setPropagateToParent(false);

          if (closeButton->sendMessage(msg))
            return true;

          msg->setPropagateToParent(p);
        }

        // Send Enter key to the Close button, Tab to change focus
        if ((scancode == kKeyEnter) ||
            (scancode == kKeyEnterPad))
          return m_button.sendMessage(msg);

        if (scancode == kKeyTab) {
          if (auto mgr = manager())
            return mgr->processFocusMovementMessage(msg);
        }

        // All keys are used if we have this window focused (so they
        // don't trigger commands)
        return true;
      }

      case ui::kKeyUpMessage:
        if (auto closeButton = this->closeButton()) {
          bool p = msg->propagateToParent();
          msg->setPropagateToParent(false);

          if (closeButton->sendMessage(msg))
            return true;

          msg->setPropagateToParent(p);
        }
        break;
    }
    return Window::onProcessMessage(msg);
  }

  void onInitTheme(InitThemeEvent& ev) override {
    Window::onInitTheme(ev);

    m_button.setMinSize(gfx::Size(60*ui::guiscale(), 0));
  }

  obs::scoped_connection m_mainWindowClosedConn;
  View m_view;
  TextBox m_textbox;
  Button m_button;
  bool m_hasText = false;
};

Console::Console(Context* ctx)
  : m_withUI(false)
{
  TRACE_CON("CON: Console this=", this, "ctx=", ctx, "is_ui_thread=", ui::is_ui_thread(), "{");

  if (!ui::is_ui_thread())
    return;

  if (ctx) {
    m_withUI = (ctx->isUIAvailable());
  }
  else {
    m_withUI =
      (App::instance() &&
       App::instance()->isGui() &&
       Manager::getDefault() &&
       Manager::getDefault()->display()->nativeWindow());
  }

  if (!m_withUI)
    return;

  TRACE_CON("CON: -> withUI=", m_withUI);

  if (!m_console)
    m_console = new ConsoleWindow;
}

Console::~Console()
{
  TRACE_CON("CON: } ~Console this=", this, "withUI=", m_withUI);

  if (!m_withUI)
    return;

  if (m_console &&
      m_console->hasConsoleText() &&
      !m_console->isVisible()) {
    m_console->manager()->attractFocus(m_console);
    m_console->openWindow();
    TRACE_CON("CON: openWindow");
  }
}

void Console::printf(const char* format, ...)
{
  std::va_list ap;
  va_start(ap, format);
  std::string msg = base::string_vprintf(format, ap);
  va_end(ap);

  if (!m_withUI) {
    fputs(msg.c_str(), stdout);
    fflush(stdout);
    return;
  }

  // Create the console window if it was closed/deleted by the user
  if (!m_console) {
    m_console = new ConsoleWindow;
  }

  // Open the window if it's hidden
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
