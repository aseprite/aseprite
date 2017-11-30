// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/quick_command.h"

#include "app/i18n/strings.h"

namespace app {

QuickCommand::QuickCommand(const char* id, std::function<void()> execute)
  : Command(id, id, CmdUIOnlyFlag)
  , m_execute(execute)
{
}

QuickCommand::~QuickCommand()
{
}

QuickCommand* QuickCommand::clone() const
{
  return new QuickCommand(*this);
}

void QuickCommand::onExecute(Context* context)
{
  m_execute();
}

std::string QuickCommand::onGetFriendlyName() const
{
  std::string id = "commands.";
  id += this->id();
  return Strings::instance()->translate(id.c_str());
}

} // namespace app
