// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/match_words.h"
#include "app/modules/gui.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "base/chrono.h"
#include "commands.h"
#include "tinyexpr.h"
#include "ui/entry.h"
#include "ui/fit_bounds.h"
#include "ui/label.h"
#include "ui/message.h"
#include "ui/system.h"

#ifdef ENABLE_SCRIPTING
  #include "app/script/engine.h"
#endif

#include "run_command.xml.h"

namespace app {

using namespace ui;

namespace {
static constexpr auto kMaxVisibleResults = 10;
static constexpr double kDisabledScore = 101;

struct RunnerDB {
  struct Item {
    std::string label;
    std::string searchableText;
    std::string shortcutString;
    Command* command;
    Params params;
  };

  std::vector<Item> items;
};

class RunnerWindow final : public gen::RunCommand {
  enum class Mode : uint8_t { Search, Script, Expression, ShowHelp };

  class CommandItemView final : public HBox {
  public:
    explicit CommandItemView()
    {
      disableFlags(IGNORE_MOUSE);
      setFocusStop(true);

      auto* filler = new BoxFiller();
      filler->setTransparent(true);
      addChild(filler);

      m_pluginTagLabel = new Label(Strings::run_command_extension_tag());
      m_pluginTagLabel->setVisible(false);
      m_pluginTagLabel->InitTheme.connect([this] {
        const auto* theme = skin::SkinTheme::get(this);
        m_pluginTagLabel->setStyle(theme->styles.runnerExtensionTag());
      });
      m_pluginTagLabel->initTheme();
      addChild(m_pluginTagLabel);

      m_shortcutLabel = new Label("");
      m_shortcutLabel->setTransparent(true);
      m_shortcutLabel->setEnabled(false);
      addChild(m_shortcutLabel);

      InitTheme.connect([this] {
        const auto* theme = skin::SkinTheme::get(this);
        this->setStyle(theme->styles.runnerCommandItem());
      });
      initTheme();
    }

    void setItem(const RunnerDB::Item* item)
    {
      m_item = item;

      if (item != nullptr) {
        setText(item->label);
        processMnemonicFromText();

        m_pluginTagLabel->setVisible(item->command->isPlugin());

        if (item->shortcutString.empty()) {
          m_shortcutLabel->setVisible(false);
        }
        else {
          m_shortcutLabel->setText(item->shortcutString);
          m_shortcutLabel->setVisible(true);
        }
      }
      else
        setVisible(false);

      layout();
    }

    void run()
    {
      if (isEnabled())
        Click(m_item);
    }

    obs::signal<void(const RunnerDB::Item*)> Click;

  protected:
    bool onProcessMessage(Message* msg) override
    {
      if (isEnabled()) {
        switch (msg->type()) {
          case kMouseDownMessage: {
            Click(m_item);
          } break;
          case kMouseEnterMessage:
          case kFocusEnterMessage: {
            setSelected(true);
          } break;
          case kMouseLeaveMessage:
          case kFocusLeaveMessage: {
            setSelected(false);
          } break;
          case kSetCursorMessage: {
            set_mouse_cursor(kHandCursor);
            return true;
          }
        }
      }

      return Widget::onProcessMessage(msg);
    }

  private:
    Label* m_shortcutLabel;
    Label* m_pluginTagLabel;
    const RunnerDB::Item* m_item = nullptr;
  };

public:
  explicit RunnerWindow(Context* ctx, RunnerDB* db) : m_db(db), m_context(ctx)
  {
    search()->Change.connect(&RunnerWindow::onTextChange, this);
#ifndef ENABLE_SCRIPTING
    helpLua()->setVisible(false);
#endif

    for (int i = 0; i <= kMaxVisibleResults; i++) {
      auto* view = new CommandItemView;
      view->Click.connect(&RunnerWindow::executeItem, this);
      commandList()->addChild(view);
    }
    commandList()->InitTheme.connect([this] { commandList()->noBorderNoChildSpacing(); });
    commandList()->initTheme();

    setSizeable(false);
  };

protected:
  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kKeyDownMessage: {
        const auto* keyMessage = static_cast<KeyMessage*>(msg);
        if (!search()->hasFocus() &&
            (keyMessage->scancode() == kKeyBackspace || keyMessage->scancode() == kKeyHome)) {
          // Go back to the search
          search()->requestFocus();
          break;
        }

        if (keyMessage->scancode() != kKeyEnter && keyMessage->scancode() != kKeyEnterPad)
          break;

        switch (m_mode) {
          case Mode::Search: {
            for (auto* child : commandList()->children()) {
              if (!child->hasFlags(HIDDEN) && child->isSelected()) {
                static_cast<CommandItemView*>(child)->run();
                return false;
              }
            }
            break;
          }
          case Mode::Expression: {
            // TODO: We could copy the result to the keyboard on Enter? Should tell the user tho.
            closeWindow(nullptr);
            return true;
          }
#ifdef ENABLE_SCRIPTING
          case Mode::Script: {
            executeScript();
            return true;
          }
#endif
          default: break;
        }
      }
    }

    return Window::onProcessMessage(msg);
  }

  // Avoids invalidating every keystroke.
  void setText(const std::string& t)
  {
    if (t != text())
      Window::setText(t);
  }

private:
  void refit()
  {
    if (bounds().size() != sizeHint())
      expandWindow(sizeHint());
  }

  void executeItem(const RunnerDB::Item* item)
  {
    closeWindow(nullptr);
    StatusBar::instance()->showTip(1000, Strings::run_command_tip_executed_command(item->label));
    m_context->executeCommand(item->command, item->params);
  }

#ifdef ENABLE_SCRIPTING
  void executeScript()
  {
    closeWindow(nullptr);

    const base::Chrono timer;
    const std::string& text = search()->text();
    const bool result = App::instance()->scriptEngine()->evalCode(text.substr(1, text.length()));

    // Give some feedback that the code executed, for errors the console will take care of that.
    // We use the timer to avoid showing the tip in cases where the command was obviously
    // successful, like after showing a modal window.
    if (result && timer.elapsed() < 0.5)
      StatusBar::instance()->showTip(1000, Strings::run_command_tip_code_executed());
  }
#endif

  void clear() const
  {
    // Switch between mode or make modeWidget()->
    expressionResult()->setVisible(false);
    help()->setVisible(false);
    commandList()->setVisible(false);
    moreResults()->setVisible(false);
  }

  void setMode(const Mode newMode)
  {
    if (m_mode != newMode)
      clear();

    m_mode = newMode;
  }

  void onTextChange()
  {
    const std::string& text = search()->text();

    switch (text[0]) {
#ifdef ENABLE_SCRIPTING
      case '@':
        setMode(Mode::Script);
        setText(Strings::run_command_title_script());
        // Will only execute when pressing enter.
        // TODO: Add an engine function to check the syntax before running it with luaL_loadstring
        // && lua_isfunction
        break;
#endif
      case '=':
        setMode(Mode::Expression);
        setText(Strings::run_command_title_expression());
        calculateExpression(text.substr(1, text.length()));
        break;
      case '?':
        setMode(Mode::ShowHelp);
        setText(Strings::run_command_title_help());
        help()->setVisible(true);
        break;
      default:
        setMode(Mode::Search);
        setText(Strings::run_command_title());
        searchCommand(text);
        break;
    }

    refit();
  }

  void searchCommand(const std::string& text)
  {
    const std::string& lowerText = base::string_to_lower(text);
    const MatchWords match(lowerText);
    std::multimap<double, const RunnerDB::Item*> results;

    for (RunnerDB::Item& item : m_db->items) {
      if (match(item.searchableText)) {
        // Crude "score" to affect ordering in the multimap of results.
        // Lower is better.

        double score = 0;
        const std::string& lowerLabel = base::string_to_lower(item.label);
        if (lowerLabel != lowerText) {
          // Deprioritize non-exact matches.
          score = 50;

          // Prioritize the ones starting with what we're typing.
          if (lowerLabel.find(lowerText) == 0)
            score -= 10;

          // Simpler commands go first.
          if (item.params.empty())
            score -= 5;
        }

        if (item.command->isPlugin())
          score += 5;

        // Disabled commands go at the bottom.
        item.command->loadParams(item.params);
        if (!item.command->isEnabled(m_context))
          score += kDisabledScore;

        results.emplace(score, &item);
      }
    }

    int resultCount = 0;
    for (auto& [score, itemPtr] : results) {
      auto* view = static_cast<CommandItemView*>(commandList()->at(resultCount));
      view->setVisible(true);
      view->setEnabled(score < kDisabledScore);
      view->setItem(itemPtr);
      view->setSelected(resultCount == 0);

      resultCount++;

      if (resultCount >= kMaxVisibleResults) {
        moreCount()->setText(Strings::run_command_more_result_count(results.size() - resultCount));
        break;
      }
    }
    for (int i = resultCount; i <= kMaxVisibleResults; i++)
      commandList()->at(i)->setVisible(false);

    commandList()->setVisible(resultCount > 0);
    moreResults()->setVisible(resultCount >= kMaxVisibleResults);

    refit();
  }

  void calculateExpression(const std::string& expression) const
  {
    if (expression.empty()) {
      expressionResult()->setVisible(false);
      return;
    }

    int err = 0;
    double v = te_interp(expression.c_str(), &err);
    if (err == 0)
      expressionResult()->setText(fmt::format(" = {}", v));
    else
      expressionResult()->setText(" = NaN");
    expressionResult()->setVisible(true);
  }

  RunnerDB* m_db;
  Context* m_context;
  Mode m_mode = Mode::Search;
};
} // Unnamed namespace

class RunCommandCommand final : public Command {
public:
  RunCommandCommand();

  void invalidateDatabase();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  void loadDatabase();

  std::unique_ptr<RunnerDB> m_db;
  obs::connection m_invalidateConn;
};

RunCommandCommand::RunCommandCommand() : Command(CommandId::RunCommand())
{
}

void RunCommandCommand::invalidateDatabase()
{
  m_db.reset(nullptr);
}

bool RunCommandCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

void RunCommandCommand::onExecute(Context* context)
{
  if (!m_db)
    loadDatabase();

  RunnerWindow window(context, m_db.get());
  window.centerWindow();
  fit_bounds(window.display(),
             &window,
             gfx::Rect(window.bounds().x, 23 * guiscale(), window.bounds().w, window.bounds().h));
  window.openWindowInForeground();
}

void RunCommandCommand::loadDatabase()
{
  std::vector<std::string> allCommandIds;
  Commands::instance()->getAllIds(allCommandIds);

  m_db.reset(new RunnerDB);
  m_db->items.reserve(allCommandIds.size());

  for (const KeyPtr& key : *KeyboardShortcuts::instance()) {
    if (const auto* cmd = key->command()) {
      std::string shortcutString;
      if (!key->shortcuts().empty())
        shortcutString = key->shortcuts()[0].toString();

      // This might help with english-level searches if I wanna look for "clipboard" or something
      // related to the parameters.
      // TODO: Evaluate if this stays.
      std::string paramString;
      if (!key->params().empty()) {
        for (const auto& [name, value] : key->params()) {
          paramString += fmt::format("{} {} ", name, value);
        }
      }

      // Ideally we'd want to have translatable "tags" of some kind for commands so they're easier
      // to find, for example if you search "Sprite New" you can't find "New File" (but you can find
      // New Sprite from Clipboard)
      const std::string searchableString =
        fmt::format("{} {} {} {}", key->triggerString(), cmd->id(), paramString, shortcutString);

      m_db->items.push_back(
        { key->triggerString(), searchableString, shortcutString, key->command(), key->params() });

      // Remove from the command list since we have the better version from Keyboard Shortcuts.
      const auto it = std::find(allCommandIds.begin(), allCommandIds.end(), cmd->id());
      if (it != allCommandIds.end())
        allCommandIds.erase(it);
    }
  }

  // Add all other commands not covered by KeyboardShortcuts
  for (const std::string& id : allCommandIds) {
    Command* command = Commands::instance()->byId(id.c_str());
    const auto& searchableString = fmt::format("{} {}", command->friendlyName(), command->id());
    m_db->items.push_back({ command->friendlyName(), searchableString, {}, command, {} });
  }

  // Sort alphabetically by label
  std::sort(m_db->items.begin(),
            m_db->items.end(),
            [](const RunnerDB::Item& a, const RunnerDB::Item& b) { return a.label < b.label; });

  if (!m_invalidateConn)
    m_invalidateConn =
      AppMenus::instance()->MenusLoaded.connect(&RunCommandCommand::invalidateDatabase, this);
}

Command* CommandFactory::createRunCommandCommand()
{
  return new RunCommandCommand;
}

} // namespace app
