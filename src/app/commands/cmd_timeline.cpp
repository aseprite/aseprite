// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline/timeline.h"

namespace app {

class TimelineCommand : public Command {
public:
  TimelineCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  bool onChecked(Context* ctx) override;

  bool m_open;
  bool m_close;
  bool m_switch;
};

TimelineCommand::TimelineCommand() : Command(CommandId::Timeline(), CmdUIOnlyFlag)
{
  m_open = true;
  m_close = false;
  m_switch = false;
}

void TimelineCommand::onLoadParams(const Params& params)
{
  m_open = params.get_as<bool>("open");
  m_close = params.get_as<bool>("close");
  m_switch = params.get_as<bool>("switch");
}

void TimelineCommand::onExecute(Context* context)
{
  bool visible = App::instance()->mainWindow()->getTimelineVisibility();
  bool newVisible = visible;

  if (m_switch)
    newVisible = !visible;
  else if (m_close)
    newVisible = false;
  else if (m_open)
    newVisible = true;

  if (visible != newVisible)
    App::instance()->mainWindow()->setTimelineVisibility(newVisible);
}

bool TimelineCommand::onChecked(Context* ctx)
{
  MainWindow* mainWin = App::instance()->mainWindow();
  if (!mainWin)
    return false;

  Timeline* timelineWin = mainWin->getTimeline();
  return (timelineWin && timelineWin->isVisible());
}

Command* CommandFactory::createTimelineCommand()
{
  return new TimelineCommand;
}

} // namespace app
