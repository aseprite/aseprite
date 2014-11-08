/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"

namespace app {

class SetLoopSectionCommand : public Command {
public:
  enum class Action { Auto, On, Off };

  SetLoopSectionCommand();
  Command* clone() const override { return new SetLoopSectionCommand(*this); }

protected:
  void onLoadParams(Params* params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

  Action m_action;
  FrameNumber m_begin, m_end;
};

SetLoopSectionCommand::SetLoopSectionCommand()
  : Command("SetLoopSection",
            "Set Loop Section",
            CmdRecordableFlag)
  , m_action(Action::Auto)
  , m_begin(0)
  , m_end(0)
{
}

void SetLoopSectionCommand::onLoadParams(Params* params)
{
  std::string action = params->get("action");
  if (action == "on") m_action = Action::On;
  else if (action == "off") m_action = Action::Off;
  else m_action = Action::Auto;

  std::string begin = params->get("begin");
  std::string end = params->get("end");

  m_begin = FrameNumber(strtol(begin.c_str(), NULL, 10));
  m_end = FrameNumber(strtol(end.c_str(), NULL, 10));
}

bool SetLoopSectionCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::HasActiveDocument);
}

void SetLoopSectionCommand::onExecute(Context* ctx)
{
  Document* doc = ctx->activeDocument();
  if (!doc)
    return;

  IDocumentSettings* docSets = ctx->settings()->getDocumentSettings(doc);
  if (!docSets)
    return;

  FrameNumber begin = m_begin;
  FrameNumber end = m_end;
  bool on = false;

  switch (m_action) {

    case Action::Auto: {
      Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
      if (range.enabled() && (range.frames() > 1)) {
        begin = range.frameBegin();
        end = range.frameEnd();
        on = true;
      }
      else {
        on = false;
      }
      break;
    }

    case Action::On:
      on = true;
      break;

    case Action::Off:
      on = false;
      break;

  }

  if (on) {
    docSets->setLoopAnimation(true);
    docSets->setLoopRange(begin, end);
  }
  else 
    docSets->setLoopAnimation(false);
}

Command* CommandFactory::createSetLoopSectionCommand()
{
  return new SetLoopSectionCommand;
}

} // namespace app
