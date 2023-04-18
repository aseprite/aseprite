// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/script/engine.h"
#include "app/ui/optional_alert.h"
#include "base/fs.h"
#include "fmt/format.h"
#include "ui/manager.h"

#include <cstdio>

namespace app {

class RunScriptCommand : public Command {
public:
  RunScriptCommand();

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  std::string m_filename;
  Params m_params;
};

RunScriptCommand::RunScriptCommand()
  : Command(CommandId::RunScript(), CmdRecordableFlag)
{
}

void RunScriptCommand::onLoadParams(const Params& params)
{
  m_filename = params.get("filename");
  if (base::get_file_path(m_filename).empty()) {
    ResourceFinder rf;
    rf.includeDataDir(base::join_path("scripts", m_filename).c_str());
    if (rf.findFirst())
      m_filename = rf.filename();
  }

  m_params = params;
}

void RunScriptCommand::onExecute(Context* context)
{
#if ENABLE_UI
  if (context->isUIAvailable()) {
    int ret = OptionalAlert::show(
      Preferences::instance().scripts.showRunScriptAlert,
      1, // Yes is the default option when the alert dialog is disabled
      fmt::format(Strings::alerts_run_script(), m_filename));
    if (ret != 1)
      return;
  }
#endif // ENABLE_UI

  App::instance()
    ->scriptEngine()
    ->evalUserFile(m_filename, m_params);

#if ENABLE_UI
  if (context->isUIAvailable())
    ui::Manager::getDefault()->invalidate();
#endif
}

std::string RunScriptCommand::onGetFriendlyName() const
{
  if (m_filename.empty())
    return getBaseFriendlyName();
  else
    return fmt::format("{0}: {1}",
                       getBaseFriendlyName(),
                       base::get_file_name(m_filename));
}

Command* CommandFactory::createRunScriptCommand()
{
  return new RunScriptCommand;
}

} // namespace app
