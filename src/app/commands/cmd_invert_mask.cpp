// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/sprite.h"

namespace app {

class InvertMaskCommand : public Command {
public:
  InvertMaskCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

InvertMaskCommand::InvertMaskCommand()
  : Command(CommandId::InvertMask(), CmdRecordableFlag)
{
}

bool InvertMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void InvertMaskCommand::onExecute(Context* context)
{
  bool hasMask = false;
  {
    const ContextReader reader(context);
    if (reader.document()->isMaskVisible())
      hasMask = true;
  }

  // without mask?...
  if (!hasMask) {
    // so we select all
    Command* mask_all_cmd =
      Commands::instance()->byId(CommandId::MaskAll());
    context->executeCommand(mask_all_cmd);
  }
  // invert the current mask
  else {
    ContextWriter writer(context);
    Doc* document(writer.document());
    Sprite* sprite(writer.sprite());

    // Select all the sprite area
    std::unique_ptr<Mask> mask(new Mask());
    mask->replace(sprite->bounds());

    // Remove in the new mask the current sprite marked region
    const gfx::Rect& maskBounds = document->mask()->bounds();
    doc::fill_rect(mask->bitmap(),
      maskBounds.x, maskBounds.y,
      maskBounds.x + maskBounds.w-1,
      maskBounds.y + maskBounds.h-1, 0);

    Mask* curMask = document->mask();
    if (curMask->bitmap()) {
      // Copy the inverted region in the new mask (we just modify the
      // document's mask temporaly here)
      curMask->freeze();
      curMask->invert();
      doc::copy_image(mask->bitmap(),
        curMask->bitmap(),
        curMask->bounds().x,
        curMask->bounds().y);
      curMask->invert();
      curMask->unfreeze();
    }

    // We need only need the area inside the sprite
    mask->intersect(sprite->bounds());

    // Set the new mask
    Tx tx(writer.context(), "Mask Invert", DoesntModifyDocument);
    tx(new cmd::SetMask(document, mask.get()));
    tx.commit();

    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

Command* CommandFactory::createInvertMaskCommand()
{
  return new InvertMaskCommand;
}

} // namespace app
