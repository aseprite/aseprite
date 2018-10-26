// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/doc.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui_context.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "ui/accelerator.h"
#include "ui/message.h"

#include <algorithm>
#include <set>
#include <vector>

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
    { "ScaleFromCenter"     , "Scale From Center"  , app::KeyAction::ScaleFromCenter },
    { "LockAxis"            , "Lock Axis"          , app::KeyAction::LockAxis },
    { "AddSelection"        , "Add Selection"      , app::KeyAction::AddSelection },
    { "SubtractSelection"   , "Subtract Selection" , app::KeyAction::SubtractSelection },
    { "IntersectSelection"  , "Intersect Selection" , app::KeyAction::IntersectSelection },
    { "AutoSelectLayer"     , "Auto Select Layer"  , app::KeyAction::AutoSelectLayer },
    { "StraightLineFromLastPoint", "Straight Line from Last Point", app::KeyAction::StraightLineFromLastPoint },
    { "AngleSnapFromLastPoint", "Angle Snap from Last Point", app::KeyAction::AngleSnapFromLastPoint },
    { "MoveOrigin"          , "Move Origin"        , app::KeyAction::MoveOrigin },
    { "SquareAspect"        , "Square Aspect"      , app::KeyAction::SquareAspect },
    { "DrawFromCenter"      , "Draw From Center"   , app::KeyAction::DrawFromCenter },
    { "RotateShape"         , "Rotate Shape"       , app::KeyAction::RotateShape },
    { "LeftMouseButton"     , "Trigger Left Mouse Button" , app::KeyAction::LeftMouseButton },
    { "RightMouseButton"    , "Trigger Right Mouse Button" , app::KeyAction::RightMouseButton },
    { NULL                  , NULL                 , app::KeyAction::None }
  };

  static struct {
    const char* name;
    const char* userfriendly;
    app::WheelAction action;
  } wheel_actions[] = {
    { "Zoom"         , "Zoom"                     , app::WheelAction::Zoom },
    { "VScroll"      , "Scroll: Vertically"       , app::WheelAction::VScroll },
    { "HScroll"      , "Scroll: Horizontally"     , app::WheelAction::HScroll },
    { "FgColor"      , "Color: Foreground Palette Entry" , app::WheelAction::FgColor },
    { "BgColor"      , "Color: Background Palette Entry" , app::WheelAction::BgColor },
    { "Frame"        , "Change Frame"             , app::WheelAction::Frame },
    { "BrushSize"    , "Change Brush Size"        , app::WheelAction::BrushSize },
    { "BrushAngle"   , "Change Brush Angle"       , app::WheelAction::BrushAngle },
    { "ToolSameGroup"  , "Change Tool (same group)" , app::WheelAction::ToolSameGroup },
    { "ToolOtherGroup" , "Change Tool"            , app::WheelAction::ToolOtherGroup },
    { "Layer"        , "Change Layer"             , app::WheelAction::Layer },
    { "InkOpacity"   , "Change Ink Opacity"       , app::WheelAction::InkOpacity },
    { "LayerOpacity" , "Change Layer Opacity"     , app::WheelAction::LayerOpacity },
    { "CelOpacity"   , "Change Cel Opacity"       , app::WheelAction::CelOpacity },
    { "Alpha"        , "Color: Alpha"             , app::WheelAction::Alpha },
    { "HslHue"       , "Color: HSL Hue"           , app::WheelAction::HslHue },
    { "HslSaturation", "Color: HSL Saturation"    , app::WheelAction::HslSaturation },
    { "HslLightness" , "Color: HSL Lightness"     , app::WheelAction::HslLightness },
    { "HsvHue"       , "Color: HSV Hue"           , app::WheelAction::HsvHue },
    { "HsvSaturation", "Color: HSV Saturation"    , app::WheelAction::HsvSaturation },
    { "HsvValue"     , "Color: HSV Value"         , app::WheelAction::HsvValue },
    { nullptr        , nullptr                    , app::WheelAction::None }
  };

  const char* get_shortcut(TiXmlElement* elem) {
    const char* shortcut = NULL;

#ifdef _WIN32
    if (!shortcut) shortcut = elem->Attribute("win");
#elif defined __APPLE__
    if (!shortcut) shortcut = elem->Attribute("mac");
#else
    if (!shortcut) shortcut = elem->Attribute("linux");
#endif

    if (!shortcut)
      shortcut = elem->Attribute("shortcut");

    return shortcut;
  }

  std::string get_user_friendly_string_for_keyaction(app::KeyAction action) {
    for (int c=0; actions[c].name; ++c) {
      if (action == actions[c].action)
        return actions[c].userfriendly;
    }
    return std::string();
  }

  std::string get_user_friendly_string_for_wheelaction(app::WheelAction wheelAction) {
    for (int c=0; wheel_actions[c].name; ++c) {
      if (wheelAction == wheel_actions[c].action)
        return wheel_actions[c].userfriendly;
    }
    return std::string();
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

  template<> app::WheelAction convert_to(const std::string& from) {
    app::WheelAction action = app::WheelAction::None;
    for (int c=0; wheel_actions[c].name; ++c) {
      if (from == wheel_actions[c].name)
        return wheel_actions[c].action;
    }
    return action;
  }

  template<> std::string convert_to(const app::WheelAction& from) {
    for (int c=0; wheel_actions[c].name; ++c) {
      if (from == wheel_actions[c].action)
        return wheel_actions[c].name;
    }
    return "";
  }

} // namespace base

namespace app {

using namespace ui;

//////////////////////////////////////////////////////////////////////
// Key

Key::Key(Command* command, const Params& params, KeyContext keyContext)
  : m_type(KeyType::Command)
  , m_useUsers(false)
  , m_keycontext(keyContext)
  , m_command(command)
  , m_params(params)
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
  switch (action) {
    case KeyAction::None:
      m_keycontext = KeyContext::Any;
      break;
    case KeyAction::CopySelection:
    case KeyAction::SnapToGrid:
    case KeyAction::LockAxis:
      m_keycontext = KeyContext::TranslatingSelection;
      break;
    case KeyAction::AngleSnap:
      m_keycontext = KeyContext::RotatingSelection;
      break;
    case KeyAction::MaintainAspectRatio:
    case KeyAction::ScaleFromCenter:
      m_keycontext = KeyContext::ScalingSelection;
      break;
    case KeyAction::AddSelection:
    case KeyAction::SubtractSelection:
    case KeyAction::IntersectSelection:
      m_keycontext = KeyContext::SelectionTool;
      break;
    case KeyAction::AutoSelectLayer:
      m_keycontext = KeyContext::MoveTool;
      break;
    case KeyAction::StraightLineFromLastPoint:
    case KeyAction::AngleSnapFromLastPoint:
      m_keycontext = KeyContext::FreehandTool;
      break;
    case KeyAction::MoveOrigin:
    case KeyAction::SquareAspect:
    case KeyAction::DrawFromCenter:
    case KeyAction::RotateShape:
      m_keycontext = KeyContext::ShapeTool;
      break;
    case KeyAction::LeftMouseButton:
      m_keycontext = KeyContext::Any;
      break;
    case KeyAction::RightMouseButton:
      m_keycontext = KeyContext::Any;
      break;
  }
}

Key::Key(WheelAction wheelAction)
  : m_type(KeyType::WheelAction)
  , m_useUsers(false)
  , m_keycontext(KeyContext::MouseWheel)
  , m_action(KeyAction::None)
  , m_wheelAction(wheelAction)
{
}

void Key::add(const ui::Accelerator& accel,
              const KeySource source,
              KeyboardShortcuts& globalKeys)
{
  Accelerators* accels = &m_accels;

  if (source == KeySource::UserDefined) {
    if (!m_useUsers) {
      m_useUsers = true;
      m_users = m_accels;
    }
    accels = &m_users;
  }

  // Remove the accelerator from other commands
  if (source == KeySource::UserDefined) {
    globalKeys.disableAccel(accel, m_keycontext, this);
    m_userRemoved.remove(accel);
  }

  // Add the accelerator
  accels->add(accel);
}

const ui::Accelerator* Key::isPressed(const Message* msg,
                                      KeyboardShortcuts& globalKeys) const
{
  if (auto keyMsg = dynamic_cast<const KeyMessage*>(msg)) {
    for (const Accelerator& accel : accels()) {
      if (accel.isPressed(keyMsg->modifiers(),
                          keyMsg->scancode(),
                          keyMsg->unicodeChar()) &&
          (m_keycontext == KeyContext::Any ||
           m_keycontext == globalKeys.getCurrentKeyContext())) {
        return &accel;
      }
    }
  }
  else if (auto mouseMsg = dynamic_cast<const MouseMessage*>(msg)) {
    for (const Accelerator& accel : accels()) {
      if ((accel.modifiers() == mouseMsg->modifiers()) &&
          (m_keycontext == KeyContext::Any ||
           // TODO we could have multiple mouse wheel key-context,
           // like "sprite editor" context, or "timeline" context,
           // etc.
           m_keycontext == KeyContext::MouseWheel)) {
        return &accel;
      }
    }
  }
  return nullptr;
}

bool Key::isPressed() const
{
  for (const Accelerator& accel : this->accels()) {
    if (accel.isPressed())
      return true;
  }
  return false;
}

bool Key::isLooselyPressed() const
{
  for (const Accelerator& accel : this->accels()) {
    if (accel.isLooselyPressed())
      return true;
  }
  return false;
}

bool Key::hasAccel(const ui::Accelerator& accel) const
{
  return accels().has(accel);
}

void Key::disableAccel(const ui::Accelerator& accel)
{
  if (!m_useUsers) {
    m_useUsers = true;
    m_users = m_accels;
  }

  m_users.remove(accel);

  if (m_accels.has(accel))
    m_userRemoved.add(accel);
}

void Key::reset()
{
  m_users.clear();
  m_userRemoved.clear();
  m_useUsers = false;
}

void Key::copyOriginalToUser()
{
  m_users = m_accels;
  m_userRemoved.clear();
  m_useUsers = true;
}

std::string Key::triggerString() const
{
  switch (m_type) {
    case KeyType::Command:
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
    case KeyType::WheelAction:
      return get_user_friendly_string_for_wheelaction(m_wheelAction);
  }
  return "Unknown";
}

//////////////////////////////////////////////////////////////////////
// KeyboardShortcuts

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

void KeyboardShortcuts::setKeys(const KeyboardShortcuts& keys,
                                const bool cloneKeys)
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
    bool removed = bool_attr_is_true(xmlKey, "removed");

    if (command_name) {
      Command* command = Commands::instance()->byId(command_name);
      if (command) {
        // Read context
        KeyContext keycontext = KeyContext::Any;
        const char* keycontextstr = xmlKey->Attribute("context");
        if (keycontextstr) {
          if (strcmp(keycontextstr, "Selection") == 0)
            keycontext = KeyContext::SelectionTool;
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

        // add the keyboard shortcut to the command
        KeyPtr key = this->command(command_name, params, keycontext);
        if (key && command_key) {
          Accelerator accel(command_key);

          if (!removed) {
            key->add(accel, source, *this);

            // Add the shortcut to the menuitems with this command
            // (this is only visual, the
            // "CustomizedGuiManager::onProcessMessage" is the only
            // one that process keyboard shortcuts)
            if (key->accels().size() == 1) {
              AppMenus::instance()->applyShortcutToMenuitemsWithCommand(
                command, params, key);
            }
          }
          else
            key->disableAccel(accel);
        }
      }
    }

    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for tools
  // <keyboard><tools><key>
  xmlKey = handle
    .FirstChild("tools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    bool removed = bool_attr_is_true(xmlKey, "removed");

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->tool(tool);
        if (key && tool_key) {
          LOG(VERBOSE) << "KEYS: Shortcut for tool " << tool_id << ": " << tool_key << "\n";
          Accelerator accel(tool_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for quicktools
  // <keyboard><quicktools><key>
  xmlKey = handle
    .FirstChild("quicktools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = get_shortcut(xmlKey);
    bool removed = bool_attr_is_true(xmlKey, "removed");

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->quicktool(tool);
        if (key && tool_key) {
          LOG(VERBOSE) << "KEYS: Shortcut for quicktool " << tool_id << ": " << tool_key << "\n";
          Accelerator accel(tool_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for sprite editor customization
  // <keyboard><actions><key>
  xmlKey = handle
    .FirstChild("actions")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr_is_true(xmlKey, "removed");

    if (action_id) {
      KeyAction action = base::convert_to<KeyAction, std::string>(action_id);
      if (action != KeyAction::None) {
        KeyPtr key = this->action(action);
        if (key && action_key) {
          LOG(VERBOSE) << "KEYS: Shortcut for action " << action_id
                       << ": " << action_key << "\n";
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for mouse wheel customization
  // <keyboard><wheel><key>
  xmlKey = handle
    .FirstChild("wheel")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* action_id = xmlKey->Attribute("action");
    const char* action_key = get_shortcut(xmlKey);
    bool removed = bool_attr_is_true(xmlKey, "removed");

    if (action_id) {
      WheelAction action = base::convert_to<WheelAction, std::string>(action_id);
      if (action != WheelAction::None) {
        KeyPtr key = this->wheelAction(action);
        if (key && action_key) {
          LOG(VERBOSE) << "KEYS: Shortcut for wheel action " << action_id
                       << ": " << action_key << "\n";
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }
}

void KeyboardShortcuts::importFile(const std::string& filename, KeySource source)
{
  XmlDocumentRef doc = app::open_xml(filename);
  TiXmlHandle handle(doc.get());
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
  TiXmlElement wheel("wheel");

  keyboard.SetAttribute("version", XML_KEYBOARD_FILE_VERSION);

  exportKeys(commands, KeyType::Command);
  exportKeys(tools, KeyType::Tool);
  exportKeys(quicktools, KeyType::Quicktool);
  exportKeys(actions, KeyType::Action);
  exportKeys(wheel, KeyType::WheelAction);

  keyboard.InsertEndChild(commands);
  keyboard.InsertEndChild(tools);
  keyboard.InsertEndChild(quicktools);
  keyboard.InsertEndChild(actions);
  keyboard.InsertEndChild(wheel);

  TiXmlDeclaration declaration("1.0", "utf-8", "");
  doc->InsertEndChild(declaration);
  doc->InsertEndChild(keyboard);
  save_xml(doc, filename);
}

void KeyboardShortcuts::exportKeys(TiXmlElement& parent, KeyType type)
{
  for (KeyPtr& key : m_keys) {
    // Save only user defined accelerators.
    if (key->type() != type)
      continue;

    for (const ui::Accelerator& accel : key->userRemovedAccels())
      exportAccel(parent, key.get(), accel, true);

    for (const ui::Accelerator& accel : key->userAccels())
      exportAccel(parent, key.get(), accel, false);
  }
}

void KeyboardShortcuts::exportAccel(TiXmlElement& parent, const Key* key, const ui::Accelerator& accel, bool removed)
{
  TiXmlElement elem("key");

  switch (key->type()) {

    case KeyType::Command: {
      elem.SetAttribute("command", key->command()->id().c_str());

      if (key->keycontext() != KeyContext::Any)
        elem.SetAttribute(
          "context", convertKeyContextToString(key->keycontext()).c_str());

      for (const auto& param : key->params()) {
        if (param.second.empty())
          continue;

        TiXmlElement paramElem("param");
        paramElem.SetAttribute("name", param.first.c_str());
        paramElem.SetAttribute("value", param.second.c_str());
        elem.InsertEndChild(paramElem);
      }
      break;
    }

    case KeyType::Tool:
    case KeyType::Quicktool:
      elem.SetAttribute("tool", key->tool()->getId().c_str());
      break;

    case KeyType::Action:
      elem.SetAttribute("action",
        base::convert_to<std::string>(key->action()).c_str());
      break;

    case KeyType::WheelAction:
      elem.SetAttribute("action",
        base::convert_to<std::string>(key->wheelAction()).c_str());
      break;
  }

  elem.SetAttribute("shortcut", accel.toString().c_str());

  if (removed)
    elem.SetAttribute("removed", "true");

  parent.InsertEndChild(elem);
}

void KeyboardShortcuts::reset()
{
  for (KeyPtr& key : m_keys)
    key->reset();
}

KeyPtr KeyboardShortcuts::command(const char* commandName, const Params& params, KeyContext keyContext)
{
  Command* command = Commands::instance()->byId(commandName);
  if (!command)
    return nullptr;

  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Command &&
        key->keycontext() == keyContext &&
        key->command() == command &&
        key->params() == params) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(command, params, keyContext);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::tool(tools::Tool* tool)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Tool &&
        key->tool() == tool) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(KeyType::Tool, tool);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::quicktool(tools::Tool* tool)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Quicktool &&
        key->tool() == tool) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(KeyType::Quicktool, tool);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::action(KeyAction action)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Action &&
        key->action() == action) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(action);
  m_keys.push_back(key);
  return key;
}

KeyPtr KeyboardShortcuts::wheelAction(WheelAction wheelAction)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        key->wheelAction() == wheelAction) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(wheelAction);
  m_keys.push_back(key);
  return key;
}

void KeyboardShortcuts::disableAccel(const ui::Accelerator& accel,
                                     const KeyContext keyContext,
                                     const Key* newKey)
{
  for (KeyPtr& key : m_keys) {
    if (key->keycontext() == keyContext &&
        key->hasAccel(accel) &&
        // Tools can contain the same keyboard shortcut
        (key->type() != KeyType::Tool ||
         newKey == nullptr ||
         newKey->type() != KeyType::Tool)) {
      key->disableAccel(accel);
    }
  }
}

KeyContext KeyboardShortcuts::getCurrentKeyContext()
{
  Doc* doc = UIContext::instance()->activeDocument();

  if (doc &&
      doc->isMaskVisible() &&
      App::instance()->activeTool()->getInk(0)->isSelection())
    return KeyContext::SelectionTool;
  else
    return KeyContext::Normal;
}

bool KeyboardShortcuts::getCommandFromKeyMessage(const Message* msg, Command** command, Params* params)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Command &&
        key->isPressed(msg, *this)) {
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
    KeyPtr key = action(KeyAction::CopySelection);
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
    if (key->type() == KeyType::Action &&
        key->keycontext() == context &&
        key->isLooselyPressed()) {
      flags = static_cast<KeyAction>(int(flags) | int(key->action()));
    }
  }

  return flags;
}

WheelAction KeyboardShortcuts::getWheelActionFromMouseMessage(const KeyContext context,
                                                              const ui::Message* msg)
{
  WheelAction wheelAction = WheelAction::None;
  const ui::Accelerator* bestAccel = nullptr;
  KeyPtr bestKey;
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        key->keycontext() == context) {
      const ui::Accelerator* accel = key->isPressed(msg, *this);
      if ((accel) &&
          (!bestAccel || bestAccel->modifiers() < accel->modifiers())) {
        bestAccel = accel;
        wheelAction = key->wheelAction();
      }
    }
  }
  return wheelAction;
}

bool KeyboardShortcuts::hasMouseWheelCustomization() const
{
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        !key->userAccels().empty())
      return true;
  }
  return false;
}

void KeyboardShortcuts::clearMouseWheelKeys()
{
  for (auto it=m_keys.begin(); it!=m_keys.end(); ) {
    if ((*it)->type() == KeyType::WheelAction)
      it = m_keys.erase(it);
    else
      ++it;
  }
}

void KeyboardShortcuts::addMissingMouseWheelKeys()
{
  for (int wheelAction=int(WheelAction::First);
       wheelAction<=int(WheelAction::Last); ++wheelAction) {
    auto it = std::find_if(
      m_keys.begin(), m_keys.end(),
      [wheelAction](const KeyPtr& key) -> bool {
        return key->wheelAction() == (WheelAction)wheelAction;
      });
    if (it == m_keys.end()) {
      KeyPtr key = std::make_shared<Key>((WheelAction)wheelAction);
      m_keys.push_back(key);
    }
  }
}

void KeyboardShortcuts::setDefaultMouseWheelKeys(const bool zoomWithWheel)
{
  clearMouseWheelKeys();

  KeyPtr key;
  key = std::make_shared<Key>(WheelAction::Zoom);
  key->add(Accelerator(zoomWithWheel ? kKeyNoneModifier:
                                       kKeyCtrlModifier, kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  if (!zoomWithWheel) {
    key = std::make_shared<Key>(WheelAction::VScroll);
    key->add(Accelerator(kKeyNoneModifier, kKeyNil, 0),
             KeySource::Original, *this);
    m_keys.push_back(key);
  }

  key = std::make_shared<Key>(WheelAction::HScroll);
  key->add(Accelerator(kKeyShiftModifier, kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  key = std::make_shared<Key>(WheelAction::FgColor);
  key->add(Accelerator(kKeyAltModifier, kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  key = std::make_shared<Key>(WheelAction::BgColor);
  key->add(Accelerator((KeyModifiers)(kKeyAltModifier | kKeyShiftModifier), kKeyNil, 0),
           KeySource::Original, *this);
  m_keys.push_back(key);

  if (zoomWithWheel) {
    key = std::make_shared<Key>(WheelAction::BrushSize);
    key->add(Accelerator(kKeyCtrlModifier, kKeyNil, 0),
             KeySource::Original, *this);
    m_keys.push_back(key);

    key = std::make_shared<Key>(WheelAction::Frame);
    key->add(Accelerator((KeyModifiers)(kKeyCtrlModifier | kKeyShiftModifier), kKeyNil, 0),
             KeySource::Original, *this);
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

std::string key_tooltip(const char* str, const app::Key* key)
{
  std::string res;
  if (str)
    res += str;
  if (key && !key->accels().empty()) {
    res += " (";
    res += key->accels().front().toString();
    res += ")";
  }
  return res;
}

std::string convertKeyContextToString(KeyContext keyContext)
{
  switch (keyContext) {
    case KeyContext::Any:
      return std::string();
    case KeyContext::Normal:
      return "Normal";
    case KeyContext::SelectionTool:
      return "Selection";
    case KeyContext::TranslatingSelection:
      return "TranslatingSelection";
    case KeyContext::ScalingSelection:
      return "ScalingSelection";
    case KeyContext::RotatingSelection:
      return "RotatingSelection";
    case KeyContext::MoveTool:
      return "MoveTool";
    case KeyContext::FreehandTool:
      return "FreehandTool";
    case KeyContext::ShapeTool:
      return "ShapeTool";
  }
  return std::string();
}

std::string convertKeyContextToUserFriendlyString(KeyContext keyContext)
{
  switch (keyContext) {
    case KeyContext::Any:
      return std::string();
    case KeyContext::Normal:
      return "Normal";
    case KeyContext::SelectionTool:
      return "Selection";
    case KeyContext::TranslatingSelection:
      return "Translating Selection";
    case KeyContext::ScalingSelection:
      return "Scaling Selection";
    case KeyContext::RotatingSelection:
      return "Rotating Selection";
    case KeyContext::MoveTool:
      return "Move Tool";
    case KeyContext::FreehandTool:
      return "Freehand Tool";
    case KeyContext::ShapeTool:
      return "Shape Tool";
  }
  return std::string();
}

} // namespace app
