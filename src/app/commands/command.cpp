// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/i18n/strings.h"

namespace app {

Command::Command(const char* id, CommandFlags flags) : m_id(id), m_flags(flags)
{
}

Command::~Command()
{
}

bool Command::needsParams() const
{
  return onNeedsParams();
}

void Command::loadParams(const Params& params)
{
  onLoadParams(params);
}

bool Command::isEnabled(Context* context)
{
  try {
    return onEnabled(context);
  }
  catch (...) {
    // TODO add a status-bar item
    return false;
  }
}

bool Command::isChecked(Context* context)
{
  try {
    return onChecked(context);
  }
  catch (...) {
    // TODO add a status-bar item...
    return false;
  }
}

void Command::execute(Context* context)
{
  onExecute(context);
}

bool Command::onNeedsParams() const
{
  // By default a command can be called without params
  return false;
}

// Converts specified parameters to class members.
void Command::onLoadParams(const Params& params)
{
  // do nothing
}

// Preconditions to execute the command
bool Command::onEnabled(Context* context)
{
  return true;
}

// Should the menu-item be checked?
bool Command::onChecked(Context* context)
{
  return false;
}

// Execute the command (after checking the preconditions).
void Command::onExecute(Context* context)
{
  // Do nothing
}

std::string Command::onGetFriendlyName() const
{
  if (auto* strings = Strings::instance())
    return strings->translate(("commands." + id()).c_str());
  return id();
}

} // namespace app
