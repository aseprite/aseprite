// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/loop_tag.h"
#include "app/tx.h"
#include "app/ui/timeline/timeline.h"
#include "base/convert_to.h"
#include "doc/frame_tag.h"

namespace app {

class RemoveFrameTagCommand : public Command {
public:
  RemoveFrameTagCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_tagName;
  ObjectId m_tagId;
};

RemoveFrameTagCommand::RemoveFrameTagCommand()
  : Command(CommandId::RemoveFrameTag(), CmdRecordableFlag)
  , m_tagId(NullId)
{
}

void RemoveFrameTagCommand::onLoadParams(const Params& params)
{
  m_tagName = params.get("name");

  std::string id = params.get("id");
  if (!id.empty())
    m_tagId = ObjectId(base::convert_to<ObjectId>(id));
  else
    m_tagId = NullId;
}

bool RemoveFrameTagCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void RemoveFrameTagCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Sprite* sprite(writer.sprite());
  frame_t frame = writer.frame();
  FrameTag* foundTag = nullptr;

  if (!m_tagName.empty())
    foundTag = sprite->frameTags().getByName(m_tagName);
  else if (m_tagId != NullId)
    foundTag = sprite->frameTags().getById(m_tagId);
  else
    foundTag = sprite->frameTags().innerTag(frame);

  if (!foundTag)
    return;

  Tx tx(writer.context(), "Remove Frame Tag");
  tx(new cmd::RemoveFrameTag(sprite, foundTag));
  tx.commit();

  App::instance()->timeline()->invalidate();
}

Command* CommandFactory::createRemoveFrameTagCommand()
{
  return new RemoveFrameTagCommand;
}

} // namespace app
