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
#include "app/launcher.h"
#include "base/fs.h"
#include "ver/info.h"

namespace app {

class LaunchCommand : public Command {
public:
  LaunchCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;
  bool isListed(const Params& params) const override { return !params.get("path").empty(); }

private:
  enum Type { Url };

  Type m_type;
  std::string m_path;
};

LaunchCommand::LaunchCommand()
  : Command(CommandId::Launch(), CmdUIOnlyFlag)
  , m_type(Url)
  , m_path("")
{
}

void LaunchCommand::onLoadParams(const Params& params)
{
  m_path = params.get("path");

  if (m_type == Url && !m_path.empty() && m_path[0] == '/') {
    m_path = std::string(get_app_url()) + m_path.substr(1);
  }
}

void LaunchCommand::onExecute(Context* context)
{
  switch (m_type) {
    case Url: launcher::open_url(m_path); break;
  }
}

std::string LaunchCommand::onGetFriendlyName() const
{
  return Command::onGetFriendlyName() + ": " + m_path;
}

Command* CommandFactory::createLaunchCommand()
{
  return new LaunchCommand;
}

} // namespace app
