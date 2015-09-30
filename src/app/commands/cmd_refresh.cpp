// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/status_bar.h"
#include "ui/system.h"
#include "ui/theme.h"

#if defined _WIN32 && defined _DEBUG
  #include <windows.h>

  #include <psapi.h>
#endif

namespace app {

class RefreshCommand : public Command {
public:
  RefreshCommand();
  Command* clone() const override { return new RefreshCommand(*this); }

protected:
  void onExecute(Context* context) override;
};

RefreshCommand::RefreshCommand()
  : Command("Refresh",
            "Refresh",
            CmdUIOnlyFlag)
{
}

void RefreshCommand::onExecute(Context* context)
{
  ui::CurrentTheme::get()->regenerate();

  app_refresh_screen();

  // Print memory information
#if defined _WIN32 && defined _DEBUG
  {
    PROCESS_MEMORY_COUNTERS pmc;
    if (::GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      StatusBar::instance()
        ->showTip(1000,
                  "Current memory: %.16g KB (%lu)\n"
                  "Peak of memory: %.16g KB (%lu)",
                  pmc.WorkingSetSize / 1024.0, pmc.WorkingSetSize,
                  pmc.PeakWorkingSetSize / 1024.0, pmc.PeakWorkingSetSize);
    }
  }
#endif
}

Command* CommandFactory::createRefreshCommand()
{
  return new RefreshCommand;
}

} // namespace app
