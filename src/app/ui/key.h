// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_KEY_H_INCLUDED
#define APP_UI_KEY_H_INCLUDED
#pragma once

#include "app/commands/params.h"
#include "app/ui/key_context.h"
#include "base/convert_to.h"
#include "ui/accelerator.h"

#include <memory>
#include <vector>

namespace ui {
  class Message;
}

namespace app {
  class Command;
  class KeyboardShortcuts;

  namespace tools {
    class Tool;
  }

  enum class KeySource {
    Original,
    UserDefined
  };

  enum class KeyType {
    Command,
    Tool,
    Quicktool,
    Action,
    WheelAction,
  };

  // TODO This should be called "KeyActionModifier" or something similar
  enum class KeyAction {
    None                      = 0x00000000,
    CopySelection             = 0x00000001,
    SnapToGrid                = 0x00000002,
    AngleSnap                 = 0x00000004,
    MaintainAspectRatio       = 0x00000008,
    LockAxis                  = 0x00000010,
    AddSelection              = 0x00000020,
    SubtractSelection         = 0x00000040,
    IntersectSelection        = 0x00000080,
    AutoSelectLayer           = 0x00000100,
    LeftMouseButton           = 0x00000200,
    RightMouseButton          = 0x00000400,
    StraightLineFromLastPoint = 0x00000800,
    MoveOrigin                = 0x00001000,
    SquareAspect              = 0x00002000,
    DrawFromCenter            = 0x00004000,
    ScaleFromCenter           = 0x00008000,
    AngleSnapFromLastPoint    = 0x00010000,
    RotateShape               = 0x00020000,
  };

  enum class WheelAction {
    None,
    Zoom,
    VScroll,
    HScroll,
    FgColor,
    BgColor,
    Frame,
    BrushSize,
    BrushAngle,
    ToolSameGroup,
    ToolOtherGroup,
    Layer,
    InkOpacity,
    LayerOpacity,
    CelOpacity,
    Alpha,
    HslHue,
    HslSaturation,
    HslLightness,
    HsvHue,
    HsvSaturation,
    HsvValue,

    // Range
    First = Zoom,
    Last = HsvValue,
  };

  inline KeyAction operator&(KeyAction a, KeyAction b) {
    return KeyAction(int(a) & int(b));
  }

  class Key {
  public:
    Key(Command* command, const Params& params, KeyContext keyContext);
    Key(KeyType type, tools::Tool* tool);
    explicit Key(KeyAction action);
    explicit Key(WheelAction action);

    KeyType type() const { return m_type; }
    const ui::Accelerators& accels() const {
      return (m_useUsers ? m_users: m_accels);
    }
    const ui::Accelerators& origAccels() const { return m_accels; }
    const ui::Accelerators& userAccels() const { return m_users; }
    const ui::Accelerators& userRemovedAccels() const { return m_userRemoved; }

    void add(const ui::Accelerator& accel,
             const KeySource source,
             KeyboardShortcuts& globalKeys);
    const ui::Accelerator* isPressed(const ui::Message* msg,
                                     KeyboardShortcuts& globalKeys) const;
    bool isPressed() const;
    bool isLooselyPressed() const;

    bool hasAccel(const ui::Accelerator& accel) const;
    void disableAccel(const ui::Accelerator& accel);

    // Resets user accelerators to the original ones.
    void reset();

    void copyOriginalToUser();

    // for KeyType::Command
    Command* command() const { return m_command; }
    const Params& params() const { return m_params; }
    KeyContext keycontext() const { return m_keycontext; }
    // for KeyType::Tool or Quicktool
    tools::Tool* tool() const { return m_tool; }
    // for KeyType::Action
    KeyAction action() const { return m_action; }
    // for KeyType::WheelAction
    WheelAction wheelAction() const { return m_wheelAction; }

    std::string triggerString() const;

  private:
    KeyType m_type;
    ui::Accelerators m_accels;      // Default accelerators (from gui.xml)
    ui::Accelerators m_users;       // User-defined accelerators
    ui::Accelerators m_userRemoved; // Default accelerators removed by user
    bool m_useUsers;
    KeyContext m_keycontext;

    // for KeyType::Command
    Command* m_command;
    Params m_params;
    // for KeyType::Tool or Quicktool
    tools::Tool* m_tool;
    // for KeyType::Action
    KeyAction m_action;
    // for KeyType::WheelAction
    WheelAction m_wheelAction;
  };

  typedef std::shared_ptr<Key> KeyPtr;
  typedef std::vector<KeyPtr> Keys;

  std::string convertKeyContextToString(KeyContext keyContext);
  std::string convertKeyContextToUserFriendlyString(KeyContext keyContext);

} // namespace app

namespace base {

  template<> app::KeyAction convert_to(const std::string& from);
  template<> std::string convert_to(const app::KeyAction& from);

  template<> app::WheelAction convert_to(const std::string& from);
  template<> std::string convert_to(const app::WheelAction& from);

} // namespace base

#endif
