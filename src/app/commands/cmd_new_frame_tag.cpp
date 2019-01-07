// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_frame_tag.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/frame_tag_window.h"
#include "app/ui/timeline/timeline.h"
#include "doc/frame_tag.h"

#include <stdexcept>

namespace app {

using namespace doc;

class NewFrameTagCommand : public Command {
public:
  NewFrameTagCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

NewFrameTagCommand::NewFrameTagCommand()
  : Command(CommandId::NewFrameTag(), CmdRecordableFlag)
{
}

bool NewFrameTagCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewFrameTagCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());
  frame_t from = reader.frame();
  frame_t to = reader.frame();

  auto range = App::instance()->timeline()->range();
  if (range.enabled() &&
      (range.type() == DocRange::kFrames ||
       range.type() == DocRange::kCels)) {
    from = range.selectedFrames().firstFrame();
    to = range.selectedFrames().lastFrame();
  }

  std::unique_ptr<FrameTag> frameTag(new FrameTag(from, to));
  FrameTagWindow window(sprite, frameTag.get());
  if (!window.show())
    return;

  window.rangeValue(from, to);
  frameTag->setFrameRange(from, to);
  frameTag->setName(window.nameValue());
  frameTag->setColor(window.colorValue());
  frameTag->setAniDir(window.aniDirValue());

  {
    ContextWriter writer(reader);
    Tx tx(writer.context(), "New Frames Tag");
    tx(new cmd::AddFrameTag(writer.sprite(), frameTag.get()));
    frameTag.release();
    tx.commit();
  }

  App::instance()->timeline()->invalidate();
}

Command* CommandFactory::createNewFrameTagCommand()
{
  return new NewFrameTagCommand;
}

} // namespace app
