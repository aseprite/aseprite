// Aseprite
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/keyboard_shortcuts.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/editor/editor.h"
#include "app/ui/key.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "fmt/format.h"
#include "ui/message.h"
#include "ui/shortcut.h"

#include "tinyxml2.h"

#include <algorithm>
#include <set>
#include <vector>

#define XML_KEYBOARD_FILE_VERSION "1"

using namespace tinyxml2;

namespace {

const char* get_shortcut(XMLElement* elem)
{
  const char* shortcut = NULL;

#ifdef _WIN32
  if (!shortcut)
    shortcut = elem->Attribute("win");
#elif defined __APPLE__
  if (!shortcut)
    shortcut = elem->Attribute("mac");
#else
  if (!shortcut)
    shortcut = elem->Attribute("linux");
#endif

  if (!shortcut)
    shortcut = elem->Attribute("shortcut");

  return shortcut;
}

} // namespace

namespace app {

using namespace ui;

static std::unique_ptr<KeyboardShortcuts> g_singleton;

// static
KeyboardShortcuts* KeyboardShortcuts::instance()
{
  if (!g_singleton)
    g_singleton = std::make_unique<KeyboardShortcuts>();
  return g_singleton.get();
}

// static
void KeyboardShortcuts::destroyInstance()
{
  g_singleton.reset();
}

KeyboardShortcuts::KeyboardShortcuts()
{
  // Strings instance can be nullptr in tests.
  if (auto* strings = Strings::instance()) {
    strings->LanguageChange.connect([] { reset_key_tables_that_depends_on_language(); });
  }
}

KeyboardShortcuts::~KeyboardShortcuts()
{
  clear();
}

void KeyboardShortcuts::setKeys(const KeyboardShortcuts& keys, const bool cloneKeys)
{
  if (cloneKeys) {
    for (const KeyPtr& key : keys)
      m_keys.push_back(std::make_shared<Key>(*key));
  }
  else {
    m_keys = keys.m_keys;
  }
  UserChange();
}

void KeyboardShortcuts::clear()
{
  m_keys.clear();
}

void KeyboardShortcuts::importFile(XMLElement* rootElement, KeySource source)
{
  // <keyboard><commands><key>
  XMLHandle handle(rootElement);
  XMLElement* xmlKey = handle.FirstChildElement("commands").FirstChildElement("key").ToElement();
  while (xmlKey) {
    const char* command_name = xmlKey->Attribute("command");
    const char* command_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (command_name) {
      Command* command = Commands::instance()->byId(command_name);
      if (command) {
        // Read context
        KeyContext keycontext = KeyContext::Any;
        const char* keycontextstr = xmlKey->Attribute("context");
        if (keycontextstr)
          keycontext = base::convert_to<KeyContext>(std::string(keycontextstr));

        // Read params
        Params params;

        XMLElement* xmlParam = xmlKey->FirstChildElement("param");
        while (xmlParam) {
          const char* param_name = xmlParam->Attribute("name");
          const char* param_value = xmlParam->Attribute("value");

          if (param_name && param_value)
            params.set(param_name, param_value);

          xmlParam = xmlParam->NextSiblingElement();
        }

        // add the keyboard shortcut to the command
        KeyPtr key = this->command(command_name, params, keycontext);
        if (key && command_key) {
          Shortcut shortcut(command_key);

          if (!removed) {
            key->add(shortcut, source, *this);

            // Add the shortcut to the menuitems with this command
            // (this is only visual, the
            // "CustomizedGuiManager::onProcessMessage" is the only
            // one that process keyboard shortcuts)
            if (key->shortcuts().size() == 1) {
              AppMenus::instance()->applyShortcutToMenuitemsWithCommand(command, params, key);
            }
          }
          else
            key->disableShortcut(shortcut, source);
        }
      }
    }

    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for tools
  // <keyboard><tools><key>
  xmlKey = handle.FirstChildElement("tools").FirstChildElement("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->tool(tool);
        if (key && tool_key) {
          LOG(VERBOSE, "KEYS: Shortcut for tool %s: %s\n", tool_id, tool_key);
          Shortcut shortcut(tool_key);

          if (!removed)
            key->add(shortcut, source, *this);
          else
            key->disableShortcut(shortcut, source);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for quicktools
  // <keyboard><quicktools><key>
  xmlKey = handle.FirstChildElement("quicktools").FirstChildElement("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->quicktool(tool);
        if (key && tool_key) {
          LOG(VERBOSE, "KEYS: Shortcut for quicktool %s: %s\n", tool_id, tool_key);
          Shortcut shortcut(tool_key);

          if (!removed)
            key->add(shortcut, source, *this);
          else
            key->disableShortcut(shortcut, source);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for sprite editor customization
  // <keyboard><actions><key>
  xmlKey = handle.FirstChildElement("actions").FirstChildElement("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (action_id) {
      KeyAction action = base::convert_to<KeyAction, std::string>(action_id);
      if (action != KeyAction::None) {
        // Read context
        KeyContext keycontext = KeyContext::Any;
        const char* keycontextstr = xmlKey->Attribute("context");
        if (keycontextstr)
          keycontext = base::convert_to<KeyContext>(std::string(keycontextstr));

        KeyPtr key = this->action(action, keycontext);
        if (key && action_key) {
          LOG(VERBOSE,
              "KEYS: Shortcut for action %s/%s: %s\n",
              action_id,
              (keycontextstr ? keycontextstr : "Any"),
              action_key);
          Shortcut shortcut(action_key);

          if (!removed)
            key->add(shortcut, source, *this);
          else
            key->disableShortcut(shortcut, source);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for mouse wheel customization
  // <keyboard><wheel><key>
  xmlKey = handle.FirstChildElement("wheel").FirstChildElement("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (action_id) {
      WheelAction action = base::convert_to<WheelAction, std::string>(action_id);
      if (action != WheelAction::None) {
        KeyPtr key = this->wheelAction(action);
        if (key && action_key) {
          LOG(VERBOSE, "KEYS: Shortcut for wheel action %s: %s\n", action_id, action_key);
          Shortcut shortcut(action_key);

          if (!removed)
            key->add(shortcut, source, *this);
          else
            key->disableShortcut(shortcut, source);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts to simulate mouse wheel actions
  // <keyboard><drag><key>
  xmlKey = handle.FirstChildElement("drag").FirstChildElement("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr(xmlKey, "removed", false);

    if (action_id) {
      WheelAction action = base::convert_to<WheelAction, std::string>(action_id);
      if (action != WheelAction::None) {
        KeyPtr key = this->dragAction(action);
        if (key && action_key) {
          if (auto vector_str = xmlKey->Attribute("vector")) {
            double x, y = 0.0;
            // Parse a string like "double,double"
            x = std::strtod(vector_str, (char**)&vector_str);
            if (vector_str && *vector_str == ',') {
              ++vector_str;
              y = std::strtod(vector_str, nullptr);
            }
            key->setDragVector(DragVector(x, y));
          }

          LOG(VERBOSE, "KEYS: Shortcut for drag action %s: %s\n", action_id, action_key);
          Shortcut shortcut(action_key);

          if (!removed)
            key->add(shortcut, source, *this);
          else
            key->disableShortcut(shortcut, source);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }
}

void KeyboardShortcuts::importFile(const std::string& filename, KeySource source)
{
  XMLDocumentRef doc = app::open_xml(filename);
  XMLHandle handle(doc.get());
  XMLElement* xmlKey = handle.FirstChildElement("keyboard").ToElement();

  importFile(xmlKey, source);
}

void KeyboardShortcuts::exportFile(const std::string& filename)
{
  auto doc = std::make_unique<XMLDocument>();
  XMLElement* keyboard = doc->NewElement("keyboard");
  XMLElement* commands = keyboard->InsertNewChildElement("commands");
  XMLElement* tools = keyboard->InsertNewChildElement("tools");
  XMLElement* quicktools = keyboard->InsertNewChildElement("quicktools");
  XMLElement* actions = keyboard->InsertNewChildElement("actions");
  XMLElement* wheel = keyboard->InsertNewChildElement("wheel");
  XMLElement* drag = keyboard->InsertNewChildElement("drag");

  keyboard->SetAttribute("version", XML_KEYBOARD_FILE_VERSION);

  exportKeys(commands, KeyType::Command);
  exportKeys(tools, KeyType::Tool);
  exportKeys(quicktools, KeyType::Quicktool);
  exportKeys(actions, KeyType::Action);
  exportKeys(wheel, KeyType::WheelAction);
  exportKeys(drag, KeyType::DragAction);

  doc->InsertEndChild(doc->NewDeclaration("xml version=\"1.0\" encoding=\"utf-8\""));
  doc->InsertEndChild(keyboard);
  save_xml(doc.get(), filename);
}

void KeyboardShortcuts::exportKeys(XMLElement* parent, KeyType type)
{
  for (KeyPtr& key : m_keys) {
    // Save only user defined shortcuts.
    if (key->type() != type)
      continue;

    for (const auto& kv : key->delsKeys())
      if (kv.source() == KeySource::UserDefined)
        exportShortcut(parent, key.get(), kv, true);

    for (const auto& kv : key->addsKeys())
      if (kv.source() == KeySource::UserDefined)
        exportShortcut(parent, key.get(), kv, false);
  }
}

// static
void KeyboardShortcuts::exportShortcut(XMLElement* parent,
                                       const Key* key,
                                       const ui::Shortcut& shortcut,
                                       bool removed)
{
  XMLElement* elem = parent->InsertNewChildElement("key");

  switch (key->type()) {
    case KeyType::Command: {
      elem->SetAttribute("command", key->command()->id().c_str());

      if (key->keycontext() != KeyContext::Any) {
        elem->SetAttribute("context", base::convert_to<std::string>(key->keycontext()).c_str());
      }

      for (const auto& param : key->params()) {
        if (param.second.empty())
          continue;

        XMLElement* paramElem = elem->InsertNewChildElement("param");
        paramElem->SetAttribute("name", param.first.c_str());
        paramElem->SetAttribute("value", param.second.c_str());
      }
      break;
    }

    case KeyType::Tool:
    case KeyType::Quicktool: elem->SetAttribute("tool", key->tool()->getId().c_str()); break;

    case KeyType::Action:
      elem->SetAttribute("action", base::convert_to<std::string>(key->action()).c_str());
      if (key->keycontext() != KeyContext::Any)
        elem->SetAttribute("context", base::convert_to<std::string>(key->keycontext()).c_str());
      break;

    case KeyType::WheelAction:
      elem->SetAttribute("action", base::convert_to<std::string>(key->wheelAction()).c_str());
      break;

    case KeyType::DragAction:
      elem->SetAttribute("action", base::convert_to<std::string>(key->wheelAction()).c_str());
      elem->SetAttribute("vector",
                         fmt::format("{},{}", key->dragVector().x, key->dragVector().y).c_str());
      break;
  }

  elem->SetAttribute("shortcut", shortcut.toString().c_str());

  if (removed)
    elem->SetAttribute("removed", "true");
}

void KeyboardShortcuts::reset()
{
  for (KeyPtr& key : m_keys)
    key->reset();
}

KeyPtr KeyboardShortcuts::command(const char* commandName,
                                  const Params& params,
                                  const KeyContext keyContext) const
{
  Command* command = Commands::instance()->byId(commandName);
  if (!command)
    return nullptr;

  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Command && key->keycontext() == keyContext &&
        key->command() == command && key->params() == params) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(command, params, keyContext);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::tool(tools::Tool* tool) const
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Tool && key->tool() == tool) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(KeyType::Tool, tool);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::quicktool(tools::Tool* tool) const
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Quicktool && key->tool() == tool) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(KeyType::Quicktool, tool);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::action(const KeyAction action, const KeyContext keyContext) const
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Action && key->action() == action &&
        key->keycontext() == keyContext) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(action, keyContext);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::wheelAction(const WheelAction wheelAction) const
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction && key->wheelAction() == wheelAction) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(wheelAction);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::dragAction(const WheelAction dragAction) const
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::DragAction && key->wheelAction() == dragAction) {
      return key;
    }
  }

  KeyPtr key = Key::MakeDragAction(dragAction);
  m_keys.push_back(key);
  return key;
}

void KeyboardShortcuts::disableShortcut(const ui::Shortcut& shortcut,
                                        const KeySource source,
                                        const KeyContext keyContext,
                                        const Key* newKey)
{
  for (KeyPtr& key : m_keys) {
    if (key.get() != newKey && key->keycontext() == keyContext && key->hasShortcut(shortcut) &&
        // Tools can contain the same keyboard shortcut
        (key->type() != KeyType::Tool || newKey == nullptr || newKey->type() != KeyType::Tool) &&
        // DragActions can share the same keyboard shortcut (e.g. to
        // change different values using different DragVectors)
        (key->type() != KeyType::DragAction || newKey == nullptr ||
         newKey->type() != KeyType::DragAction)) {
      key->disableShortcut(shortcut, source);
    }
  }
}

// static
KeyContext KeyboardShortcuts::getCurrentKeyContext()
{
  // For shortcuts to Apply/Cancel transformation/moving pixels state.
  auto* editor = Editor::activeEditor();
  if (editor && editor->isMovingPixels()) {
    return KeyContext::Transformation;
  }

  auto* ctx = UIContext::instance();
  Doc* doc = ctx->activeDocument();
  if (doc && doc->isMaskVisible() &&
      // The active key context will be the selectedTool() (in the
      // toolbox) instead of the activeTool() (which depends on the
      // quick tool shortcuts).
      //
      // E.g. If we have the rectangular marquee tool selected
      // (selectedTool()) are going to press keys like alt+left or
      // alt+right to move the selection edge in the selection
      // context, the alt key switches the activeTool() to the
      // eyedropper, but we want to use alt+left and alt+right in the
      // original context (the selection tool).
      App::instance()->activeToolManager()->selectedTool()->getInk(0)->isSelection()) {
    return KeyContext::SelectionTool;
  }

  const view::RealRange& range = ctx->range();
  if (doc && !range.selectedFrames().empty() &&
      (range.type() == view::Range::kFrames || range.type() == view::Range::kCels)) {
    return KeyContext::FramesSelection;
  }

  return KeyContext::Normal;
}

KeyPtr KeyboardShortcuts::findBestKeyFromMessage(const ui::Message* msg,
                                                 KeyContext currentKeyContext,
                                                 std::optional<KeyType> filterByType) const
{
  const KeyContext contexts[] = { currentKeyContext, KeyContext::Normal };
  int n = (contexts[0] != contexts[1] ? 2 : 1);
  KeyPtr bestKey = nullptr;
  const AppShortcut* bestShortcut = nullptr;
  for (int i = 0; i < n; ++i) {
    for (const KeyPtr& key : m_keys) {
      // Skip keys that are not for the specific KeyType (e.g. only for commands).
      if (filterByType.has_value() && key->type() != *filterByType)
        continue;

      const AppShortcut* shortcut = key->isPressed(msg, contexts[i]);
      if (shortcut && (!bestKey || shortcut->fitsBetterThan(currentKeyContext,
                                                            key->keycontext(),
                                                            bestKey->keycontext(),
                                                            *bestShortcut))) {
        bestKey = key;
        bestShortcut = shortcut;
      }
    }
  }
  return bestKey;
}

bool KeyboardShortcuts::getCommandFromKeyMessage(const ui::Message* msg,
                                                 Command** command,
                                                 Params* params,
                                                 KeyContext currentKeyContext)
{
  KeyPtr key = findBestKeyFromMessage(msg, currentKeyContext, std::make_optional(KeyType::Command));
  if (key) {
    ASSERT(key->type() == KeyType::Command);
    if (command)
      *command = key->command();
    if (params)
      *params = key->params();
    return true;
  }
  return false;
}

tools::Tool* KeyboardShortcuts::getCurrentQuicktool(tools::Tool* currentTool)
{
  if (currentTool && currentTool->getInk(0)->isSelection()) {
    KeyPtr key = action(KeyAction::CopySelection, KeyContext::TranslatingSelection);
    if (key && key->isPressed())
      return NULL;
  }

  tools::ToolBox* toolbox = App::instance()->toolBox();

  // Iterate over all tools
  for (tools::Tool* tool : *toolbox) {
    KeyPtr key = quicktool(tool);

    // Collect all tools with the pressed keyboard-shortcut
    if (key && key->isPressed()) {
      return tool;
    }
  }

  return NULL;
}

KeyAction KeyboardShortcuts::getCurrentActionModifiers(KeyContext context)
{
  KeyAction flags = KeyAction::None;

  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Action && key->keycontext() == context && key->isLooselyPressed()) {
      flags = static_cast<KeyAction>(int(flags) | int(key->action()));
    }
  }

  return flags;
}

WheelAction KeyboardShortcuts::getWheelActionFromMouseMessage(const KeyContext context,
                                                              const ui::Message* msg)
{
  WheelAction wheelAction = WheelAction::None;
  const AppShortcut* bestShortcut = nullptr;
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction && key->keycontext() == context) {
      const AppShortcut* shortcut = key->isPressed(msg);
      if ((shortcut) && (!bestShortcut || bestShortcut->modifiers() < shortcut->modifiers())) {
        bestShortcut = shortcut;
        wheelAction = key->wheelAction();
      }
    }
  }
  return wheelAction;
}

Keys KeyboardShortcuts::getDragActionsFromKeyMessage(const ui::Message* msg)
{
  KeyPtr bestKey = nullptr;
  Keys keys;
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::DragAction) {
      const AppShortcut* shortcut = key->isPressed(msg);
      if (shortcut) {
        keys.push_back(key);
      }
    }
  }
  return keys;
}

bool KeyboardShortcuts::hasMouseWheelCustomization() const
{
  return std::any_of(m_keys.begin(), m_keys.end(), [](const KeyPtr& key) {
    return (key->type() == KeyType::WheelAction && key->hasUserDefinedShortcuts());
  });
}

void KeyboardShortcuts::clearMouseWheelKeys()
{
  for (auto it = m_keys.begin(); it != m_keys.end();) {
    if ((*it)->type() == KeyType::WheelAction)
      it = m_keys.erase(it);
    else
      ++it;
  }
}

void KeyboardShortcuts::addMissingMouseWheelKeys()
{
  for (int action = int(WheelAction::First); action <= int(WheelAction::Last); ++action) {
    // Wheel actions
    auto it = std::find_if(m_keys.begin(), m_keys.end(), [action](const KeyPtr& key) -> bool {
      return key->type() == KeyType::WheelAction && key->wheelAction() == (WheelAction)action;
    });
    if (it == m_keys.end()) {
      KeyPtr key = std::make_shared<Key>((WheelAction)action);
      m_keys.push_back(key);
    }

    // Drag actions
    it = std::find_if(m_keys.begin(), m_keys.end(), [action](const KeyPtr& key) -> bool {
      return key->type() == KeyType::DragAction && key->wheelAction() == (WheelAction)action;
    });
    if (it == m_keys.end()) {
      KeyPtr key = Key::MakeDragAction((WheelAction)action);
      m_keys.push_back(key);
    }
  }
}

void KeyboardShortcuts::setDefaultMouseWheelKeys(const bool zoomWithWheel)
{
  clearMouseWheelKeys();

  KeyPtr key;
  key = std::make_shared<Key>(WheelAction::Zoom);
  key->add(Shortcut(zoomWithWheel ? kKeyNoneModifier : kKeyCtrlModifier, kKeyNil, 0),
           KeySource::Original,
           *this);
  m_keys.push_back(key);

  if (!zoomWithWheel) {
    key = std::make_shared<Key>(WheelAction::VScroll);
    key->add(Shortcut(kKeyNoneModifier, kKeyNil, 0), KeySource::Original, *this);
    m_keys.push_back(key);
  }

  key = std::make_shared<Key>(WheelAction::HScroll);
  key->add(Shortcut(kKeyShiftModifier, kKeyNil, 0), KeySource::Original, *this);
  m_keys.push_back(key);

  key = std::make_shared<Key>(WheelAction::FgColor);
  key->add(Shortcut(kKeyAltModifier, kKeyNil, 0), KeySource::Original, *this);
  m_keys.push_back(key);

  key = std::make_shared<Key>(WheelAction::BgColor);
  key->add(Shortcut((KeyModifiers)(kKeyAltModifier | kKeyShiftModifier), kKeyNil, 0),
           KeySource::Original,
           *this);
  m_keys.push_back(key);

  if (zoomWithWheel) {
    key = std::make_shared<Key>(WheelAction::BrushSize);
    key->add(Shortcut(kKeyCtrlModifier, kKeyNil, 0), KeySource::Original, *this);
    m_keys.push_back(key);

    key = std::make_shared<Key>(WheelAction::Frame);
    key->add(Shortcut((KeyModifiers)(kKeyCtrlModifier | kKeyShiftModifier), kKeyNil, 0),
             KeySource::Original,
             *this);
    m_keys.push_back(key);
  }
}

void KeyboardShortcuts::addMissingKeysForCommands()
{
  std::set<std::string> commandsAlreadyAdded;
  for (const KeyPtr& key : m_keys) {
    if (key->type() != KeyType::Command)
      continue;

    if (key->params().empty())
      commandsAlreadyAdded.insert(key->command()->id());
  }

  std::vector<std::string> ids;
  Commands* commands = Commands::instance();
  commands->getAllIds(ids);

  for (const std::string& id : ids) {
    Command* command = commands->byId(id.c_str());

    // Don't add commands that need params (they will be added to
    // the list using the list of keyboard shortcuts from gui.xml).
    if (command->needsParams())
      continue;

    auto it = commandsAlreadyAdded.find(command->id());
    if (it != commandsAlreadyAdded.end())
      continue;

    // Create the new Key element in KeyboardShortcuts for this
    // command without params.
    this->command(command->id().c_str());
  }
}

} // namespace app
