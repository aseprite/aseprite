// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/loop_tag.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "doc/tag.h"

namespace app {

class CollapseTagCommand : public Command {
public:
  CollapseTagCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* ctx) override;
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;

  // TODO onGetFriendlyName() needs the Context so we can return
  //      "Collapse Tag" or "Expand Tag" depending on the context
  //std::string onGetFriendlyName() const override;

private:
  Tag* tag(Context* ctx) const;

  ObjectId m_tagId;
};

CollapseTagCommand::CollapseTagCommand()
  : Command(CommandId::CollapseTag(), CmdRecordableFlag)
{
}

void CollapseTagCommand::onLoadParams(const Params& params)
{
  std::string id = params.get("id");
  if (!id.empty())
    m_tagId = ObjectId(base::convert_to<ObjectId>(id));
  else
    m_tagId = NullId;
}

bool CollapseTagCommand::onEnabled(Context* ctx)
{
  return (ctx->checkFlags(ContextFlags::ActiveDocumentIsReadable) &&
          tag(ctx) != nullptr);
}

bool CollapseTagCommand::onChecked(Context* ctx)
{
  const ContextReader reader(ctx);
  auto tag = this->tag(ctx);
  return (tag && tag->isCollapsed());
}

void CollapseTagCommand::onExecute(Context* ctx)
{
  auto tag = this->tag(ctx);
  if (!tag)
    return;

  ContextWriter writer(ctx);
  Doc* doc = writer.document();
  tag->setCollapsed(tag->isExpanded());
  doc->notifyTagCollapseChange(tag);
}

Tag* CollapseTagCommand::tag(Context* ctx) const
{
  const ContextReader reader(ctx);
  const Sprite* sprite = reader.sprite();

  // TODO add the tag to app::Site
  //const Tag* tag = reader.tag();
  doc::Tag* tag = nullptr;

  if (m_tagId != NullId) {
    tag = sprite->tags().getById(m_tagId);
  }
#if ENABLE_UI
  else if (auto editor = Editor::activeEditor()) {
    tag = editor
      ->getCustomizationDelegate()
      ->getTagProvider()
      ->getTagByFrame(editor->frame(), true);
  }
#endif
  return tag;
}

Command* CommandFactory::createCollapseTagCommand()
{
  return new CollapseTagCommand;
}

} // namespace app
