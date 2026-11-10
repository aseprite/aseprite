// Aseprite
// Copyright (C)      2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/ui/workspace.h"
#include <string>

namespace app {

class GotoNextTabCommand : public Command {
public:
  GotoNextTabCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

GotoNextTabCommand::GotoNextTabCommand() : Command(CommandId::GotoNextTab())
{
}

bool GotoNextTabCommand::onEnabled(Context* context)
{
  Workspace* workspace = App::instance()->workspace();
  if (!workspace) // Workspace (main window) can be null if we are in --batch mode
    return false;
  return workspace->canSelectOtherTab();
}

void GotoNextTabCommand::onExecute(Context* context)
{
  App::instance()->workspace()->selectNextTab();
}

class GotoPreviousTabCommand : public Command {
public:
  GotoPreviousTabCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

GotoPreviousTabCommand::GotoPreviousTabCommand() : Command(CommandId::GotoPreviousTab())
{
}

bool GotoPreviousTabCommand::onEnabled(Context* context)
{
  Workspace* workspace = App::instance()->workspace();
  if (!workspace) // Workspace (main window) can be null if we are in --batch mode
    return false;
  return workspace->canSelectOtherTab();
}

void GotoPreviousTabCommand::onExecute(Context* context)
{
  App::instance()->workspace()->selectPreviousTab();
}
class GotoTabIndexCommand : public Command {
public:
  GotoTabIndexCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  int m_index;
};
GotoTabIndexCommand::GotoTabIndexCommand() : Command(CommandId::GotoTabIndex())
{
}

void GotoTabIndexCommand::onLoadParams(const Params& params)
{
  std::string Index = params.get("index");
  m_index = std::stoi(Index);
}

bool GotoTabIndexCommand::onEnabled(Context* context)
{
  Workspace* workspace = App::instance()->workspace();
  if (!workspace) // Workspace (main window) can be null if we are in --batch mode
    return false;
  return workspace->canSelectOtherTab();
}

void GotoTabIndexCommand::onExecute(Context* context)
{
  App::instance()->workspace()->selectTabIndex(m_index);
}

std::string GotoTabIndexCommand::onGetFriendlyName() const
{
  std::string text;
  switch (m_index) {
    case 9:  text = Strings::commands_GotoLastTab(); break;
    default: text = Strings::commands_GotoTabIndex(m_index); break;
  }
  return text;
}

Command* CommandFactory::createGotoTabIndexCommand()
{
  return new GotoTabIndexCommand;
}

Command* CommandFactory::createGotoNextTabCommand()
{
  return new GotoNextTabCommand;
}

Command* CommandFactory::createGotoPreviousTabCommand()
{
  return new GotoPreviousTabCommand;
}

} // namespace app
