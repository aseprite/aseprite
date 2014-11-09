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

#ifndef APP_UI_KEYBOARD_SHORTCUTS_H_INCLUDED
#define APP_UI_KEYBOARD_SHORTCUTS_H_INCLUDED
#pragma once

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
    Selection,
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

  enum class KeyAction {
    None,
    CopySelection,
    SnapToGrid,
    AngleSnap,
    MaintainAspectRatio,
    LockAxis,
    AddSelection,
    SubtractSelection,
    LeftMouseButton,
    RightMouseButton
  };

  class Key {
  public:
    Key(Command* command, const Params* params, KeyContext keyContext);
    Key(KeyType type, tools::Tool* tool);
    explicit Key(KeyAction action);

    KeyType type() const { return m_type; }
    const ui::Accelerators& accels() const {
      return (m_useUsers ? m_users: m_accels);
    }
    const ui::Accelerators& origAccels() const { return m_accels; }
    const ui::Accelerators& userAccels() const { return m_users; }

    void setUserAccels(const ui::Accelerators& accels);

    void add(const ui::Accelerator& accel, KeySource source);
    bool isPressed(ui::Message* msg) const;
    bool checkFromAllegroKeyArray();

    bool hasAccel(const ui::Accelerator& accel) const;
    void disableAccel(const ui::Accelerator& accel);

    // Resets user accelerators to the original ones.
    void reset();

    // for KeyType::Command
    Command* command() const { return m_command; }
    Params* params() const { return m_params; }
    KeyContext keycontext() const { return m_keycontext; }
    // for KeyType::Tool or Quicktool
    tools::Tool* tool() const { return m_tool; }
    // for KeyType::Action
    KeyAction action() const { return m_action; }

    std::string triggerString() const;

  private:
    KeyType m_type;
    ui::Accelerators m_accels;
    ui::Accelerators m_users;
    bool m_useUsers;
    KeyContext m_keycontext;

    // for KeyType::Command
    Command* m_command;
    Params* m_params;
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
      Params* params = NULL, KeyContext keyContext = KeyContext::Any);
    Key* tool(tools::Tool* tool);
    Key* quicktool(tools::Tool* tool);
    Key* action(KeyAction action);

    void disableAccel(const ui::Accelerator& accel);

    KeyContext getCurrentKeyContext();
    bool getCommandFromKeyMessage(ui::Message* msg, Command** command, Params** params);
    tools::Tool* getCurrentQuicktool(tools::Tool* currentTool);

  private:
    KeyboardShortcuts();

    void exportKeys(TiXmlElement& parent, KeyType type);

    Keys m_keys;

    DISABLE_COPYING(KeyboardShortcuts);
  };

} // namespace app

namespace base {

  template<> app::KeyAction convert_to(const std::string& from);
  template<> std::string convert_to(const app::KeyAction& from);
  
} // namespace base

#endif
