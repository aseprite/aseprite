// Aseprite
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/key.h"

#include "app/commands/command.h"
#include "app/i18n/strings.h"
#include "app/tools/tool.h"
#include "app/ui/keyboard_shortcuts.h"
#include "ui/message.h"
#include "ui/shortcut.h"

#include <string>
#include <vector>

#define I18N_KEY(a) app::Strings::keyboard_shortcuts_##a()

namespace {

struct KeyShortcutAction {
  const char* name;
  std::string userfriendly;
  app::KeyAction action;
  app::KeyContext context;
};

std::vector<KeyShortcutAction> g_actions;

const std::vector<KeyShortcutAction>& actions()
{
  if (g_actions.empty()) {
    g_actions = std::vector<KeyShortcutAction>{
      { "CopySelection",
       I18N_KEY(copy_selection),
       app::KeyAction::CopySelection,
       app::KeyContext::TranslatingSelection },
      { "SnapToGrid",
       I18N_KEY(snap_to_grid),
       app::KeyAction::SnapToGrid,
       app::KeyContext::TranslatingSelection },
      { "LockAxis",
       I18N_KEY(lock_axis),
       app::KeyAction::LockAxis,
       app::KeyContext::TranslatingSelection },
      { "FineControl",
       I18N_KEY(fine_translating),
       app::KeyAction::FineControl,
       app::KeyContext::TranslatingSelection },
      { "MaintainAspectRatio",
       I18N_KEY(maintain_aspect_ratio),
       app::KeyAction::MaintainAspectRatio,
       app::KeyContext::ScalingSelection     },
      { "ScaleFromCenter",
       I18N_KEY(scale_from_center),
       app::KeyAction::ScaleFromCenter,
       app::KeyContext::ScalingSelection     },
      { "FineControl",
       I18N_KEY(fine_scaling),
       app::KeyAction::FineControl,
       app::KeyContext::ScalingSelection     },
      { "AngleSnap",
       I18N_KEY(angle_snap),
       app::KeyAction::AngleSnap,
       app::KeyContext::RotatingSelection    },
      { "AddSelection",
       I18N_KEY(add_selection),
       app::KeyAction::AddSelection,
       app::KeyContext::SelectionTool        },
      { "SubtractSelection",
       I18N_KEY(subtract_selection),
       app::KeyAction::SubtractSelection,
       app::KeyContext::SelectionTool        },
      { "IntersectSelection",
       I18N_KEY(intersect_selection),
       app::KeyAction::IntersectSelection,
       app::KeyContext::SelectionTool        },
      { "AutoSelectLayer",
       I18N_KEY(auto_select_layer),
       app::KeyAction::AutoSelectLayer,
       app::KeyContext::MoveTool             },
      { "StraightLineFromLastPoint",
       I18N_KEY(line_from_last_point),
       app::KeyAction::StraightLineFromLastPoint,
       app::KeyContext::FreehandTool         },
      { "AngleSnapFromLastPoint",
       I18N_KEY(angle_from_last_point),
       app::KeyAction::AngleSnapFromLastPoint,
       app::KeyContext::FreehandTool         },
      { "MoveOrigin",
       I18N_KEY(move_origin),
       app::KeyAction::MoveOrigin,
       app::KeyContext::ShapeTool            },
      { "SquareAspect",
       I18N_KEY(square_aspect),
       app::KeyAction::SquareAspect,
       app::KeyContext::ShapeTool            },
      { "DrawFromCenter",
       I18N_KEY(draw_from_center),
       app::KeyAction::DrawFromCenter,
       app::KeyContext::ShapeTool            },
      { "RotateShape",
       I18N_KEY(rotate_shape),
       app::KeyAction::RotateShape,
       app::KeyContext::ShapeTool            },
      { "CornerRadius",
       I18N_KEY(corner_radius),
       app::KeyAction::CornerRadius,
       app::KeyContext::ShapeTool            },
      { "LeftMouseButton",
       I18N_KEY(trigger_left_mouse_button),
       app::KeyAction::LeftMouseButton,
       app::KeyContext::Any                  },
      { "RightMouseButton",
       I18N_KEY(trigger_right_mouse_button),
       app::KeyAction::RightMouseButton,
       app::KeyContext::Any                  }
    };
  }
  return g_actions;
}

struct {
  const char* name;
  app::KeyContext context;
} g_contexts[] = {
  { "",                     app::KeyContext::Any                  },
  { "Normal",               app::KeyContext::Normal               },
  { "Selection",            app::KeyContext::SelectionTool        },
  { "TranslatingSelection", app::KeyContext::TranslatingSelection },
  { "ScalingSelection",     app::KeyContext::ScalingSelection     },
  { "RotatingSelection",    app::KeyContext::RotatingSelection    },
  { "MoveTool",             app::KeyContext::MoveTool             },
  { "FreehandTool",         app::KeyContext::FreehandTool         },
  { "ShapeTool",            app::KeyContext::ShapeTool            },
  { "FramesSelection",      app::KeyContext::FramesSelection      },
  { "Transformation",       app::KeyContext::Transformation       },
  { NULL,                   app::KeyContext::Any                  }
};

using Vec = app::DragVector;

struct KeyShortcutWheelAction {
  const char* name;
  const std::string userfriendly;
  Vec vector;
};

std::vector<KeyShortcutWheelAction> g_wheel_actions;

const std::vector<KeyShortcutWheelAction>& wheel_actions()
{
  if (g_wheel_actions.empty()) {
    g_wheel_actions = std::vector<KeyShortcutWheelAction>{
      { "",               "",                               Vec(0.0,  0.0)   },
      { "Zoom",           I18N_KEY(zoom),                   Vec(8.0,  0.0)   },
      { "VScroll",        I18N_KEY(scroll_vertically),      Vec(4.0,  0.0)   },
      { "HScroll",        I18N_KEY(scroll_horizontally),    Vec(4.0,  0.0)   },
      { "FgColor",        I18N_KEY(fg_color),               Vec(8.0,  0.0)   },
      { "BgColor",        I18N_KEY(bg_color),               Vec(8.0,  0.0)   },
      { "FgTile",         I18N_KEY(fg_tile),                Vec(8.0,  0.0)   },
      { "BgTile",         I18N_KEY(bg_tile),                Vec(8.0,  0.0)   },
      { "Frame",          I18N_KEY(change_frame),           Vec(16.0, 0.0)   },
      { "BrushSize",      I18N_KEY(change_brush_size),      Vec(4.0,  0.0)   },
      { "BrushAngle",     I18N_KEY(change_brush_angle),     Vec(-4.0, 0.0)   },
      { "ToolSameGroup",  I18N_KEY(change_tool_same_group), Vec(8.0,  0.0)   },
      { "ToolOtherGroup", I18N_KEY(change_tool),            Vec(0.0,  -8.0)  },
      { "Layer",          I18N_KEY(change_layer),           Vec(0.0,  8.0)   },
      { "InkType",        I18N_KEY(change_ink_type),        Vec(0.0,  -16.0) },
      { "InkOpacity",     I18N_KEY(change_ink_opacity),     Vec(0.0,  1.0)   },
      { "LayerOpacity",   I18N_KEY(change_layer_opacity),   Vec(0.0,  1.0)   },
      { "CelOpacity",     I18N_KEY(change_cel_opacity),     Vec(0.0,  1.0)   },
      { "Alpha",          I18N_KEY(color_alpha),            Vec(4.0,  0.0)   },
      { "HslHue",         I18N_KEY(color_hsl_hue),          Vec(1.0,  0.0)   },
      { "HslSaturation",  I18N_KEY(color_hsl_saturation),   Vec(4.0,  0.0)   },
      { "HslLightness",   I18N_KEY(color_hsl_lightness),    Vec(0.0,  4.0)   },
      { "HsvHue",         I18N_KEY(color_hsv_hue),          Vec(1.0,  0.0)   },
      { "HsvSaturation",  I18N_KEY(color_hsv_saturation),   Vec(4.0,  0.0)   },
      { "HsvValue",       I18N_KEY(color_hsv_value),        Vec(0.0,  4.0)   }
    };
  }
  return g_wheel_actions;
}

std::string get_user_friendly_string_for_keyaction(app::KeyAction action, app::KeyContext context)
{
  for (const auto& a : actions()) {
    if (action == a.action && context == a.context)
      return a.userfriendly;
  }
  return {};
}

std::string get_user_friendly_string_for_wheelaction(app::WheelAction wheelAction)
{
  const int c = int(wheelAction);
  if (c >= int(app::WheelAction::First) && c <= int(app::WheelAction::Last))
    return wheel_actions()[c].userfriendly;
  return {};
}

void erase_shortcut(app::AppShortcuts& kvs,
                    const app::KeySource source,
                    const ui::Shortcut& shortcut)
{
  for (auto it = kvs.begin(); it != kvs.end();) {
    auto& kv = *it;
    if (kv.source() == source && kv.shortcut() == shortcut)
      it = kvs.erase(it);
    else
      ++it;
  }
}

void erase_shortcuts(app::AppShortcuts& kvs, const app::KeySource source)
{
  for (auto it = kvs.begin(); it != kvs.end();) {
    auto& kv = *it;
    if (kv.source() == source)
      it = kvs.erase(it);
    else
      ++it;
  }
}

} // anonymous namespace

namespace base {

template<>
app::KeyAction convert_to(const std::string& from)
{
  for (const auto& a : actions()) {
    if (from == a.name)
      return a.action;
  }
  return app::KeyAction::None;
}

template<>
std::string convert_to(const app::KeyAction& from)
{
  for (const auto& a : actions()) {
    if (from == a.action)
      return a.name;
  }
  return {};
}

template<>
app::WheelAction convert_to(const std::string& from)
{
  for (int c = int(app::WheelAction::First); c <= int(app::WheelAction::Last); ++c) {
    if (from == wheel_actions()[c].name)
      return (app::WheelAction)c;
  }
  return app::WheelAction::None;
}

template<>
std::string convert_to(const app::WheelAction& from)
{
  const int c = int(from);
  if (c >= int(app::WheelAction::First) && c <= int(app::WheelAction::Last))
    return wheel_actions()[c].name;
  return {};
}

template<>
app::KeyContext convert_to(const std::string& from)
{
  for (int c = 0; g_contexts[c].name; ++c) {
    if (from == g_contexts[c].name)
      return g_contexts[c].context;
  }
  return app::KeyContext::Any;
}

template<>
std::string convert_to(const app::KeyContext& from)
{
  for (int c = 0; g_contexts[c].name; ++c) {
    if (from == g_contexts[c].context)
      return g_contexts[c].name;
  }
  return {};
}

} // namespace base

namespace app {

using namespace ui;

bool AppShortcut::fitsBetterThan(const KeyContext currentContext,
                                 const KeyContext thisShortcutContext,
                                 const KeyContext otherShortcutContext,
                                 const AppShortcut& otherShortcut) const
{
  // Better context in the same source
  if (otherShortcut.source() == this->source() && otherShortcutContext != currentContext &&
      thisShortcutContext == currentContext)
    return true;

  // Better key source/level: User-defined > Extension-defined > App-defined
  if (int(source()) > int(otherShortcut.source()) && (thisShortcutContext == currentContext ||
                                                      // User-defined "Any" context overwrites all
                                                      // app-defined context
                                                      thisShortcutContext == KeyContext::Any))
    return true;

  // Normal > SelectionTool > Transformation
  if ((currentContext == KeyContext::Transformation &&
       otherShortcutContext != KeyContext::Transformation &&
       thisShortcutContext == KeyContext::SelectionTool))
    return true;

  return false;
}

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
    case KeyType::Quicktool: m_tool = k.m_tool; break;
    case KeyType::Action:    m_action = k.m_action; break;
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

Key::Key(const KeyAction action, const KeyContext keyContext)
  : m_type(KeyType::Action)
  , m_keycontext(keyContext)
  , m_action(action)
{
  if (m_keycontext != KeyContext::Any)
    return;

  // Automatic key context
  switch (action) {
    case KeyAction::None:                      m_keycontext = KeyContext::Any; break;
    case KeyAction::CopySelection:
    case KeyAction::SnapToGrid:
    case KeyAction::LockAxis:
    case KeyAction::FineControl:               m_keycontext = KeyContext::TranslatingSelection; break;
    case KeyAction::AngleSnap:                 m_keycontext = KeyContext::RotatingSelection; break;
    case KeyAction::MaintainAspectRatio:
    case KeyAction::ScaleFromCenter:           m_keycontext = KeyContext::ScalingSelection; break;
    case KeyAction::AddSelection:
    case KeyAction::SubtractSelection:
    case KeyAction::IntersectSelection:        m_keycontext = KeyContext::SelectionTool; break;
    case KeyAction::AutoSelectLayer:           m_keycontext = KeyContext::MoveTool; break;
    case KeyAction::StraightLineFromLastPoint:
    case KeyAction::AngleSnapFromLastPoint:    m_keycontext = KeyContext::FreehandTool; break;
    case KeyAction::MoveOrigin:
    case KeyAction::SquareAspect:
    case KeyAction::DrawFromCenter:
    case KeyAction::RotateShape:               m_keycontext = KeyContext::ShapeTool; break;
    case KeyAction::LeftMouseButton:
    case KeyAction::RightMouseButton:          m_keycontext = KeyContext::Any; break;
    case KeyAction::CornerRadius:              m_keycontext = KeyContext::ShapeTool; break;
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

const AppShortcuts& Key::shortcuts() const
{
  if (!m_shortcuts) {
    m_shortcuts = std::make_unique<AppShortcuts>();

    // Add default keys
    for (const auto& kv : m_adds) {
      if (kv.source() == KeySource::Original)
        m_shortcuts->add(kv);
    }

    // Delete/add extension-defined keys
    for (const auto& kv : m_dels) {
      if (kv.source() == KeySource::ExtensionDefined)
        m_shortcuts->remove(kv);
      else {
        ASSERT(kv.source() != KeySource::Original);
      }
    }
    for (const auto& kv : m_adds) {
      if (kv.source() == KeySource::ExtensionDefined)
        m_shortcuts->add(kv);
    }

    // Delete/add user-defined keys
    for (const auto& kv : m_dels) {
      if (kv.source() == KeySource::UserDefined)
        m_shortcuts->remove(kv);
    }
    for (const auto& kv : m_adds) {
      if (kv.source() == KeySource::UserDefined)
        m_shortcuts->add(kv);
    }
  }
  return *m_shortcuts;
}

void Key::add(const ui::Shortcut& shortcut, const KeySource source, KeyboardShortcuts& globalKeys)
{
  AppShortcut appShortcut(source, shortcut);

  // For tools and mouse actions, we prefer "Space" as a modifier
  // instead of a "trigger action". Without this, "Space+mouse down"
  // doesn't start the Hand tool operation correctly (instead of
  // scrolling with the Han dtool it will start painting with the
  // Pencil tool).
  if (!m_command)
    appShortcut.preferAsModifierOnly();

  m_adds.push_back(appShortcut);
  m_shortcuts.reset();

  // Remove the shortcut from other commands
  if (source == KeySource::ExtensionDefined || source == KeySource::UserDefined) {
    erase_shortcut(m_dels, source, shortcut);

    globalKeys.disableShortcut(shortcut, source, m_keycontext, this);
  }
}

bool Key::fitsContext(const KeyContext keyContext) const
{
  // This key is for any context
  if (m_keycontext == KeyContext::Any)
    return true;

  // This key is for the same context
  if (m_keycontext == keyContext)
    return true;

  // Use Normal or SelectionTool keys in Transformation context
  if (keyContext == KeyContext::Transformation &&
      (m_keycontext == KeyContext::SelectionTool || m_keycontext == KeyContext::Normal))
    return true;

  // Use Normal keys in SelectionTool or FramesSelection contexts
  if ((keyContext == KeyContext::SelectionTool || keyContext == KeyContext::FramesSelection) &&
      (m_keycontext == KeyContext::Normal))
    return true;

  return false;
}

const AppShortcut* Key::isPressed(const Message* msg, const KeyContext keyContext) const
{
  const AppShortcut* best = nullptr;

  if (const auto* keyMsg = dynamic_cast<const KeyMessage*>(msg)) {
    if (fitsContext(keyContext)) {
      const bool pressed = (msg->type() != kKeyUpMessage);
      const auto pressedScancode = (pressed ? keyMsg->scancode() : kKeyNil);
      const auto pressedUnicodeChar = (pressed ? keyMsg->unicodeChar() : 0);

      for (const AppShortcut& shortcut : shortcuts()) {
        if (shortcut.isPressed(keyMsg->modifiers(), pressedScancode, pressedUnicodeChar) &&
            (!best || shortcut.fitsBetterThan(keyContext, keycontext(), keycontext(), *best))) {
          best = &shortcut;
        }
      }
    }
  }
  else if (const auto* mouseMsg = dynamic_cast<const MouseMessage*>(msg)) {
    if (m_keycontext == KeyContext::Any ||
        // TODO we could have multiple mouse wheel key-context,
        // like "sprite editor" context, or "timeline" context,
        // etc.
        m_keycontext == KeyContext::MouseWheel) {
      const bool pressed = (msg->type() != kMouseUpMessage);
      const auto pressedMouseButton = (pressed ? mouseMsg->button() : kButtonNone);

      for (const AppShortcut& shortcut : shortcuts()) {
        if ((shortcut.modifiers() == mouseMsg->modifiers()) &&
            ((shortcut.scancode() == kKeyNil && shortcut.unicodeChar() == 0) ||
             (shortcut.modifiers() == kKeySpaceModifier && shortcut.scancode() == kKeySpace &&
              shortcut.unicodeChar() == ' ')) &&
            (shortcut.mouseButton() == kButtonNone ||
             (pressedMouseButton != kButtonNone && shortcut.mouseButton() == mouseMsg->button())) &&
            (!best || shortcut.fitsBetterThan(keyContext, keycontext(), keycontext(), *best))) {
          best = &shortcut;
        }
      }
    }
  }
  return best;
}

const AppShortcut* Key::isPressed(const Message* msg) const
{
  if (!msg)
    return isPressed();
  return isPressed(msg, KeyboardShortcuts::getCurrentKeyContext());
}

const AppShortcut* Key::isPressed() const
{
  const KeyContext keyContext = KeyboardShortcuts::getCurrentKeyContext();
  const AppShortcut* best = nullptr;
  if (fitsContext(keyContext)) {
    for (const AppShortcut& shortcut : shortcuts()) {
      if (shortcut.isPressed() &&
          (!best || shortcut.fitsBetterThan(keyContext, keycontext(), keycontext(), *best))) {
        best = &shortcut;
      }
    }
  }
  return best;
}

bool Key::isLooselyPressed() const
{
  const auto& ss = this->shortcuts();
  return std::any_of(ss.begin(), ss.end(), [](const AppShortcut& shortcut) {
    return shortcut.isLooselyPressed();
  });
}

bool Key::isCommandListed() const
{
  return type() == KeyType::Command && command()->isListed(params());
}

bool Key::hasShortcut(const ui::Shortcut& shortcut) const
{
  return shortcuts().has(AppShortcut(
    // KeySource is not used in has()
    KeySource::Original,
    shortcut));
}

bool Key::hasUserDefinedShortcuts() const
{
  return std::any_of(m_adds.begin(), m_adds.end(), [](const auto& kv) {
    return (kv.source() == KeySource::UserDefined);
  });
}

void Key::disableShortcut(const ui::Shortcut& shortcut, const KeySource source)
{
  // It doesn't make sense that the default keyboard shortcuts file
  // (gui.xml) removes some shortcut.
  ASSERT(source != KeySource::Original);

  erase_shortcut(m_adds, source, shortcut);
  erase_shortcut(m_dels, source, shortcut);

  m_dels.push_back(AppShortcut(source, shortcut));
  m_shortcuts.reset();
}

void Key::reset()
{
  erase_shortcuts(m_adds, KeySource::UserDefined);
  erase_shortcuts(m_dels, KeySource::UserDefined);
  m_shortcuts.reset();
}

void Key::copyOriginalToUser()
{
  // Erase all user-defined keys
  erase_shortcuts(m_adds, KeySource::UserDefined);
  erase_shortcuts(m_dels, KeySource::UserDefined);

  // Then copy all original & extension-defined keys as user-defined
  auto copy = m_adds;
  for (const auto& kv : copy)
    m_adds.push_back(AppShortcut(KeySource::UserDefined, kv));
  m_shortcuts.reset();
}

std::string Key::triggerString() const
{
  switch (m_type) {
    case KeyType::Command:   m_command->loadParams(m_params); return m_command->friendlyName();
    case KeyType::Tool:
    case KeyType::Quicktool: {
      std::string text = m_tool->getText();
      if (m_type == KeyType::Quicktool)
        text += " (quick)";
      return text;
    }
    case KeyType::Action:      return get_user_friendly_string_for_keyaction(m_action, m_keycontext);
    case KeyType::WheelAction:
    case KeyType::DragAction:  return get_user_friendly_string_for_wheelaction(m_wheelAction);
  }
  return "Unknown";
}

void reset_key_tables_that_depends_on_language()
{
  g_actions.clear();
  g_wheel_actions.clear();
}

std::string key_tooltip(const char* str, const app::Key* key)
{
  std::string res;
  if (str)
    res += str;
  if (key && !key->shortcuts().empty()) {
    res += " (";
    res += key->shortcuts().front().toString();
    res += ")";
  }
  return res;
}

std::string convert_keycontext_to_user_friendly_string(const KeyContext keyctx)
{
  switch (keyctx) {
    case KeyContext::Any:                  return {};
    case KeyContext::Normal:               return I18N_KEY(key_context_normal);
    case KeyContext::SelectionTool:        return I18N_KEY(key_context_selection);
    case KeyContext::TranslatingSelection: return I18N_KEY(key_context_translating_selection);
    case KeyContext::ScalingSelection:     return I18N_KEY(key_context_scaling_selection);
    case KeyContext::RotatingSelection:    return I18N_KEY(key_context_rotating_selection);
    case KeyContext::MoveTool:             return I18N_KEY(key_context_move_tool);
    case KeyContext::FreehandTool:         return I18N_KEY(key_context_freehand_tool);
    case KeyContext::ShapeTool:            return I18N_KEY(key_context_shape_tool);
    case KeyContext::FramesSelection:      return I18N_KEY(key_context_frames_selection);
    case KeyContext::Transformation:       return I18N_KEY(key_context_transformation);
  }
  return {};
}

} // namespace app
