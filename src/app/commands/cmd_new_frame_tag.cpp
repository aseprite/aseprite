// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_tag.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/tag_window.h"
#include "doc/tag.h"

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

NewFrameTagCommand::NewFrameTagCommand() : Command(CommandId::NewFrameTag(), CmdRecordableFlag)
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

  view::RealRange range = context->range();
  if (range.enabled() &&
      (range.type() == view::Range::kFrames || range.type() == view::Range::kCels)) {
    from = range.selectedFrames().firstFrame();
    to = range.selectedFrames().lastFrame();
  }

  std::unique_ptr<Tag> tag(new Tag(from, to));
  TagWindow window(sprite, tag.get());
  if (!window.show())
    return;

  window.rangeValue(from, to);
  tag->setFrameRange(from, to);
  tag->setName(window.nameValue());
  tag->setAniDir(window.aniDirValue());
  tag->setRepeat(window.repeatValue());
  tag->setUserData(window.userDataValue());

  {
    ContextWriter writer(reader);
    Tx tx(writer, friendlyName());
    tx(new cmd::AddTag(writer.sprite(), tag.get()));
    tag.release();
    tx.commit();
  }
}

Command* CommandFactory::createNewFrameTagCommand()
{
  return new NewFrameTagCommand;
}

} // namespace app
