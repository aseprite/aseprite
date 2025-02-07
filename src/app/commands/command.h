// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_COMMAND_H_INCLUDED
#define APP_COMMANDS_COMMAND_H_INCLUDED
#pragma once

#include "app/commands/command_factory.h"
#include "app/commands/command_ids.h"
#include "app/ui/key_context.h"

#include <string>

namespace app {

class Context;
class Params;

enum CommandFlags {
  CmdUIOnlyFlag = 0x00000001,
  CmdRecordableFlag = 0x00000002,
};

class Command {
public:
  Command(const char* id, CommandFlags flags);
  virtual ~Command();

  const std::string& id() const { return m_id; }
  std::string friendlyName() const { return onGetFriendlyName(); }

  bool needsParams() const;
  void loadParams(const Params& params);
  bool isEnabled(Context* context);
  bool isChecked(Context* context);

  // Returns true if the command must be displayed in the Keyboard
  // Shortcuts list.
  virtual bool isListed(const Params& params) const { return true; }

protected:
  virtual bool onNeedsParams() const;
  virtual void onLoadParams(const Params& params);
  virtual bool onEnabled(Context* context);
  virtual bool onChecked(Context* context);
  virtual void onExecute(Context* context);
  virtual std::string onGetFriendlyName() const;

private:
  friend class Context;
  void execute(Context* context);

  std::string m_id;
  CommandFlags m_flags;
};

} // namespace app

#endif
