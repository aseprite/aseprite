// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
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
#include "base/vector2d.h"
#include "ui/accelerator.h"

#include <memory>
#include <utility>
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
    ExtensionDefined,
    UserDefined
  };

  enum class KeyType {
    Command,
    Tool,
    Quicktool,
    Action,
    WheelAction,
    DragAction,
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
    FineControl               = 0x00040000,
  };

  enum class WheelAction {
    None,
    Zoom,
    VScroll,
    HScroll,
    FgColor,
    BgColor,
    FgTile,
    BgTile,
    Frame,
    BrushSize,
    BrushAngle,
    ToolSameGroup,
    ToolOtherGroup,
    Layer,
    InkType,
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

  class Key;
  using KeyPtr = std::shared_ptr<Key>;
  using Keys = std::vector<KeyPtr>;
  using KeySourceAccelList = std::vector<std::pair<KeySource, ui::Accelerator>>;
  using DragVector = base::Vector2d<double>;

  class Key {
  public:
    Key(const Key& key);
    Key(Command* command, const Params& params,
        const KeyContext keyContext);
    Key(const KeyType type, tools::Tool* tool);
    explicit Key(const KeyAction action,
                 const KeyContext keyContext);
    explicit Key(const WheelAction action);
    static KeyPtr MakeDragAction(WheelAction dragAction);

    KeyType type() const { return m_type; }
    const ui::Accelerators& accels() const;
    const KeySourceAccelList addsKeys() const { return m_adds; }
    const KeySourceAccelList delsKeys() const { return m_dels; }

    void add(const ui::Accelerator& accel,
             const KeySource source,
             KeyboardShortcuts& globalKeys);
    const ui::Accelerator* isPressed(const ui::Message* msg,
                                     const KeyboardShortcuts& globalKeys,
                                     const KeyContext keyContext) const;
    const ui::Accelerator* isPressed(const ui::Message* msg,
                                     const KeyboardShortcuts& globalKeys) const;
    bool isPressed() const;
    bool isLooselyPressed() const;

    bool hasAccel(const ui::Accelerator& accel) const;
    bool hasUserDefinedAccels() const;

    // The KeySource indicates from where the key was disabled
    // (e.g. if it was removed from an extension-defined file, or from
    // user-defined).
    void disableAccel(const ui::Accelerator& accel,
                      const KeySource source);

    // Resets user accelerators to the original & extension-defined ones.
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
    // for KeyType::WheelAction / KeyType::DragAction
    WheelAction wheelAction() const { return m_wheelAction; }
    // for KeyType::DragAction
    DragVector dragVector() const { return m_dragVector; }
    void setDragVector(const DragVector& v) { m_dragVector = v; }

    std::string triggerString() const;

  private:
    KeyType m_type;
    KeySourceAccelList m_adds;
    KeySourceAccelList m_dels;
    // Final list of accelerators after processing the
    // addition/deletion of extension-defined & user-defined keys.
    mutable std::unique_ptr<ui::Accelerators> m_accels;
    KeyContext m_keycontext;

    // for KeyType::Command
    Command* m_command;
    Params m_params;

    tools::Tool* m_tool;        // for KeyType::Tool or Quicktool
    KeyAction m_action;         // for KeyType::Action
    WheelAction m_wheelAction;  // for KeyType::WheelAction / DragAction
    DragVector m_dragVector;    // for KeyType::DragAction
  };

  std::string convertKeyContextToUserFriendlyString(KeyContext keyContext);

} // namespace app

namespace base {

  template<> app::KeyAction convert_to(const std::string& from);
  template<> std::string convert_to(const app::KeyAction& from);

  template<> app::WheelAction convert_to(const std::string& from);
  template<> std::string convert_to(const app::WheelAction& from);

} // namespace base

#endif
