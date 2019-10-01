// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_tag_anidir.h"
#include "app/cmd/set_tag_color.h"
#include "app/cmd/set_tag_name.h"
#include "app/cmd/set_tag_range.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/loop_tag.h"
#include "app/tx.h"
#include "app/ui/tag_window.h"
#include "base/convert_to.h"
#include "doc/anidir.h"
#include "doc/sprite.h"
#include "doc/tag.h"

namespace app {

using namespace ui;

class FrameTagPropertiesCommand : public Command {
public:
  FrameTagPropertiesCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  std::string m_tagName;
  ObjectId m_tagId;
};

FrameTagPropertiesCommand::FrameTagPropertiesCommand()
  : Command(CommandId::FrameTagProperties(), CmdUIOnlyFlag)
  , m_tagId(NullId)
{
}

void FrameTagPropertiesCommand::onLoadParams(const Params& params)
{
  m_tagName = params.get("name");

  std::string id = params.get("id");
  if (!id.empty())
    m_tagId = ObjectId(base::convert_to<ObjectId>(id));
  else
    m_tagId = NullId;
}

bool FrameTagPropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FrameTagPropertiesCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  frame_t frame = reader.frame();
  const Tag* foundTag = nullptr;

  if (!m_tagName.empty())
    foundTag = sprite->tags().getByName(m_tagName);
  else if (m_tagId != NullId)
    foundTag = sprite->tags().getById(m_tagId);
  else
    foundTag = sprite->tags().innerTag(frame);

  if (!foundTag)
    return;

  TagWindow window(sprite, foundTag);
  if (!window.show())
    return;

  ContextWriter writer(reader);
  Tx tx(writer.context(), friendlyName());
  Tag* tag = const_cast<Tag*>(foundTag);

  std::string name = window.nameValue();
  if (tag->name() != name)
    tx(new cmd::SetTagName(tag, name));

  doc::frame_t from, to;
  window.rangeValue(from, to);
  if (tag->fromFrame() != from ||
      tag->toFrame() != to) {
    tx(new cmd::SetTagRange(tag, from, to));
  }

  doc::color_t docColor = window.colorValue();
  if (tag->color() != docColor)
    tx(new cmd::SetTagColor(tag, docColor));

  doc::AniDir anidir = window.aniDirValue();
  if (tag->aniDir() != anidir)
    tx(new cmd::SetTagAniDir(tag, anidir));

  tx.commit();
}

Command* CommandFactory::createFrameTagPropertiesCommand()
{
  return new FrameTagPropertiesCommand;
}

} // namespace app
