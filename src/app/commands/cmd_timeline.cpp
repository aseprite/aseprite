/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"

namespace app {

class TimelineCommand : public Command {
public:
  TimelineCommand();
  Command* clone() { return new TimelineCommand(*this); }

protected:
  void onLoadParams(Params* params) OVERRIDE;
  void onExecute(Context* context) OVERRIDE;

  bool m_open;
  bool m_close;
  bool m_switch;
};

TimelineCommand::TimelineCommand()
  : Command("Timeline",
            "Switch Timeline",
            CmdUIOnlyFlag)
{
  m_open = true;
  m_close = false;
  m_switch = false;
}

void TimelineCommand::onLoadParams(Params* params)
{
  std::string open_str = params->get("open");
  if (open_str == "true") m_open = true;
  else m_open = false;

  std::string close_str = params->get("close");
  if (close_str == "true") m_close = true;
  else m_close = false;

  std::string switch_str = params->get("switch");
  if (switch_str == "true") m_switch = true;
  else m_switch = false;
}

void TimelineCommand::onExecute(Context* context)
{
  bool visible = App::instance()->getMainWindow()->getTimelineVisibility();
  bool newVisible = visible;

  if (m_switch)
    newVisible = !visible;
  else if (m_close)
    newVisible = false;
  else if (m_open)
    newVisible = true;

  if (visible != newVisible)
    App::instance()->getMainWindow()->setTimelineVisibility(newVisible);
}

Command* CommandFactory::createTimelineCommand()
{
  return new TimelineCommand;
}

} // namespace app
