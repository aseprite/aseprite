// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_frame_tag_anidir.h"
#include "app/cmd/set_frame_tag_color.h"
#include "app/cmd/set_frame_tag_name.h"
#include "app/cmd/set_frame_tag_range.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/transaction.h"
#include "app/ui/color_button.h"
#include "doc/anidir.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"

#include "generated_frame_tag_properties.h"

namespace app {

using namespace ui;

class FrameTagPropertiesCommand : public Command {
public:
  FrameTagPropertiesCommand();
  Command* clone() const override { return new FrameTagPropertiesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
};

FrameTagPropertiesCommand::FrameTagPropertiesCommand()
  : Command("FrameTagProperties",
            "Frame Tag Properties",
            CmdUIOnlyFlag)
{
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

  const FrameTag* best = nullptr;
  for (const FrameTag* tag : sprite->frameTags()) {
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

  app::gen::FrameTagProperties window;

  window.name()->setText(best->name());
  window.from()->setTextf("%d", best->fromFrame()+1);
  window.to()->setTextf("%d", best->toFrame()+1);
  window.color()->setColor(app::Color::fromRgb(
      doc::rgba_getr(best->color()),
      doc::rgba_getg(best->color()),
      doc::rgba_getb(best->color())));

  static_assert(
    int(doc::AniDir::FORWARD) == 0 &&
    int(doc::AniDir::REVERSE) == 1 &&
    int(doc::AniDir::PING_PONG) == 2, "doc::AniDir has changed");
  window.anidir()->addItem("Forward");
  window.anidir()->addItem("Reverse");
  window.anidir()->addItem("Ping-pong");
  window.anidir()->setSelectedItemIndex(int(best->aniDir()));

  window.openWindowInForeground();
  if (window.getKiller() == window.ok()) {
    std::string name = window.name()->getText();
    frame_t first = 0;
    frame_t last = sprite->lastFrame();
    frame_t from = window.from()->getTextInt()-1;
    frame_t to = window.to()->getTextInt()-1;
    from = MID(first, from, last);
    to = MID(from, to, last);
    app::Color color = window.color()->getColor();
    doc::color_t docColor = doc::rgba(
      color.getRed(), color.getGreen(), color.getBlue(), 255);
    doc::AniDir anidir = (doc::AniDir)window.anidir()->getSelectedItemIndex();

    ContextWriter writer(reader);
    Transaction transaction(writer.context(), "Change Frame Tag Properties");

    FrameTag* tag = const_cast<FrameTag*>(best);

    if (tag->name() != name)
      transaction.execute(new cmd::SetFrameTagName(tag, name));

    if (tag->fromFrame() != from ||
        tag->toFrame() != to)
      transaction.execute(new cmd::SetFrameTagRange(tag, from, to));

    if (tag->color() != docColor)
      transaction.execute(new cmd::SetFrameTagColor(tag, docColor));

    if (tag->aniDir() != anidir)
      transaction.execute(new cmd::SetFrameTagAniDir(tag, anidir));

    transaction.commit();
  }
}

Command* CommandFactory::createFrameTagPropertiesCommand()
{
  return new FrameTagPropertiesCommand;
}

} // namespace app
