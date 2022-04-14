// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
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
#include "app/tools/active_tool.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/key.h"
#include "app/ui_context.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "fmt/format.h"
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
    app::KeyContext context;
  } actions[] = {
    { "CopySelection"       , "Copy Selection"     , app::KeyAction::CopySelection, app::KeyContext::TranslatingSelection },
    { "SnapToGrid"          , "Snap To Grid"       , app::KeyAction::SnapToGrid, app::KeyContext::TranslatingSelection },
    { "LockAxis"            , "Lock Axis"          , app::KeyAction::LockAxis, app::KeyContext::TranslatingSelection },
    { "FineControl"         , "Fine Translating"   , app::KeyAction::FineControl , app::KeyContext::TranslatingSelection },
    { "MaintainAspectRatio" , "Maintain Aspect Ratio", app::KeyAction::MaintainAspectRatio, app::KeyContext::ScalingSelection },
    { "ScaleFromCenter"     , "Scale From Center"  , app::KeyAction::ScaleFromCenter, app::KeyContext::ScalingSelection },
    { "FineControl"         , "Fine Scaling"       , app::KeyAction::FineControl , app::KeyContext::ScalingSelection },
    { "AngleSnap"           , "Angle Snap"         , app::KeyAction::AngleSnap, app::KeyContext::RotatingSelection },
    { "AddSelection"        , "Add Selection"      , app::KeyAction::AddSelection, app::KeyContext::SelectionTool },
    { "SubtractSelection"   , "Subtract Selection" , app::KeyAction::SubtractSelection, app::KeyContext::SelectionTool },
    { "IntersectSelection"  , "Intersect Selection" , app::KeyAction::IntersectSelection, app::KeyContext::SelectionTool },
    { "AutoSelectLayer"     , "Auto Select Layer"  , app::KeyAction::AutoSelectLayer, app::KeyContext::MoveTool },
    { "StraightLineFromLastPoint", "Straight Line from Last Point", app::KeyAction::StraightLineFromLastPoint, app::KeyContext::FreehandTool },
    { "AngleSnapFromLastPoint", "Angle Snap from Last Point", app::KeyAction::AngleSnapFromLastPoint, app::KeyContext::FreehandTool },
    { "MoveOrigin"          , "Move Origin"        , app::KeyAction::MoveOrigin, app::KeyContext::ShapeTool },
    { "SquareAspect"        , "Square Aspect"      , app::KeyAction::SquareAspect, app::KeyContext::ShapeTool },
    { "DrawFromCenter"      , "Draw From Center"   , app::KeyAction::DrawFromCenter, app::KeyContext::ShapeTool },
    { "RotateShape"         , "Rotate Shape"       , app::KeyAction::RotateShape, app::KeyContext::ShapeTool },
    { "LeftMouseButton"     , "Trigger Left Mouse Button" , app::KeyAction::LeftMouseButton, app::KeyContext::Any },
    { "RightMouseButton"    , "Trigger Right Mouse Button" , app::KeyAction::RightMouseButton, app::KeyContext::Any },
    { NULL                  , NULL                 , app::KeyAction::None, app::KeyContext::Any }
  };

  static struct {
    const char* name;
    app::KeyContext context;
  } contexts[] = {
    { ""                     , app::KeyContext::Any },
    { "Normal"               , app::KeyContext::Normal },
    { "Selection"            , app::KeyContext::SelectionTool },
    { "TranslatingSelection" , app::KeyContext::TranslatingSelection },
    { "ScalingSelection"     , app::KeyContext::ScalingSelection },
    { "RotatingSelection"    , app::KeyContext::RotatingSelection },
    { "MoveTool"             , app::KeyContext::MoveTool },
    { "FreehandTool"         , app::KeyContext::FreehandTool },
    { "ShapeTool"            , app::KeyContext::ShapeTool },
    { NULL                   , app::KeyContext::Any }
  };

  using Vec = app::DragVector;

  static struct {
    const char* name;
    const char* userfriendly;
    Vec vector;
  } wheel_actions[] = {
    { ""             , ""                         , Vec(0.0, 0.0) },
    { "Zoom"         , "Zoom"                     , Vec(8.0, 0.0) },
    { "VScroll"      , "Scroll: Vertically"       , Vec(4.0, 0.0) },
    { "HScroll"      , "Scroll: Horizontally"     , Vec(4.0, 0.0) },
    { "FgColor"      , "Color: Foreground Palette Entry" , Vec(8.0, 0.0) },
    { "BgColor"      , "Color: Background Palette Entry" , Vec(8.0, 0.0) },
    { "FgTile"       , "Tile: Foreground Tile Entry" , Vec(8.0, 0.0) },
    { "BgTile"       , "Tile: Background Tile Entry" , Vec(8.0, 0.0) },
    { "Frame"        , "Change Frame"             , Vec(16.0, 0.0) },
    { "BrushSize"    , "Change Brush Size"        , Vec(4.0, 0.0) },
    { "BrushAngle"   , "Change Brush Angle"       , Vec(-4.0, 0.0) },
    { "ToolSameGroup"  , "Change Tool (same group)" , Vec(8.0, 0.0) },
    { "ToolOtherGroup" , "Change Tool"            , Vec(0.0, -8.0) },
    { "Layer"        , "Change Layer"             , Vec(0.0, 8.0) },
    { "InkType"      , "Change Ink Type"          , Vec(0.0, -16.0) },
    { "InkOpacity"   , "Change Ink Opacity"       , Vec(0.0, 1.0) },
    { "LayerOpacity" , "Change Layer Opacity"     , Vec(0.0, 1.0) },
    { "CelOpacity"   , "Change Cel Opacity"       , Vec(0.0, 1.0) },
    { "Alpha"        , "Color: Alpha"             , Vec(4.0, 0.0) },
    { "HslHue"       , "Color: HSL Hue"           , Vec(1.0, 0.0) },
    { "HslSaturation", "Color: HSL Saturation"    , Vec(4.0, 0.0) },
    { "HslLightness" , "Color: HSL Lightness"     , Vec(0.0, 4.0) },
    { "HsvHue"       , "Color: HSV Hue"           , Vec(1.0, 0.0) },
    { "HsvSaturation", "Color: HSV Saturation"    , Vec(4.0, 0.0) },
    { "HsvValue"     , "Color: HSV Value"         , Vec(0.0, 4.0) },
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

  std::string get_user_friendly_string_for_keyaction(app::KeyAction action,
                                                     app::KeyContext context) {
    for (int c=0; actions[c].name; ++c) {
      if (action == actions[c].action &&
          context == actions[c].context)
        return actions[c].userfriendly;
    }
    return std::string();
  }

  std::string get_user_friendly_string_for_wheelaction(app::WheelAction wheelAction) {
    int c = int(wheelAction);
    if (c >= int(app::WheelAction::First) &&
        c <= int(app::WheelAction::Last)) {
      return wheel_actions[c].userfriendly;
    }
    else
      return std::string();
  }

  void erase_accel(app::KeySourceAccelList& kvs,
                   const app::KeySource source,
                   const ui::Accelerator& accel) {
    for (auto it=kvs.begin(); it!=kvs.end(); ) {
      auto& kv = *it;
      if (kv.first == source &&
          kv.second == accel) {
        it = kvs.erase(it);
      }
      else
        ++it;
    }
  }

  void erase_accels(app::KeySourceAccelList& kvs,
                    const app::KeySource source) {
    for (auto it=kvs.begin(); it!=kvs.end(); ) {
      auto& kv = *it;
      if (kv.first == source) {
        it = kvs.erase(it);
      }
      else
        ++it;
    }
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
    for (int c=int(app::WheelAction::First);
            c<=int(app::WheelAction::Last); ++c) {
      if (from == wheel_actions[c].name)
        return (app::WheelAction)c;
    }
    return app::WheelAction::None;
  }

  template<> std::string convert_to(const app::WheelAction& from) {
    int c = int(from);
    if (c >= int(app::WheelAction::First) &&
        c <= int(app::WheelAction::Last)) {
      return wheel_actions[c].name;
    }
    else
      return std::string();
  }

  template<> app::KeyContext convert_to(const std::string& from) {
    for (int c=0; contexts[c].name; ++c) {
      if (from == contexts[c].name)
        return contexts[c].context;
    }
    return app::KeyContext::Any;
  }

  template<> std::string convert_to(const app::KeyContext& from) {
    for (int c=0; contexts[c].name; ++c) {
      if (from == contexts[c].context)
        return contexts[c].name;
    }
    return std::string();
  }

} // namespace base

namespace app {

using namespace ui;

//////////////////////////////////////////////////////////////////////
// Key

Key::Key(const Key& k)
  : m_type(k.m_type)
  , m_adds(k.m_adds)
  , m_dels(k.m_dels)
  , m_keycontext(k.m_keycontext)
{
  switch (m_type) {
    case KeyType::Command:
      m_command = k.m_command;
      m_params = k.m_params;
      break;
    case KeyType::Tool:
    case KeyType::Quicktool:
      m_tool = k.m_tool;
      break;
    case KeyType::Action:
      m_action = k.m_action;
      break;
    case KeyType::WheelAction:
      m_action = k.m_action;
      m_wheelAction = k.m_wheelAction;
      break;
    case KeyType::DragAction:
      m_action = k.m_action;
      m_wheelAction = k.m_wheelAction;
      m_dragVector = k.m_dragVector;
      break;
  }
}

Key::Key(Command* command, const Params& params, const KeyContext keyContext)
  : m_type(KeyType::Command)
  , m_keycontext(keyContext)
  , m_command(command)
  , m_params(params)
{
}

Key::Key(const KeyType type, tools::Tool* tool)
  : m_type(type)
  , m_keycontext(KeyContext::Any)
  , m_tool(tool)
{
}

Key::Key(const KeyAction action,
         const KeyContext keyContext)
  : m_type(KeyType::Action)
  , m_keycontext(keyContext)
  , m_action(action)
{
  if (m_keycontext != KeyContext::Any)
    return;

  // Automatic key context
  switch (action) {
    case KeyAction::None:
      m_keycontext = KeyContext::Any;
      break;
    case KeyAction::CopySelection:
    case KeyAction::SnapToGrid:
    case KeyAction::LockAxis:
    case KeyAction::FineControl:
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

Key::Key(const WheelAction wheelAction)
  : m_type(KeyType::WheelAction)
  , m_keycontext(KeyContext::MouseWheel)
  , m_action(KeyAction::None)
  , m_wheelAction(wheelAction)
{
}

// static
KeyPtr Key::MakeDragAction(WheelAction dragAction)
{
  KeyPtr k(new Key(dragAction));
  k->m_type = KeyType::DragAction;
  k->m_keycontext = KeyContext::Any;
  k->m_dragVector = wheel_actions[(int)dragAction].vector;
  return k;
}

const ui::Accelerators& Key::accels() const
{
  if (!m_accels) {
    m_accels = std::make_unique<ui::Accelerators>();

    // Add default keys
    for (const auto& kv : m_adds) {
      if (kv.first == KeySource::Original)
        m_accels->add(kv.second);
    }

    // Delete/add extension-defined keys
    for (const auto& kv : m_dels) {
      if (kv.first == KeySource::ExtensionDefined)
        m_accels->remove(kv.second);
      else {
        ASSERT(kv.first != KeySource::Original);
      }
    }
    for (const auto& kv : m_adds) {
      if (kv.first == KeySource::ExtensionDefined)
        m_accels->add(kv.second);
    }

    // Delete/add user-defined keys
    for (const auto& kv : m_dels) {
      if (kv.first == KeySource::UserDefined)
        m_accels->remove(kv.second);
    }
    for (const auto& kv : m_adds) {
      if (kv.first == KeySource::UserDefined)
        m_accels->add(kv.second);
    }
  }
  return *m_accels;
}

void Key::add(const ui::Accelerator& accel,
              const KeySource source,
              KeyboardShortcuts& globalKeys)
{
  m_adds.emplace_back(source, accel);
  m_accels.reset();

  // Remove the accelerator from other commands
  if (source == KeySource::ExtensionDefined ||
      source == KeySource::UserDefined) {
    erase_accel(m_dels, source, accel);

    globalKeys.disableAccel(accel, source, m_keycontext, this);
  }
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

bool Key::hasUserDefinedAccels() const
{
  for (const auto& kv : m_adds) {
    if (kv.first == KeySource::UserDefined)
      return true;
  }
  return false;
}

void Key::disableAccel(const ui::Accelerator& accel,
                       const KeySource source)
{
  // It doesn't make sense that the default keyboard shortcuts file
  // (gui.xml) removes some accelerator.
  ASSERT(source != KeySource::Original);

  erase_accel(m_adds, source, accel);
  erase_accel(m_dels, source, accel);

  m_dels.emplace_back(source, accel);
  m_accels.reset();
}

void Key::reset()
{
  erase_accels(m_adds, KeySource::UserDefined);
  erase_accels(m_dels, KeySource::UserDefined);
  m_accels.reset();
}

void Key::copyOriginalToUser()
{
  // Erase all user-defined keys
  erase_accels(m_adds, KeySource::UserDefined);
  erase_accels(m_dels, KeySource::UserDefined);

  // Then copy all original & extension-defined keys as user-defined
  auto copy = m_adds;
  for (const auto& kv : copy)
    m_adds.emplace_back(KeySource::UserDefined, kv.second);
  m_accels.reset();
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
      return get_user_friendly_string_for_keyaction(m_action,
                                                    m_keycontext);
    case KeyType::WheelAction:
    case KeyType::DragAction:
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
            key->disableAccel(accel, source);
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
    bool removed = bool_attr(xmlKey, "removed", false);

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->tool(tool);
        if (key && tool_key) {
          LOG(VERBOSE, "KEYS: Shortcut for tool %s: %s\n", tool_id, tool_key);
          Accelerator accel(tool_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel, source);
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
    bool removed = bool_attr(xmlKey, "removed", false);

    if (tool_id) {
      tools::Tool* tool = App::instance()->toolBox()->getToolById(tool_id);
      if (tool) {
        KeyPtr key = this->quicktool(tool);
        if (key && tool_key) {
          LOG(VERBOSE, "KEYS: Shortcut for quicktool %s: %s\n", tool_id, tool_key);
          Accelerator accel(tool_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel, source);
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
          LOG(VERBOSE, "KEYS: Shortcut for action %s/%s: %s\n", action_id,
              (keycontextstr ? keycontextstr: "Any"), action_key);
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel, source);
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
    bool removed = bool_attr(xmlKey, "removed", false);

    if (action_id) {
      WheelAction action = base::convert_to<WheelAction, std::string>(action_id);
      if (action != WheelAction::None) {
        KeyPtr key = this->wheelAction(action);
        if (key && action_key) {
          LOG(VERBOSE, "KEYS: Shortcut for wheel action %s: %s\n", action_id, action_key);
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel, source);
        }
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts to simulate mouse wheel actions
  // <keyboard><drag><key>
  xmlKey = handle
    .FirstChild("drag")
    .FirstChild("key").ToElement();
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
          Accelerator accel(action_key);

          if (!removed)
            key->add(accel, source, *this);
          else
            key->disableAccel(accel, source);
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
  TiXmlElement drag("drag");

  keyboard.SetAttribute("version", XML_KEYBOARD_FILE_VERSION);

  exportKeys(commands, KeyType::Command);
  exportKeys(tools, KeyType::Tool);
  exportKeys(quicktools, KeyType::Quicktool);
  exportKeys(actions, KeyType::Action);
  exportKeys(wheel, KeyType::WheelAction);
  exportKeys(drag, KeyType::DragAction);

  keyboard.InsertEndChild(commands);
  keyboard.InsertEndChild(tools);
  keyboard.InsertEndChild(quicktools);
  keyboard.InsertEndChild(actions);
  keyboard.InsertEndChild(wheel);
  keyboard.InsertEndChild(drag);

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

    for (const auto& kv : key->delsKeys())
      if (kv.first == KeySource::UserDefined)
        exportAccel(parent, key.get(), kv.second, true);

    for (const auto& kv : key->addsKeys())
      if (kv.first == KeySource::UserDefined)
        exportAccel(parent, key.get(), kv.second, false);
  }
}

void KeyboardShortcuts::exportAccel(TiXmlElement& parent, const Key* key, const ui::Accelerator& accel, bool removed)
{
  TiXmlElement elem("key");

  switch (key->type()) {

    case KeyType::Command: {
      elem.SetAttribute("command", key->command()->id().c_str());

      if (key->keycontext() != KeyContext::Any) {
        elem.SetAttribute("context",
                          base::convert_to<std::string>(key->keycontext()).c_str());
      }

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
      if (key->keycontext() != KeyContext::Any)
        elem.SetAttribute("context",
                          base::convert_to<std::string>(key->keycontext()).c_str());
      break;

    case KeyType::WheelAction:
      elem.SetAttribute("action",
                        base::convert_to<std::string>(key->wheelAction()));
      break;

    case KeyType::DragAction:
      elem.SetAttribute("action",
                        base::convert_to<std::string>(key->wheelAction()));
      elem.SetAttribute("vector",
                        fmt::format("{},{}",
                                    key->dragVector().x,
                                    key->dragVector().y));
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

KeyPtr KeyboardShortcuts::action(KeyAction action,
                                 KeyContext keyContext)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::Action &&
        key->action() == action &&
        key->keycontext() == keyContext) {
      return key;
    }
  }

  KeyPtr key = std::make_shared<Key>(action, keyContext);
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

KeyPtr KeyboardShortcuts::dragAction(WheelAction dragAction)
{
  for (KeyPtr& key : m_keys) {
    if (key->type() == KeyType::DragAction &&
        key->wheelAction() == dragAction) {
      return key;
    }
  }

  KeyPtr key = Key::MakeDragAction(dragAction);
  m_keys.push_back(key);
  return key;
}

void KeyboardShortcuts::disableAccel(const ui::Accelerator& accel,
                                     const KeySource source,
                                     const KeyContext keyContext,
                                     const Key* newKey)
{
  for (KeyPtr& key : m_keys) {
    if (key.get() != newKey &&
        key->keycontext() == keyContext &&
        key->hasAccel(accel) &&
        // Tools can contain the same keyboard shortcut
        (key->type() != KeyType::Tool ||
         newKey == nullptr ||
         newKey->type() != KeyType::Tool) &&
        // DragActions can share the same keyboard shortcut (e.g. to
        // change different values using different DragVectors)
        (key->type() != KeyType::DragAction ||
         newKey == nullptr ||
         newKey->type() != KeyType::DragAction)) {
      key->disableAccel(accel, source);
    }
  }
}

KeyContext KeyboardShortcuts::getCurrentKeyContext()
{
  Doc* doc = UIContext::instance()->activeDocument();
  if (doc &&
      doc->isMaskVisible() &&
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
      App::instance()->activeToolManager()
        ->selectedTool()->getInk(0)->isSelection())
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

Keys KeyboardShortcuts::getDragActionsFromKeyMessage(const KeyContext context,
                                                     const ui::Message* msg)
{
  KeyPtr bestKey = nullptr;
  Keys keys;
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::DragAction) {
      const ui::Accelerator* accel = key->isPressed(msg, *this);
      if (accel) {
        keys.push_back(key);
      }
    }
  }
  return keys;
}

bool KeyboardShortcuts::hasMouseWheelCustomization() const
{
  for (const KeyPtr& key : m_keys) {
    if (key->type() == KeyType::WheelAction &&
        key->hasUserDefinedAccels())
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
  for (int action=int(WheelAction::First);
          action<=int(WheelAction::Last); ++action) {
    // Wheel actions
    auto it = std::find_if(
      m_keys.begin(), m_keys.end(),
      [action](const KeyPtr& key) -> bool {
        return
          key->type() == KeyType::WheelAction &&
          key->wheelAction() == (WheelAction)action;
      });
    if (it == m_keys.end()) {
      KeyPtr key = std::make_shared<Key>((WheelAction)action);
      m_keys.push_back(key);
    }

    // Drag actions
    it = std::find_if(
      m_keys.begin(), m_keys.end(),
      [action](const KeyPtr& key) -> bool {
        return
          key->type() == KeyType::DragAction &&
          key->wheelAction() == (WheelAction)action;
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
