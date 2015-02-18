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
#include "app/cmd/remove_frame_tag.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "doc/frame_tag.h"

namespace app {

class RemoveFrameTagCommand : public Command {
public:
  RemoveFrameTagCommand();
  Command* clone() const override { return new RemoveFrameTagCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

RemoveFrameTagCommand::RemoveFrameTagCommand()
  : Command("RemoveFrameTag",
            "Remove Frame Tag",
            CmdRecordableFlag)
{
}

bool RemoveFrameTagCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void RemoveFrameTagCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  frame_t frame = writer.frame();

  FrameTag* best = nullptr;
  for (FrameTag* tag : sprite->frameTags()) {
    if (frame >= tag->fromFrame() &&
        frame <= tag->toFrame()) {
      if (!best ||
          (tag->toFrame() - tag->fromFrame()) < (best->toFrame() - best->fromFrame())) {
        best = tag;
      }
    }
  }

  if (!best)
    return;

  Transaction transaction(writer.context(), "Remove Frame Tag");
  transaction.execute(new cmd::RemoveFrameTag(sprite, best));
  transaction.commit();

  App::instance()->getMainWindow()->getTimeline()->invalidate();
}

Command* CommandFactory::createRemoveFrameTagCommand()
{
  return new RemoveFrameTagCommand;
}

} // namespace app
