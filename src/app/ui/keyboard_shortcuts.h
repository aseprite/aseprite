// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_KEYBOARD_SHORTCUTS_H_INCLUDED
#define APP_UI_KEYBOARD_SHORTCUTS_H_INCLUDED
#pragma once

#include "app/ui/key.h"
#include "obs/signal.h"

class TiXmlElement;

namespace app {

  class KeyboardShortcuts {
  public:
    typedef Keys::iterator iterator;
    typedef Keys::const_iterator const_iterator;

    static KeyboardShortcuts* instance();
    static void destroyInstance();

    KeyboardShortcuts();
    KeyboardShortcuts(const KeyboardShortcuts&) = delete;
    KeyboardShortcuts& operator=(const KeyboardShortcuts&) = delete;
    ~KeyboardShortcuts();

    iterator begin() { return m_keys.begin(); }
    iterator end() { return m_keys.end(); }
    const_iterator begin() const { return m_keys.begin(); }
    const_iterator end() const { return m_keys.end(); }

    void setKeys(const KeyboardShortcuts& keys,
                 const bool cloneKeys);

    void clear();
    void importFile(TiXmlElement* rootElement, KeySource source);
    void importFile(const std::string& filename, KeySource source);
    void exportFile(const std::string& filename);
    void reset();

    KeyPtr command(const char* commandName,
                   const Params& params = Params(),
                   const KeyContext keyContext = KeyContext::Any) const;
    KeyPtr tool(tools::Tool* tool) const;
    KeyPtr quicktool(tools::Tool* tool) const;
    KeyPtr action(const KeyAction action,
                  const KeyContext keyContext = KeyContext::Any) const;
    KeyPtr wheelAction(const WheelAction action) const;
    KeyPtr dragAction(const WheelAction action) const;

    void disableAccel(const ui::Accelerator& accel,
                      const KeySource source,
                      const KeyContext keyContext,
                      const Key* newKey);

    KeyContext getCurrentKeyContext() const;
    bool getCommandFromKeyMessage(const ui::Message* msg, Command** command, Params* params);
    tools::Tool* getCurrentQuicktool(tools::Tool* currentTool);
    KeyAction getCurrentActionModifiers(KeyContext context);
    WheelAction getWheelActionFromMouseMessage(const KeyContext context,
                                               const ui::Message* msg);
    Keys getDragActionsFromKeyMessage(const KeyContext context,
                                      const ui::Message* msg);
    bool hasMouseWheelCustomization() const;
    void clearMouseWheelKeys();
    void addMissingMouseWheelKeys();
    void setDefaultMouseWheelKeys(const bool zoomWithWheel);

    void addMissingKeysForCommands();

    // Generated when the keyboard shortcuts are modified by the user.
    // Useful to regenerate tooltips with shortcuts.
    obs::signal<void()> UserChange;

  private:
    void exportKeys(TiXmlElement& parent, KeyType type);
    void exportAccel(TiXmlElement& parent, const Key* key, const ui::Accelerator& accel, bool removed);

    mutable Keys m_keys;
  };

  std::string key_tooltip(const char* str, const Key* key);

  inline std::string key_tooltip(const char* str,
                                 const char* commandName,
                                 const Params& params = Params(),
                                 KeyContext keyContext = KeyContext::Any) {
    return key_tooltip(
      str, KeyboardShortcuts::instance()->command(
        commandName, params, keyContext).get());
  }

  inline std::string key_tooltip(const char* str,
                                 KeyAction keyAction,
                                 KeyContext keyContext = KeyContext::Any) {
    return key_tooltip(
      str, KeyboardShortcuts::instance()->action(keyAction, keyContext).get());
  }

} // namespace app

#endif
