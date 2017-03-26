// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_frame_tag.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/cmd/set_frame_tag_range.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/loop_tag.h"
#include "app/transaction.h"
#include "app/ui/timeline/timeline.h"
#include "doc/frame_tag.h"

namespace app {

class SetLoopSectionCommand : public Command {
public:
  enum class Action { Auto, On, Off };

  SetLoopSectionCommand();
  Command* clone() const override { return new SetLoopSectionCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

  Action m_action;
  doc::frame_t m_begin, m_end;
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

void SetLoopSectionCommand::onLoadParams(const Params& params)
{
  std::string action = params.get("action");
  if (action == "on") m_action = Action::On;
  else if (action == "off") m_action = Action::Off;
  else m_action = Action::Auto;

  std::string begin = params.get("begin");
  std::string end = params.get("end");

  m_begin = frame_t(strtol(begin.c_str(), NULL, 10));
  m_end = frame_t(strtol(end.c_str(), NULL, 10));
}

bool SetLoopSectionCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::HasActiveDocument);
}

void SetLoopSectionCommand::onExecute(Context* ctx)
{
  doc::Document* doc = ctx->activeDocument();
  if (!doc)
    return;

  doc::Sprite* sprite = doc->sprite();
  doc::frame_t begin = m_begin;
  doc::frame_t end = m_end;
  bool on = false;

  switch (m_action) {

    case Action::Auto: {
      auto range = App::instance()->timeline()->range();
      if (range.enabled() && (range.frames() > 1)) {
        begin = range.selectedFrames().firstFrame();
        end = range.selectedFrames().lastFrame();
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

  doc::FrameTag* loopTag = get_loop_tag(sprite);
  if (on) {
    if (!loopTag) {
      loopTag = create_loop_tag(begin, end);

      ContextWriter writer(ctx);
      Transaction transaction(writer.context(), "Add Loop");
      transaction.execute(new cmd::AddFrameTag(sprite, loopTag));
      transaction.commit();
    }
    else if (loopTag->fromFrame() != begin ||
             loopTag->toFrame() != end) {
      ContextWriter writer(ctx);
      Transaction transaction(writer.context(), "Set Loop Range");
      transaction.execute(new cmd::SetFrameTagRange(loopTag, begin, end));
      transaction.commit();
    }
    else {
      Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::FrameTagProperties);
      ctx->executeCommand(cmd);
    }
  }
  else {
    if (loopTag) {
      ContextWriter writer(ctx);
      Transaction transaction(writer.context(), "Remove Loop");
      transaction.execute(new cmd::RemoveFrameTag(sprite, loopTag));
      transaction.commit();
    }
  }

  App::instance()->timeline()->invalidate();
}

Command* CommandFactory::createSetLoopSectionCommand()
{
  return new SetLoopSectionCommand;
}

} // namespace app
