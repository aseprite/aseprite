// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_APP_MENUITEM_H_INCLUDED
#define APP_UI_APP_MENUITEM_H_INCLUDED
#pragma once

#include "app/commands/params.h"
#include "app/ui/key.h"
#include "os/menus.h"
#include "os/shortcut.h"
#include "ui/menu.h"

#include <memory>

namespace app {

  // A widget that represent a menu item of the application.
  //
  // It's like a MenuItme, but it has a extra properties: the name of
  // the command to be executed when it's clicked (also that command is
  // used to check the availability of the command).
  class AppMenuItem : public ui::MenuItem {
  public:
    struct Native {
      os::MenuItemRef menuItem = nullptr;
      os::Shortcut shortcut;
      app::KeyContext keyContext = app::KeyContext::Any;
    };

    AppMenuItem(const std::string& text,
                const std::string& commandId = std::string(),
                const Params& params = Params());
    AppMenuItem(const std::string& text, std::nullptr_t) = delete;

    KeyPtr key() { return m_key; }
    void setKey(const KeyPtr& key);

    void setIsRecentFileItem(bool state) { m_isRecentFileItem = state; }
    bool isRecentFileItem() const { return m_isRecentFileItem; }

    std::string getCommandId() const { return m_commandId; }
    Command* getCommand() const;
    const Params& getParams() const { return m_params; }

    Native* native() const { return m_native.get(); }
    void setNative(const Native& native);
    void disposeNative();
    void syncNativeMenuItemKeyShortcut();

    // Indicates if this is the standard "Edit" menu, used for macOS
    // which requires a standard "Edit" menu when the native file
    // dialog is displayed, so Command+C/X/V/A, etc. shortcuts start
    // working as expected.
    bool isStandardEditMenu() const { return m_isStandardEditMenu; }
    void setStandardEditMenu() { m_isStandardEditMenu = true; }

    static void setContextParams(const Params& params);

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onClick() override;
    void onValidate() override;

  private:
    KeyPtr m_key;
    std::string m_commandId;
    Params m_params;
    bool m_isRecentFileItem = false;
    bool m_isStandardEditMenu = false;
    std::unique_ptr<Native> m_native;

    static Params s_contextParams;
  };

} // namespace app

#endif
