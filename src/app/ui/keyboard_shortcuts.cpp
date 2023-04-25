// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
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
#include "app/ui/key.h"
#include "app/ui/timeline/timeline.h"
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

#define I18N_KEY(a) app::Strings::keyboard_shortcuts_##a()

namespace {

  struct KeyShortcutAction {
    const char* name;
    std::string userfriendly;
    app::KeyAction action;
    app::KeyContext context;
  };

  static std::vector<KeyShortcutAction> g_actions;

  static const std::vector<KeyShortcutAction>& actions() {
    if (g_actions.empty()) {
      g_actions = std::vector<KeyShortcutAction> {
        { "CopySelection"       , I18N_KEY(copy_selection)             , app::KeyAction::CopySelection, app::KeyContext::TranslatingSelection },
        { "SnapToGrid"          , I18N_KEY(snap_to_grid)               , app::KeyAction::SnapToGrid, app::KeyContext::TranslatingSelection },
        { "LockAxis"            , I18N_KEY(lock_axis)                  , app::KeyAction::LockAxis, app::KeyContext::TranslatingSelection },
        { "FineControl"         , I18N_KEY(fine_translating)           , app::KeyAction::FineControl , app::KeyContext::TranslatingSelection },
        { "MaintainAspectRatio" , I18N_KEY(maintain_aspect_ratio)      , app::KeyAction::MaintainAspectRatio, app::KeyContext::ScalingSelection },
        { "ScaleFromCenter"     , I18N_KEY(scale_from_center)          , app::KeyAction::ScaleFromCenter, app::KeyContext::ScalingSelection },
        { "FineControl"         , I18N_KEY(fine_scaling)               , app::KeyAction::FineControl , app::KeyContext::ScalingSelection },
        { "AngleSnap"           , I18N_KEY(angle_snap)                 , app::KeyAction::AngleSnap, app::KeyContext::RotatingSelection },
        { "AddSelection"        , I18N_KEY(add_selection)              , app::KeyAction::AddSelection, app::KeyContext::SelectionTool },
        { "SubtractSelection"   , I18N_KEY(subtract_selection)         , app::KeyAction::SubtractSelection, app::KeyContext::SelectionTool },
        { "IntersectSelection"  , I18N_KEY(intersect_selection)        , app::KeyAction::IntersectSelection, app::KeyContext::SelectionTool },
        { "AutoSelectLayer"     , I18N_KEY(auto_select_layer)          , app::KeyAction::AutoSelectLayer, app::KeyContext::MoveTool },
        { "StraightLineFromLastPoint", I18N_KEY(line_from_last_point)  , app::KeyAction::StraightLineFromLastPoint, app::KeyContext::FreehandTool },
        { "AngleSnapFromLastPoint", I18N_KEY(angle_from_last_point)    , app::KeyAction::AngleSnapFromLastPoint, app::KeyContext::FreehandTool },
        { "MoveOrigin"          , I18N_KEY(move_origin)                , app::KeyAction::MoveOrigin, app::KeyContext::ShapeTool },
        { "SquareAspect"        , I18N_KEY(square_aspect)              , app::KeyAction::SquareAspect, app::KeyContext::ShapeTool },
        { "DrawFromCenter"      , I18N_KEY(draw_from_center)           , app::KeyAction::DrawFromCenter, app::KeyContext::ShapeTool },
        { "RotateShape"         , I18N_KEY(rotate_shape)               , app::KeyAction::RotateShape, app::KeyContext::ShapeTool },
        { "LeftMouseButton"     , I18N_KEY(trigger_left_mouse_button)  , app::KeyAction::LeftMouseButton, app::KeyContext::Any },
        { "RightMouseButton"    , I18N_KEY(trigger_right_mouse_button) , app::KeyAction::RightMouseButton, app::KeyContext::Any }
      };
    }
    return g_actions;
  }

  static struct {
    const char* name;
    app::KeyContext context;
  } g_contexts[] = {
    { ""                     , app::KeyContext::Any },
    { "Normal"               , app::KeyContext::Normal },
    { "Selection"            , app::KeyContext::SelectionTool },
    { "TranslatingSelection" , app::KeyContext::TranslatingSelection },
    { "ScalingSelection"     , app::KeyContext::ScalingSelection },
    { "RotatingSelection"    , app::KeyContext::RotatingSelection },
    { "MoveTool"             , app::KeyContext::MoveTool },
    { "FreehandTool"         , app::KeyContext::FreehandTool },
    { "ShapeTool"            , app::KeyContext::ShapeTool },
    { "FramesSelection"      , app::KeyContext::FramesSelection },
    { NULL                   , app::KeyContext::Any }
  };

  using Vec = app::DragVector;

  struct KeyShortcutWheelAction {
    const char* name;
    const std::string userfriendly;
    Vec vector;
  };

  static std::vector<KeyShortcutWheelAction> g_wheel_actions;

  static const std::vector<KeyShortcutWheelAction>& wheel_actions() {
    if (g_wheel_actions.empty()) {
      g_wheel_actions = std::vector<KeyShortcutWheelAction> {
        { ""              , ""                               , Vec(0.0, 0.0) },
        { "Zoom"          , I18N_KEY(zoom)                   , Vec(8.0, 0.0) },
        { "VScroll"       , I18N_KEY(scroll_vertically)      , Vec(4.0, 0.0) },
        { "HScroll"       , I18N_KEY(scroll_horizontally)    , Vec(4.0, 0.0) },
        { "FgColor"       , I18N_KEY(fg_color)               , Vec(8.0, 0.0) },
        { "BgColor"       , I18N_KEY(bg_color)               , Vec(8.0, 0.0) },
        { "FgTile"        , I18N_KEY(fg_tile)                , Vec(8.0, 0.0) },
        { "BgTile"        , I18N_KEY(bg_tile)                , Vec(8.0, 0.0) },
        { "Frame"         , I18N_KEY(change_frame)           , Vec(16.0, 0.0) },
        { "BrushSize"     , I18N_KEY(change_brush_size)      , Vec(4.0, 0.0) },
        { "BrushAngle"    , I18N_KEY(change_brush_angle)     , Vec(-4.0, 0.0) },
        { "ToolSameGroup" , I18N_KEY(change_tool_same_group) , Vec(8.0, 0.0) },
        { "ToolOtherGroup", I18N_KEY(change_tool)            , Vec(0.0, -8.0) },
        { "Layer"         , I18N_KEY(change_layer)           , Vec(0.0, 8.0) },
        { "InkType"       , I18N_KEY(change_ink_type)        , Vec(0.0, -16.0) },
        { "InkOpacity"    , I18N_KEY(change_ink_opacity)     , Vec(0.0, 1.0) },
        { "LayerOpacity"  , I18N_KEY(change_layer_opacity)   , Vec(0.0, 1.0) },
        { "CelOpacity"    , I18N_KEY(change_cel_opacity)     , Vec(0.0, 1.0) },
        { "Alpha"         , I18N_KEY(color_alpha)            , Vec(4.0, 0.0) },
        { "HslHue"        , I18N_KEY(color_hsl_hue)          , Vec(1.0, 0.0) },
        { "HslSaturation" , I18N_KEY(color_hsl_saturation)   , Vec(4.0, 0.0) },
        { "HslLightness"  , I18N_KEY(color_hsl_lightness)    , Vec(0.0, 4.0) },
        { "HsvHue"        , I18N_KEY(color_hsv_hue)          , Vec(1.0, 0.0) },
        { "HsvSaturation" , I18N_KEY(color_hsv_saturation)   , Vec(4.0, 0.0) },
        { "HsvValue"      , I18N_KEY(color_hsv_value)        , Vec(0.0, 4.0) }
      };
    }
    return g_wheel_actions;
  }

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
    for (const auto& a : actions()) {
      if (action == a.action && context == a.context)
        return a.userfriendly;
    }
    return std::string();
  }

  std::string get_user_friendly_string_for_wheelaction(app::WheelAction wheelAction) {
    int c = int(wheelAction);
    if (c >= int(app::WheelAction::First) &&
        c <= int(app::WheelAction::Last)) {
      return wheel_actions()[c].userfriendly;
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
    for (const auto& a : actions()) {
      if (from == a.name)
        return a.action;
    }
    return app::KeyAction::None;
  }

  template<> std::string convert_to(const app::KeyAction& from) {
    for (const auto& a : actions()) {
      if (from == a.action)
        return a.name;
    }
    return std::string();
  }

  template<> app::WheelAction convert_to(const std::string& from) {
    for (int c=int(app::WheelAction::First);
            c<=int(app::WheelAction::Last); ++c) {
      if (from == wheel_actions()[c].name)
        return (app::WheelAction)c;
    }
    return app::WheelAction::None;
  }

  template<> std::string convert_to(const app::WheelAction& from) {
    int c = int(from);
    if (c >= int(app::WheelAction::First) &&
        c <= int(app::WheelAction::Last)) {
      return wheel_actions()[c].name;
    }
    else
      return std::string();
  }

  template<> app::KeyContext convert_to(const std::string& from) {
    for (int c=0; g_contexts[c].name; ++c) {
      if (from == g_contexts[c].name)
        return g_contexts[c].context;
    }
    return app::KeyContext::Any;
  }

  template<> std::string convert_to(const app::KeyContext& from) {
    for (int c=0; g_contexts[c].name; ++c) {
      if (from == g_contexts[c].context)
        return g_contexts[c].name;
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
  k->m_dragVector = wheel_actions()[(int)dragAction].vector;
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
                                      const KeyboardShortcuts& globalKeys,
                                      const KeyContext keyContext) const
{
  if (auto keyMsg = dynamic_cast<const KeyMessage*>(msg)) {
    for (const Accelerator& accel : accels()) {
      if (accel.isPressed(keyMsg->modifiers(),
                          keyMsg->scancode(),
                          keyMsg->unicodeChar()) &&
          (m_keycontext == KeyContext::Any ||
           m_keycontext == keyContext)) {
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

const ui::Accelerator* Key::isPressed(const Message* msg,
                                      const KeyboardShortcuts& globalKeys) const
{
  return isPressed(msg,
                   globalKeys,
                   globalKeys.getCurrentKeyContext());
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
  ASSERT(Strings::instance());
  Strings::instance()->LanguageChange.connect([]{
    // Clear collections so they are re-constructed with the new language
    g_actions.clear();
    g_wheel_actions.clear();
  });
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

KeyPtr KeyboardShortcuts::command(const char* commandName,
                                  const Params& params,
                                  const KeyContext keyContext) const
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

KeyPtr KeyboardShortcuts::tool(tools::Tool* tool) const
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

KeyPtr KeyboardShortcuts::quicktool(tools::Tool* tool) const
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

KeyPtr KeyboardShortcuts::action(const KeyAction action,
                                 const KeyContext keyContext) const
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

KeyPtr KeyboardShortcuts::wheelAction(const WheelAction wheelAction) const
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

KeyPtr KeyboardShortcuts::dragAction(const WheelAction dragAction) const
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

KeyContext KeyboardShortcuts::getCurrentKeyContext() const
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
        ->selectedTool()->getInk(0)->isSelection()) {
    return KeyContext::SelectionTool;
  }

  auto timeline = App::instance()->timeline();
  if (doc && timeline &&
      !timeline->selectedFrames().empty() &&
      (timeline->range().type() == DocRange::kFrames ||
       timeline->range().type() == DocRange::kCels)) {
    return KeyContext::FramesSelection;
  }

  return KeyContext::Normal;
}

bool KeyboardShortcuts::getCommandFromKeyMessage(const Message* msg, Command** command, Params* params)
{
  const KeyContext contexts[] = {
    getCurrentKeyContext(),
    KeyContext::Normal
  };
  int n = (contexts[0] != contexts[1] ? 2: 1);
  for (int i = 0; i < n; ++i) {
    for (KeyPtr& key : m_keys) {
      if (key->type() == KeyType::Command &&
          key->isPressed(msg, *this, contexts[i])) {
        if (command) *command = key->command();
        if (params) *params = key->params();
        return true;
      }
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
      return I18N_KEY(key_context_normal);
    case KeyContext::SelectionTool:
      return I18N_KEY(key_context_selection);
    case KeyContext::TranslatingSelection:
      return I18N_KEY(key_context_translating_selection);
    case KeyContext::ScalingSelection:
      return I18N_KEY(key_context_scaling_selection);
    case KeyContext::RotatingSelection:
      return I18N_KEY(key_context_rotating_selection);
    case KeyContext::MoveTool:
      return I18N_KEY(key_context_move_tool);
    case KeyContext::FreehandTool:
      return I18N_KEY(key_context_freehand_tool);
    case KeyContext::ShapeTool:
      return I18N_KEY(key_context_shape_tool);
    case KeyContext::FramesSelection:
      return I18N_KEY(key_context_frames_selection);
  }
  return std::string();
}

} // namespace app
