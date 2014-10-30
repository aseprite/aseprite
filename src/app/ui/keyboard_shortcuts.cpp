/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/keyboard_shortcuts.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/document_location.h"
#include "app/settings/settings.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui_context.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "ui/accelerator.h"
#include "ui/message.h"

#include <algorithm>

#define XML_KEYBOARD_FILE_VERSION "1"

namespace {

  static struct {
    const char* name;
    const char* userfriendly;
    app::KeyAction action;
  } actions[] = {
    { "CopySelection"       , "Copy Selection"     , app::KeyAction::CopySelection },
    { "SnapToGrid"          , "Snap To Grid"       , app::KeyAction::SnapToGrid },
    { "AngleSnap"           , "Angle Snap"         , app::KeyAction::AngleSnap },
    { "MaintainAspectRatio" , "Maintain Aspect Ratio", app::KeyAction::MaintainAspectRatio },
    { "LockAxis"            , "Lock Axis"          , app::KeyAction::LockAxis },
    { "AddSelection"        , "Add Selection"      , app::KeyAction::AddSelection },
    { "SubtractSelection"   , "Subtract Selection" , app::KeyAction::SubtractSelection },
    { NULL                  , NULL                 , app::KeyAction::None }
  };

  const char* get_shortcut(TiXmlElement* elem)
  {
    const char* shortcut = NULL;

#if defined ALLEGRO_WINDOWS
    if (!shortcut) shortcut = elem->Attribute("win");
#elif defined ALLEGRO_MACOSX
    if (!shortcut) shortcut = elem->Attribute("mac");
#elif defined ALLEGRO_UNIX
    if (!shortcut) shortcut = elem->Attribute("linux");
#endif

    if (!shortcut)
      shortcut = elem->Attribute("shortcut");

    return shortcut;
  }

  std::string get_user_friendly_string_for_keyaction(app::KeyAction action)
  {
    for (int c=0; actions[c].name; ++c) {
      if (action == actions[c].action)
        return actions[c].userfriendly;
    }
    return "";
  }
  
} // anonymous namespace

namespace base {

  template<> app::KeyAction convert_to(const std::string& from) {
    app::KeyAction action = app::KeyAction::None;

    for (int c=0; actions[c].name; ++c) {
      if (from == actions[c].name)
        return actions[c].action;
    }

    return action;
  }

  template<> std::string convert_to(const app::KeyAction& from) {
    for (int c=0; actions[c].name; ++c) {
      if (from == actions[c].action)
        return actions[c].name;
    }
    return "";
  }
  
} // namespace base

namespace app {

using namespace ui;

Key::Key(Command* command, const Params* params, KeyContext keyContext)
  : m_type(KeyType::Command)
  , m_useUsers(false)
  , m_keycontext(keyContext)
  , m_command(command)
  , m_params(params ? params->clone(): NULL)
{
}

Key::Key(KeyType type, tools::Tool* tool)
  : m_type(type)
  , m_useUsers(false)
  , m_keycontext(KeyContext::Any)
  , m_tool(tool)
{
}

Key::Key(KeyAction action)
  : m_type(KeyType::Action)
  , m_useUsers(false)
  , m_keycontext(KeyContext::Any)
  , m_action(action)
{
}

void Key::setUserAccels(const Accelerators& accels)
{
  m_useUsers = true;
  m_users = accels;
}

void Key::add(const ui::Accelerator& accel, KeySource source)
{
  Accelerators* accels = &m_accels;

  if (source == KeySource::UserDefined) {
    if (!m_useUsers) {
      m_useUsers = true;
      m_users = m_accels;
    }
    accels = &m_users;

    KeyboardShortcuts::instance()->disableAccel(accel);
  }

  accels->push_back(accel);
}

bool Key::isPressed(Message* msg) const
{
  ASSERT(dynamic_cast<KeyMessage*>(msg) != NULL);

  for (const Accelerator& accel : accels()) {
    if (accel.check(msg->keyModifiers(),
          static_cast<KeyMessage*>(msg)->scancode(),
          static_cast<KeyMessage*>(msg)->unicodeChar()) &&
        (m_keycontext == KeyContext::Any ||
         m_keycontext == KeyboardShortcuts::instance()->getCurrentKeyContext())) {
      return true;
    }
  }

  return false;
}

bool Key::checkFromAllegroKeyArray()
{
  for (const Accelerator& accel : this->accels()) {
    if (accel.checkFromAllegroKeyArray())
      return true;
  }
  return false;
}

bool Key::hasAccel(const ui::Accelerator& accel) const
{
  return (std::find(accels().begin(), accels().end(), accel) != accels().end());
}

void Key::disableAccel(const ui::Accelerator& accel)
{
  if (!m_useUsers) {
    m_useUsers = true;
    m_users = m_accels;
  }

  Accelerators::iterator it = std::find(m_users.begin(), m_users.end(), accel);
  if (it != m_users.end())
    m_users.erase(it);
}

void Key::reset()
{
  m_users.clear();
  m_useUsers = false;
}

std::string Key::triggerString() const
{
  switch (m_type) {
    case KeyType::Command:
      if (m_params)
        m_command->loadParams(m_params);
      return m_command->friendlyName();
    case KeyType::Tool:
    case KeyType::Quicktool: {
      std::string text = m_tool->getText();
      if (m_type == KeyType::Quicktool)
        text += " (quick)";
      return text;
    }
    case KeyType::Action:
      return get_user_friendly_string_for_keyaction(m_action);
  }
  return "Unknown";
}

KeyboardShortcuts* KeyboardShortcuts::instance()
{
  static KeyboardShortcuts* singleton = NULL;
  if (!singleton)
    singleton = new KeyboardShortcuts();
  return singleton;
}

KeyboardShortcuts::KeyboardShortcuts()
{
}

KeyboardShortcuts::~KeyboardShortcuts()
{
  clear();
}

void KeyboardShortcuts::clear()
{
  for (Key* key : m_keys) {
    delete key;
  }
  m_keys.clear();
}

void KeyboardShortcuts::importFile(TiXmlElement* rootElement, KeySource source)
{
  // <keyboard><commands><key>
  TiXmlHandle handle(rootElement);
  TiXmlElement* xmlKey = handle
    .FirstChild("commands")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* command_name = xmlKey->Attribute("command");
    const char* command_key = get_shortcut(xmlKey);

    if (command_name && command_key) {
      Command* command = CommandsModule::instance()->getCommandByName(command_name);
      if (command) {
        // Read context
        KeyContext keycontext = KeyContext::Any;
        const char* keycontextstr = xmlKey->Attribute("context");
        if (keycontextstr) {
          if (strcmp(keycontextstr, "Selection") == 0)
            keycontext = KeyContext::Selection;
          else if (strcmp(keycontextstr, "Normal") == 0)
            keycontext = KeyContext::Normal;
        }

        // Read params
        Params params;

        TiXmlElement* xmlParam = xmlKey->FirstChildElement("param");
        while (xmlParam) {
          const char* param_name = xmlParam->Attribute("name");
          const char* param_value = xmlParam->Attribute("value");

          if (param_name && param_value)
            params.set(param_name, param_value);

          xmlParam = xmlParam->NextSiblingElement();
        }

        PRINTF(" - Shortcut for command `%s' <%s>\n", command_name, command_key);

        // add the keyboard shortcut to the command
        Key* key = this->command(command_name, &params, keycontext);
        if (key) {
          key->add(Accelerator(command_key), source);

          // Add the shortcut to the menuitems with this
          // command (this is only visual, the "manager_msg_proc"
          // is the only one that process keyboard shortcuts)
          if (key->accels().size() == 1) {
            AppMenus::instance()->applyShortcutToMenuitemsWithCommand(
              command, &params, key);
          }
        }
      }
    }

    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for tools
  // <gui><keyboard><tools><key>
  xmlKey = handle
    .FirstChild("tools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);

    if (tool_id && tool_key) {
      tools::Tool* tool = App::instance()->getToolBox()->getToolById(tool_id);
      if (tool) {
        PRINTF(" - Shortcut for tool `%s': <%s>\n", tool_id, tool_key);

        Key* key = this->tool(tool);
        if (key)
          key->add(Accelerator(tool_key), source);
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for quicktools
  // <gui><keyboard><quicktools><key>
  xmlKey = handle
    .FirstChild("quicktools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    if (tool_id && tool_key) {
      tools::Tool* tool = App::instance()->getToolBox()->getToolById(tool_id);
      if (tool) {
        PRINTF(" - Shortcut for quicktool `%s': <%s>\n", tool_id, tool_key);

        Key* key = this->quicktool(tool);
        if (key)
          key->add(Accelerator(tool_key), source);
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for sprite editor customization
  // <gui><keyboard><spriteeditor>
  xmlKey = handle
    .FirstChild("actions")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_action = xmlKey->Attribute("action");
    const char* tool_key = get_shortcut(xmlKey);

    if (tool_action && tool_key) {
      PRINTF(" - Shortcut for sprite editor `%s': <%s>\n", tool_action, tool_key);

      KeyAction action = base::convert_to<KeyAction, std::string>(tool_action);

      if (action != KeyAction::None) {
        Key* key = this->action(action);
        if (key)
          key->add(Accelerator(tool_key), source);
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }
}

void KeyboardShortcuts::importFile(const std::string& filename, KeySource source)
{
  XmlDocumentRef doc = app::open_xml(filename);
  TiXmlHandle handle(doc);
  TiXmlElement* xmlKey = handle.FirstChild("keyboard").ToElement();

  importFile(xmlKey, source);
}

void KeyboardShortcuts::exportFile(const std::string& filename)
{
  XmlDocumentRef doc(new TiXmlDocument());

  TiXmlElement keyboard("keyboard");
  TiXmlElement commands("commands");
  TiXmlElement tools("tools");
  TiXmlElement quicktools("quicktools");
  TiXmlElement actions("actions");

  keyboard.SetAttribute("version", XML_KEYBOARD_FILE_VERSION);

  exportKeys(commands, KeyType::Command);
  exportKeys(tools, KeyType::Tool);
  exportKeys(quicktools, KeyType::Quicktool);
  exportKeys(actions, KeyType::Action);
  
  keyboard.InsertEndChild(commands);
  keyboard.InsertEndChild(tools);
  keyboard.InsertEndChild(quicktools);
  keyboard.InsertEndChild(actions);
  doc->InsertEndChild(keyboard);
  save_xml(doc, filename);
}

void KeyboardShortcuts::exportKeys(TiXmlElement& parent, KeyType type)
{
  for (Key* key : m_keys) {
    // Save only user defined accelerators.
    if (key->type() != type || key->userAccels().empty())
      continue;

    for (const ui::Accelerator& accel : key->userAccels()) {
      TiXmlElement elem("key");

      switch (key->type()) {
        case KeyType::Command:
          elem.SetAttribute("command", key->command()->short_name());
          if (key->params()) {
            for (const auto& param : *key->params()) {
              if (param.second.empty())
                continue;

              TiXmlElement paramElem("param");
              paramElem.SetAttribute("name", param.first.c_str());
              paramElem.SetAttribute("value", param.second.c_str());
              elem.InsertEndChild(paramElem);
            }
          }
          break;
        case KeyType::Tool:
        case KeyType::Quicktool:
          elem.SetAttribute("tool", key->tool()->getId().c_str());
          break;
        case KeyType::Action:
          elem.SetAttribute("action",
            base::convert_to<std::string>(key->action()).c_str());
          break;
      }

      elem.SetAttribute("shortcut", accel.toString().c_str());
      parent.InsertEndChild(elem);
    }
  }
}

void KeyboardShortcuts::reset()
{
  for (Key* key : m_keys)
    key->reset();
}

Key* KeyboardShortcuts::command(const char* commandName,
  Params* params, KeyContext keyContext)
{
  Command* command = CommandsModule::instance()->getCommandByName(commandName);
  if (!command)
    return NULL;

  for (Key* key : m_keys) {
    if (key->type() == KeyType::Command &&
        key->keycontext() == keyContext &&
        key->command() == command &&
        ((!params && key->params()->empty()) ||
          (params && *key->params() == *params))) {
      return key;
    }
  }

  Key* key = new Key(command, params, keyContext);
  m_keys.push_back(key);
  return key;
}

Key* KeyboardShortcuts::tool(tools::Tool* tool)
{
  for (Key* key : m_keys) {
    if (key->type() == KeyType::Tool &&
        key->tool() == tool) {
      return key;
    }
  }

  Key* key = new Key(KeyType::Tool, tool);
  m_keys.push_back(key);
  return key;
}

Key* KeyboardShortcuts::quicktool(tools::Tool* tool)
{
  for (Key* key : m_keys) {
    if (key->type() == KeyType::Quicktool &&
        key->tool() == tool) {
      return key;
    }
  }

  Key* key = new Key(KeyType::Quicktool, tool);
  m_keys.push_back(key);
  return key;
}

Key* KeyboardShortcuts::action(KeyAction action)
{
  for (Key* key : m_keys) {
    if (key->type() == KeyType::Action &&
        key->action() == action) {
      return key;
    }
  }

  Key* key = new Key(action);
  m_keys.push_back(key);
  return key;
}

void KeyboardShortcuts::disableAccel(const ui::Accelerator& accel)
{
  for (Key* key : m_keys) {
    if (key->hasAccel(accel))
      key->disableAccel(accel);
  }
}

KeyContext KeyboardShortcuts::getCurrentKeyContext()
{
  app::Context* ctx = UIContext::instance();
  DocumentLocation location = ctx->activeLocation();
  Document* doc = location.document();

  if (doc &&
      doc->isMaskVisible() &&
      ctx->settings()->getCurrentTool()->getInk(0)->isSelection())
    return KeyContext::Selection;
  else
    return KeyContext::Normal;
}

bool KeyboardShortcuts::getCommandFromKeyMessage(Message* msg, Command** command, Params** params)
{
  for (Key* key : m_keys) {
    if (key->type() == KeyType::Command && key->isPressed(msg)) {
      if (command) *command = key->command();
      if (params) *params = key->params();
      return true;
    }
  }
  return false;
}

tools::Tool* KeyboardShortcuts::getCurrentQuicktool(tools::Tool* currentTool)
{
  if (currentTool && currentTool->getInk(0)->isSelection()) {
    Key* key = action(KeyAction::CopySelection);
    if (key && key->checkFromAllegroKeyArray())
      return NULL;
  }

  tools::ToolBox* toolbox = App::instance()->getToolBox();

  // Iterate over all tools
  for (tools::Tool* tool : *toolbox) {
    Key* key = quicktool(tool);

    // Collect all tools with the pressed keyboard-shortcut
    if (key && key->checkFromAllegroKeyArray()) {
      return tool;
    }
  }

  return NULL;
}

} // namespace app
