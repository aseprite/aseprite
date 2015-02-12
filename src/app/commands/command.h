// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_COMMAND_H_INCLUDED
#define APP_COMMANDS_COMMAND_H_INCLUDED
#pragma once

#include "app/commands/command_factory.h"
#include <string>

namespace app {
  class Context;
  class Params;

  enum CommandFlags {
    CmdUIOnlyFlag     = 0x00000001,
    CmdRecordableFlag = 0x00000002,
  };

  class Command {
    const char* m_short_name;
    const char* m_friendly_name;
    CommandFlags m_flags;

  public:

    Command(const char* short_name, const char* friendly_name, CommandFlags flags);
    virtual ~Command();

    virtual Command* clone() const { return new Command(*this); }

    const char* short_name() const { return m_short_name; }
    std::string friendlyName() const;

    void loadParams(Params* params);
    bool isEnabled(Context* context);
    bool isChecked(Context* context);
    void execute(Context* context);

  protected:
    virtual void onLoadParams(Params* params);
    virtual bool onEnabled(Context* context);
    virtual bool onChecked(Context* context);
    virtual void onExecute(Context* context);
    virtual std::string onGetFriendlyName() const;
  };

} // namespace app

#endif
