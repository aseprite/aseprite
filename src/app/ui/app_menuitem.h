// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_APP_MENUITEM_H_INCLUDED
#define APP_UI_APP_MENUITEM_H_INCLUDED
#pragma once

#include "app/commands/params.h"
#include "ui/menu.h"

namespace app {
  class Key;
  class Command;

  // A widget that represent a menu item of the application.
  //
  // It's like a MenuItme, but it has a extra properties: the name of
  // the command to be executed when it's clicked (also that command is
  // used to check the availability of the command).
  class AppMenuItem : public ui::MenuItem {
  public:
    AppMenuItem(const char* text, Command* command = nullptr, const Params& params = Params());

    Key* getKey() { return m_key; }
    void setKey(Key* key) { m_key = key; }

    Command* getCommand() { return m_command; }
    const Params& getParams() const { return m_params; }

    static void setContextParams(const Params& params);

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onClick() override;

  private:
    Key* m_key;
    Command* m_command;
    Params m_params;

    static Params s_contextParams;
  };

} // namespace app

#endif
