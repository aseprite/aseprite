// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_KEYBOARD_SHORTCUTS_H_INCLUDED
#define APP_UI_KEYBOARD_SHORTCUTS_H_INCLUDED
#pragma once

#include "app/commands/params.h"
#include "base/convert_to.h"
#include "base/disable_copying.h"
#include "ui/accelerator.h"
#include <vector>

class TiXmlElement;

namespace ui {
  class Message;
}

namespace app {

  class Command;
  class Params;

  namespace tools {
    class Tool;
  }

  enum class KeyContext {
    Any,
    Normal,
    SelectionTool,
    TranslatingSelection,
    ScalingSelection,
    RotatingSelection,
    MoveTool,
    FreehandTool,
  };

  enum class KeySource {
    Original,
    UserDefined
  };

  enum class KeyType {
    Command,
    Tool,
    Quicktool,
    Action,
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
    AutoSelectLayer           = 0x00000080,
    LeftMouseButton           = 0x00000100,
    RightMouseButton          = 0x00000200,
    StraightLineFromLastPoint = 0x00000400
  };

  inline KeyAction operator&(KeyAction a, KeyAction b) {
    return KeyAction(int(a) & int(b));
  }

  class Key {
  public:
    Key(Command* command, const Params& params, KeyContext keyContext);
    Key(KeyType type, tools::Tool* tool);
    explicit Key(KeyAction action);

    KeyType type() const { return m_type; }
    const ui::Accelerators& accels() const {
      return (m_useUsers ? m_users: m_accels);
    }
    const ui::Accelerators& origAccels() const { return m_accels; }
    const ui::Accelerators& userAccels() const { return m_users; }
    const ui::Accelerators& userRemovedAccels() const { return m_userRemoved; }

    void add(const ui::Accelerator& accel, KeySource source);
    bool isPressed(ui::Message* msg) const;
    bool isPressed() const;
    bool isLooselyPressed() const;

    bool hasAccel(const ui::Accelerator& accel) const;
    void disableAccel(const ui::Accelerator& accel);

    // Resets user accelerators to the original ones.
    void reset();

    // for KeyType::Command
    Command* command() const { return m_command; }
    const Params& params() const { return m_params; }
    KeyContext keycontext() const { return m_keycontext; }
    // for KeyType::Tool or Quicktool
    tools::Tool* tool() const { return m_tool; }
    // for KeyType::Action
    KeyAction action() const { return m_action; }

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
  };

  class KeyboardShortcuts {
  public:
    typedef std::vector<Key*> Keys;
    typedef Keys::iterator iterator;
    typedef Keys::const_iterator const_iterator;

    static KeyboardShortcuts* instance();
    ~KeyboardShortcuts();

    iterator begin() { return m_keys.begin(); }
    iterator end() { return m_keys.end(); }
    const_iterator begin() const { return m_keys.begin(); }
    const_iterator end() const { return m_keys.end(); }

    void clear();
    void importFile(TiXmlElement* rootElement, KeySource source);
    void importFile(const std::string& filename, KeySource source);
    void exportFile(const std::string& filename);
    void reset();

    Key* command(const char* commandName,
      const Params& params = Params(), KeyContext keyContext = KeyContext::Any);
    Key* tool(tools::Tool* tool);
    Key* quicktool(tools::Tool* tool);
    Key* action(KeyAction action);

    void disableAccel(const ui::Accelerator& accel, KeyContext keyContext);

    KeyContext getCurrentKeyContext();
    bool getCommandFromKeyMessage(ui::Message* msg, Command** command, Params* params);
    tools::Tool* getCurrentQuicktool(tools::Tool* currentTool);
    KeyAction getCurrentActionModifiers(KeyContext context);

  private:
    KeyboardShortcuts();

    void exportKeys(TiXmlElement& parent, KeyType type);
    void exportAccel(TiXmlElement& parent, Key* key, const ui::Accelerator& accel, bool removed);

    Keys m_keys;

    DISABLE_COPYING(KeyboardShortcuts);
  };

} // namespace app

namespace base {

  template<> app::KeyAction convert_to(const std::string& from);
  template<> std::string convert_to(const app::KeyAction& from);

} // namespace base

#endif
