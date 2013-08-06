/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/undo_transaction.h"
#include "app/undoers/set_mask.h"
#include "base/unique_ptr.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"

namespace app {

class InvertMaskCommand : public Command {
public:
  InvertMaskCommand();
  Command* clone() { return new InvertMaskCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

InvertMaskCommand::InvertMaskCommand()
  : Command("InvertMask",
            "Invert Mask",
            CmdRecordableFlag)
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
      CommandsModule::instance()->getCommandByName(CommandId::MaskAll);
    context->executeCommand(mask_all_cmd);
  }
  // invert the current mask
  else {
    ContextWriter writer(context);
    Document* document(writer.document());
    Sprite* sprite(writer.sprite());
    UndoTransaction undo(writer.context(), "Mask Invert", undo::DoesntModifyDocument);
    if (undo.isEnabled())
      undo.pushUndoer(new undoers::SetMask(undo.getObjects(), document));

    /* create a new mask */
    base::UniquePtr<Mask> mask(new Mask());

    /* select all the sprite area */
    mask->replace(0, 0, sprite->getWidth(), sprite->getHeight());

    /* remove in the new mask the current sprite marked region */
    const gfx::Rect& maskBounds = document->getMask()->getBounds();
    image_rectfill(mask->getBitmap(),
                   maskBounds.x, maskBounds.y,
                   maskBounds.x + maskBounds.w-1,
                   maskBounds.y + maskBounds.h-1, 0);

    // Invert the current mask in the sprite
    document->getMask()->invert();
    if (document->getMask()->getBitmap()) {
      // Copy the inverted region in the new mask
      image_copy(mask->getBitmap(),
                 document->getMask()->getBitmap(),
                 document->getMask()->getBounds().x,
                 document->getMask()->getBounds().y);
    }

    // We need only need the area inside the sprite
    mask->intersect(0, 0, sprite->getWidth(), sprite->getHeight());

    // Set the new mask
    document->setMask(mask);
    undo.commit();

    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

Command* CommandFactory::createInvertMaskCommand()
{
  return new InvertMaskCommand;
}

} // namespace app
