// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
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

#include "app/app.h"
#include "app/context.h"
#include "app/script/engine.h"
#include "app/ui/skin/skin_theme.h"
#include "base/convert_to.h"
#include "base/file_content.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "base/trim_string.h"
#include "fmt/format.h"
#include "ui/entry.h"
#include "ui/listbox.h"
#include "ui/listitem.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/message_loop.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#ifdef ENABLE_SCRIPTING
  #include "app/script/luacpp.h"
#else
  #error Compile the debugger only when ENABLE_SCRIPTING is defined
#endif

#include "debugger.xml.h"

#include <unordered_map>

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

struct FileContent {
  base::buffer content;
  std::vector<const uint8_t*> lines;

  FileContent() {}
  FileContent(const FileContent&) = delete;
  FileContent& operator=(const FileContent&) = delete;

  void clear()
  {
    content.clear();
    lines.clear();
  }

  void setContent(const std::string& c)
  {
    content = base::buffer((const uint8_t*)c.c_str(),
                           (const uint8_t*)c.c_str() + c.size() + 1); // Include nul char

    update();
  }

  void readContentFromFile(const std::string& filename)
  {
    if (base::is_file(filename))
      content = base::read_file_content(filename);
    else
      content.clear();

    // Ensure one last nul char to print as const char* the last line
    content.push_back(0);

    update();
  }

private:
  void update()
  {
    ASSERT(content.back() == 0);

    // Replace all '\r' to ' ' (for Windows EOL and to avoid to paint
    // a square on each newline)
    for (auto& chr : content) {
      if (chr == '\r')
        chr = ' ';
    }

    // Generate the lines array
    lines.clear();
    for (size_t i = 0; i < content.size(); ++i) {
      lines.push_back(&content[i]);

      size_t j = i;
      for (; j < content.size() && content[j] != '\n'; ++j)
        ;
      if (j < content.size())
        content[j] = 0;
      i = j;
    }
  }
};

using FileContentPtr = std::shared_ptr<FileContent>;

// Cached file content of each visited file in the debugger.
// key=filename -> value=FileContent
std::unordered_map<std::string, FileContentPtr> g_fileContent;

class DebuggerSource : public Widget {
public:
  DebuggerSource() {}

  void clearFile()
  {
    m_fileContent.reset();
    m_maxLineWidth = 0;

    if (View* view = View::getView(this))
      view->updateView();
  }

  void setFileContent(const FileContentPtr& fileContent)
  {
    m_fileContent = fileContent;
    m_maxLineWidth = 0;

    if (View* view = View::getView(this))
      view->updateView();

    setCurrentLine(1);
  }

  void setFile(const std::string& filename, const std::string& content)
  {
    FileContentPtr newFileContent(new FileContent);
    if (!content.empty()) {
      newFileContent->setContent(content);
    }
    else {
      newFileContent->readContentFromFile(filename);
    }
    g_fileContent[filename] = newFileContent;

    setFileContent(newFileContent);
  }

  void setCurrentLine(int currentLine)
  {
    m_currentLine = currentLine;
    if (m_currentLine > 0) {
      if (View* view = View::getView(this)) {
        const gfx::Rect vp = view->viewportBounds();
        const int th = textHeight();
        int y = m_currentLine * th - vp.h / 2 + th / 2;
        if (y < 0)
          y = 0;
        view->setViewScroll(gfx::Point(0, y));
      }
    }
    invalidate();
  }

protected:
  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kMouseWheelMessage: {
        View* view = View::getView(this);
        if (view) {
          auto mouseMsg = static_cast<MouseMessage*>(msg);
          gfx::Point scroll = view->viewScroll();

          if (mouseMsg->preciseWheel())
            scroll += mouseMsg->wheelDelta();
          else
            scroll += mouseMsg->wheelDelta() * textHeight() * 3;

          view->setViewScroll(scroll);
        }
        break;
      }
    }
    return Widget::onProcessMessage(msg);
  }

  void onPaint(PaintEvent& ev) override
  {
    auto theme = SkinTheme::get(this);
    Graphics* g = ev.graphics();
    View* view = View::getView(this);
    gfx::Color linesBg = theme->colors.textboxCodeFace();
    gfx::Color bg = theme->colors.textboxFace();
    gfx::Color fg = theme->colors.textboxText();
    int nlines = (m_fileContent ? m_fileContent->lines.size() : 0);

    gfx::Rect vp;
    if (view)
      vp = view->viewportBounds().offset(-bounds().origin());
    else
      vp = clientBounds();

    auto f = font();
    gfx::Rect linesVp(0, vp.y, getLineNumberColumnWidth(), vp.h);

    // Fill background
    g->fillRect(bg, vp);
    g->fillRect(linesBg, linesVp);

    if (m_fileContent) {
      auto icon = theme->parts.debugContinue()->bitmap(0);
      gfx::Point pt = clientBounds().origin();
      for (int i = 0; i < nlines; ++i) {
        if (i + 1 == m_currentLine) {
          g->drawRgbaSurface(icon, pt.x + linesVp.w, pt.y);
        }

        // Draw the line number
        {
          auto lineNumText = base::convert_to<std::string>(i + 1);
          int lw = Graphics::measureUITextLength(lineNumText, f);
          g->drawText(lineNumText.c_str(),
                      fg,
                      linesBg,
                      gfx::Point(pt.x + linesVp.w - lw - 2 * guiscale(), pt.y));
        }

        // Draw the this line of source code
        const char* line = (const char*)m_fileContent->lines[i];
        g->drawText(line, fg, bg, gfx::Point(pt.x + icon->width() + linesVp.w, pt.y));

        pt.y += textHeight();
      }
    }
  }

  void onSizeHint(SizeHintEvent& ev) override
  {
    if (m_fileContent) {
      if (m_maxLineWidth == 0) {
        auto f = font();
        std::string tmp;
        for (const uint8_t* line : m_fileContent->lines) {
          ASSERT(line);
          tmp.assign((const char*)line);
          m_maxLineWidth = std::max(m_maxLineWidth, Graphics::measureUITextLength(tmp, f));
        }
      }

      ev.setSizeHint(gfx::Size(m_maxLineWidth + getLineNumberColumnWidth(),
                               m_fileContent->lines.size() * textHeight()));
    }
  }

private:
  int getLineNumberColumnWidth() const
  {
    auto f = font();
    int nlines = (m_fileContent ? m_fileContent->lines.size() : 0);
    return Graphics::measureUITextLength(base::convert_to<std::string>(nlines), f) +
           4 * guiscale(); // TODO configurable from the theme?
  }

  FileContentPtr m_fileContent;
  int m_currentLine = -1;
  int m_maxLineWidth = 0;
};

class StacktraceBox : public ListBox {
public:
  class Item : public ListItem {
  public:
    Item(lua_Debug* ar, const int stackLevel)
      : m_fn(ar->short_src)
      , m_ln(ar->currentline)
      , m_stackLevel(stackLevel)
    {
      std::string lineContent;

      auto it = g_fileContent.find(m_fn);
      if (it != g_fileContent.end()) {
        const int i = ar->currentline - 1;
        if (i >= 0 && i < it->second->lines.size())
          lineContent.assign((const char*)it->second->lines[i]);
      }
      base::trim_string(lineContent, lineContent);

      setText(fmt::format("{}:{}: {}", base::get_file_name(m_fn), m_ln, lineContent));
    }

    const std::string& filename() const { return m_fn; }
    const int lineNumber() const { return m_ln; }
    const int stackLevel() const { return m_stackLevel; }

  private:
    std::string m_fn;
    int m_ln;
    int m_stackLevel;
  };

  StacktraceBox() {}

  void clear()
  {
    while (auto item = lastChild()) {
      removeChild(item);
      item->deferDelete();
    }
  }

  void update(lua_State* L)
  {
    clear();

    lua_Debug ar;
    int level = 0;
    while (lua_getstack(L, level, &ar)) {
      lua_getinfo(L, "lnS", &ar);
      if (ar.currentline > 0)
        addChild(new Item(&ar, level));
      ++level;
    }
  }
};

// TODO similar to DevConsoleView::CommmandEntry, merge both widgets
//      or remove the DevConsoleView
class EvalEntry : public Entry {
public:
  EvalEntry() : Entry(2048, "") { setFocusStop(true); }

  obs::signal<void(const std::string&)> Execute;

protected:
  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kKeyDownMessage:
        if (hasFocus()) {
          KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
          KeyScancode scancode = keymsg->scancode();

          switch (scancode) {
            case kKeyEnter:
            case kKeyEnterPad: {
              std::string cmd = text();
              setText("");
              Execute(cmd);
              return true;
            }
          }
        }
        break;
    }
    return Entry::onProcessMessage(msg);
  }
};

} // namespace

class Debugger : public gen::Debugger,
                 public script::EngineDelegate,
                 public script::DebuggerDelegate {
  enum State {
    Hidden,
    WaitingStart,
    WaitingHook,
    WaitingNextCommand,
    ProcessingCommand,
  };

  enum Button {
    None = -1,
    Start = 0, // Start/Pause/Continue
    StepInto,
    StepOver,
    StepOut,
    Breakpoint,
  };

public:
  Debugger()
  {
    control()->ItemChange.connect([this] {
      auto button = (Button)control()->selectedItem();
      control()->deselectItems();
      onControl(button);
    });

    breakpoint()->setVisible(false); // TODO make this visible
    breakpoint()->ItemChange.connect([this] {
      breakpoint()->deselectItems();
      onControl(Button::Breakpoint);
    });

    Close.connect([this] {
      m_state = State::Hidden;

      auto app = App::instance();
      app->scriptEngine()->setDelegate(m_oldDelegate);
      app->scriptEngine()->stopDebugger();

      // Clear the cached content of all debugged files
      g_fileContent.clear();

      // Clear console & locals.
      console()->setText(std::string());
      clearLocals();
    });

    m_stacktrace.Change.connect([this] { onStacktraceChange(); });

    m_evalEntry.Execute.connect([this](const std::string& expr) { onEvalExpression(expr); });

    mainArea()->setVisible(false);

    sourcePlaceholder()->attachToView(&m_sourceViewer);
    sourcePlaceholder()->setVisible(false);

    stackPlaceholder()->attachToView(&m_stacktrace);
    stackPlaceholder()->setVisible(false);

    outputButtons()->ItemChange.connect([this] { onOutputButtonChange(); });
    outputButtons()->setSelectedItem(0);
    onOutputButtonChange();

    console()->setVisible(false);
    locals()->setVisible(false);

    evalPlaceholder()->addChild(&m_evalEntry);
  }

  void openDebugger()
  {
    m_state = State::WaitingHook;

    updateControls();
    openWindow();

    auto app = App::instance();
    m_oldDelegate = app->scriptEngine()->delegate();
    app->scriptEngine()->setDelegate(this);
    app->scriptEngine()->startDebugger(this);
  }

  void onControl(Button button)
  {
    ASSERT(m_state != State::Hidden);

    m_lastCommand = button;

    switch (button) {
      case Button::Start:
        if (m_state == State::WaitingStart) {
          m_state = State::WaitingHook;
          m_commandStackLevel = m_stackLevel = 0;
          m_sourceViewer.clearFile();
        }
        else {
          m_state = State::WaitingStart;
          m_commandStackLevel = m_stackLevel = 0;
          m_sourceViewer.clearFile();
        }
        break;

      case Button::StepInto:
      case Button::StepOver:
      case Button::StepOut:
        m_state = State::ProcessingCommand;
        m_commandStackLevel = m_stackLevel;
        break;

      case Button::Breakpoint:
        // m_state = State::WaitingNextCommand;
        // TODO
        break;
    }

    updateControls();
  }

  void updateControls()
  {
    bool isRunning = (m_state == State::WaitingHook || m_state == State::ProcessingCommand);
    bool canRunCommands = (m_state == State::WaitingNextCommand);

    auto theme = SkinTheme::get(this);
    control()->getItem(0)->setIcon(
      (isRunning ? theme->parts.debugPause() : theme->parts.debugContinue()));

    control()->getItem(1)->setEnabled(canRunCommands);
    control()->getItem(2)->setEnabled(canRunCommands);
    control()->getItem(3)->setEnabled(canRunCommands);
    breakpoint()->getItem(0)->setEnabled(canRunCommands);

    // Calling this we update the mouse widget and we can click the
    // same button several times.
    //
    // TODO why is this needed? shouldn't be this automatic from the
    //      ui::Manager side?
    if (auto man = manager())
      man->_updateMouseWidgets();
  }

  // script::EngineDelegate impl
  void onConsoleError(const char* text) override
  {
    m_fileOk = false;

    onConsolePrint(text);

    // Get error filename and number
    {
      std::vector<std::string> parts;
      base::split_string(text, parts, { ":" });
      if (parts.size() >= 3) {
        const std::string& fn = parts[0];
        const std::string& ln = parts[1];
        if (base::is_file(fn)) {
          m_sourceViewer.setFile(fn, std::string());
          m_sourceViewer.setCurrentLine(std::strtol(ln.c_str(), nullptr, 10));

          sourcePlaceholder()->setVisible(true);
          layout();
        }
      }
    }

    // Stop debugger
    auto app = App::instance();
    waitNextCommand(app->scriptEngine()->luaState());
    updateControls();
  }

  void onConsolePrint(const char* text) override
  {
    console()->setVisible(true);
    consoleView()->setViewScroll(gfx::Point(0, 0));

    if (text) {
      std::string stext(console()->text());
      stext += text;
      stext += '\n';
      console()->setText(stext);
    }

    layout();

    consoleView()->setViewScroll(gfx::Point(0, consoleView()->getScrollableSize().h));
  }

  // script::DebuggerDelegate impl
  void hook(lua_State* L, lua_Debug* ar) override
  {
    lua_getinfo(L, "lnS", ar);

    switch (ar->event) {
      case LUA_HOOKCALL:     ++m_stackLevel; break;
      case LUA_HOOKRET:      --m_stackLevel; break;
      case LUA_HOOKLINE:
      case LUA_HOOKCOUNT:
      case LUA_HOOKTAILCALL: break;
    }

    switch (m_state) {
      case State::WaitingStart:
        // Do nothing (the execution continues regularly, unexpected
        // script being executed)
        return;

      case State::WaitingHook:
        if (ar->event == LUA_HOOKLINE)
          waitNextCommand(L);
        else
          return;
        break;

      case State::ProcessingCommand:
        switch (m_lastCommand) {
          case Button::Start:
            if (ar->event == LUA_HOOKLINE) {
              // TODO Wait next error...
            }
            else {
              return;
            }
            return;

          case Button::StepInto:
            if (ar->event == LUA_HOOKLINE) {
              waitNextCommand(L);
            }
            else {
              return;
            }
            break;

          case Button::StepOver:
            if (ar->event == LUA_HOOKLINE && m_stackLevel == m_commandStackLevel) {
              waitNextCommand(L);
            }
            else {
              return;
            }
            break;

          case Button::StepOut:
            if (ar->event == LUA_HOOKLINE && m_stackLevel < m_commandStackLevel) {
              waitNextCommand(L);
            }
            else {
              return;
            }
            break;

          case Button::Breakpoint:
            if (ar->event != LUA_HOOKLINE) {
              // TODO
              return;
            }
            break;
        }
        break;
    }

    updateControls();

    if (m_state == State::WaitingNextCommand) {
      mainArea()->setVisible(true);
      sourcePlaceholder()->setVisible(true);
      stackPlaceholder()->setVisible(true);
      layout();

      if (m_lastFile != ar->short_src && base::is_file(ar->short_src)) {
        m_lastFile = ar->short_src;
        m_sourceViewer.setFile(m_lastFile, std::string());
      }
      if (m_lastFile == ar->short_src) {
        m_sourceViewer.setCurrentLine(ar->currentline);
      }

      if (!m_expanded) {
        m_expanded = true;
        gfx::Rect bounds = this->bounds();
        if (m_sourceViewer.isVisible()) {
          bounds.w = std::max(bounds.w, 256 * guiscale());
          bounds.h = std::max(bounds.h, 256 * guiscale());
        }
        expandWindow(bounds.size());
        invalidate();
      }

      MessageLoop loop(Manager::getDefault());
      while (m_state == State::WaitingNextCommand)
        loop.pumpMessages();
    }
  }

  void startFile(const std::string& file, const std::string& content) override
  {
    m_stackLevel = 0;
    m_fileOk = true;
    m_sourceViewer.setFile(file, content);
  }

  void endFile(const std::string& file) override
  {
    if (m_fileOk)
      m_sourceViewer.clearFile();
    m_stacktrace.clear();
    m_lastFile.clear();
    m_stackLevel = 0;
  }

private:
  void waitNextCommand(lua_State* L)
  {
    m_state = State::WaitingNextCommand;
    m_stacktrace.update(L);
    updateLocals(L, 0);
  }

  void onOutputButtonChange()
  {
    consoleView()->setVisible(isConsoleSelected());
    localsView()->setVisible(isLocalsSelected());
    layout();
  }

  bool isConsoleSelected() const { return (outputButtons()->selectedItem() == 0); }

  bool isLocalsSelected() const { return (outputButtons()->selectedItem() == 1); }

  void onStacktraceChange()
  {
    if (auto item = dynamic_cast<StacktraceBox::Item*>(m_stacktrace.getSelectedChild())) {
      auto it = g_fileContent.find(item->filename());
      if (it != g_fileContent.end())
        m_sourceViewer.setFileContent(it->second);
      else
        m_sourceViewer.setFile(item->filename(), std::string());
      m_sourceViewer.setCurrentLine(item->lineNumber());

      auto app = App::instance();
      updateLocals(app->scriptEngine()->luaState(), item->stackLevel());
    }
  }

  void onEvalExpression(const std::string& expr)
  {
    auto app = App::instance();
    app->scriptEngine()->evalCode(expr);
  }

  void clearLocals()
  {
    while (auto item = locals()->lastChild()) {
      locals()->removeChild(item);
      item->deferDelete();
    }
  }

  void updateLocals(lua_State* L, int level)
  {
    clearLocals();

    lua_Debug ar;
    if (lua_getstack(L, level, &ar)) {
      for (int n = 1;; ++n) {
        const char* name = lua_getlocal(L, &ar, n);
        if (!name)
          break;

        // These special names are returned by luaG_findlocal()
        if (strcmp(name, "(temporary)") == 0 || strcmp(name, "(C temporary)") == 0) {
          lua_pop(L, 1);
          continue;
        }

        std::string v = "?";
        switch (lua_type(L, -1)) {
          case LUA_TNONE:          v = "none"; break;
          case LUA_TNIL:           v = "nil"; break;
          case LUA_TBOOLEAN:       v = (lua_toboolean(L, -1) ? "true" : "false"); break;
          case LUA_TLIGHTUSERDATA: v = "lightuserdata"; break;
          case LUA_TNUMBER:        v = lua_tostring(L, -1); break;
          case LUA_TSTRING:        v = lua_tostring(L, -1); break;
          case LUA_TTABLE:         v = "table"; break;
          case LUA_TFUNCTION:      v = "function"; break;
          case LUA_TUSERDATA:      v = "userdata"; break;
          case LUA_TTHREAD:        v = "thread"; break;
        }
        std::string itemText = fmt::format("{}={}", name, v);
        lua_pop(L, 1);

        locals()->addChild(new ListItem(itemText));
      }
    }

    locals()->setVisible(true);
    localsView()->updateView();
  }

  EngineDelegate* m_oldDelegate = nullptr;
  bool m_expanded = false;
  State m_state = State::Hidden;
  Button m_lastCommand = Button::None;
  DebuggerSource m_sourceViewer;
  StacktraceBox m_stacktrace;
  EvalEntry m_evalEntry;
  std::string m_lastFile;
  int m_commandStackLevel = 0;
  int m_stackLevel = 0;
  bool m_fileOk = true;
};

DebuggerCommand::DebuggerCommand() : Command(CommandId::Debugger(), CmdRecordableFlag)
{
}

void DebuggerCommand::closeDebugger(Context* ctx)
{
  if (ctx->isUIAvailable()) {
    if (m_debugger)
      m_debugger->closeWindow(nullptr);
  }
}

void DebuggerCommand::onExecute(Context* ctx)
{
  if (ctx->isUIAvailable()) {
    auto app = App::instance();

    // Create the debugger window for the first time
    if (!m_debugger) {
      m_debugger.reset(new Debugger);
      app->Exit.connect([this] { m_debugger.reset(); });
    }

    if (!m_debugger->isVisible()) {
      m_debugger->openDebugger();
    }
    else {
      m_debugger->closeWindow(nullptr);
    }
  }
}

Command* CommandFactory::createDebuggerCommand()
{
  return new DebuggerCommand;
}

} // namespace app
