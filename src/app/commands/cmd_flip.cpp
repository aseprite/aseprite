/* Aseprite
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

#include "app/commands/cmd_flip.h"

#include "app/app.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/undo_transaction.h"
#include "app/util/range_utils.h"
#include "gfx/size.h"
#include "doc/algorithm/flip_image.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "doc/stock.h"

namespace app {

FlipCommand::FlipCommand()
  : Command("Flip",
            "Flip",
            CmdRecordableFlag)
{
  m_flipMask = false;
  m_flipType = doc::algorithm::FlipHorizontal;
}

void FlipCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  m_flipMask = (target == "mask");

  std::string orientation = params->get("orientation");
  m_flipType = (orientation == "vertical" ? doc::algorithm::FlipVertical:
                                            doc::algorithm::FlipHorizontal);
}

bool FlipCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FlipCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document = writer.document();
  Sprite* sprite = writer.sprite();
  DocumentApi api = document->getApi();

  {
    UndoTransaction undoTransaction(writer.context(),
                                    m_flipMask ?
                                    (m_flipType == doc::algorithm::FlipHorizontal ?
                                     "Flip Horizontal":
                                     "Flip Vertical"):
                                    (m_flipType == doc::algorithm::FlipHorizontal ?
                                     "Flip Canvas Horizontal":
                                     "Flip Canvas Vertical"));

    if (m_flipMask) {
      Mask* mask = document->mask();
      CelList cels;

      DocumentLocation loc = *writer.location();
      DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
      if (range.enabled())
        cels = get_cels_in_range(sprite, range);
      else if (writer.cel())
        cels.push_back(writer.cel());

      for (Cel* cel : cels) {
        loc.frame(cel->frame());
        loc.layer(cel->layer());

        int x, y;
        Image* image = loc.image(&x, &y);
        if (!image)
          continue;

        bool alreadyFlipped = false;

        // This variable will be the area to be flipped inside the image.
        gfx::Rect bounds(image->bounds());

        // If there is some portion of sprite selected, we flip the
        // selected region only. If the mask isn't visible, we flip the
        // whole image.
        if (document->isMaskVisible()) {
          // Intersect the full area of the image with the mask's
          // bounds, so we don't request to flip an area outside the
          // image's bounds.
          bounds = bounds.createIntersect(gfx::Rect(mask->bounds()).offset(-x, -y));

          // If the mask isn't a rectangular area, we've to flip the mask too.
          if (mask->bitmap() && !mask->isRectangular()) {
            // Flip the portion of image specified by the mask.
            mask->offsetOrigin(-x, -y);
            api.flipImageWithMask(writer.layer(), image, mask, m_flipType);
            mask->offsetOrigin(x, y);
            alreadyFlipped = true;
          }
        }

        // Flip the portion of image specified by "bounds" variable.
        if (!alreadyFlipped) {
          api.setCelPosition
            (sprite, cel,
              (m_flipType == doc::algorithm::FlipHorizontal ?
                sprite->width() - image->width() - cel->x():
                cel->x()),
              (m_flipType == doc::algorithm::FlipVertical ?
                sprite->height() - image->height() - cel->y():
                cel->y()));

          api.flipImage(image, bounds, m_flipType);
        }
      }

      // Flip the mask.
      Image* maskBitmap = mask->bitmap();
      if (maskBitmap) {
        // Create a flipped copy of the current mask.
        base::UniquePtr<Mask> newMask(new Mask(*mask));
        newMask->freeze();
        doc::algorithm::flip_image(newMask->bitmap(),
          maskBitmap->bounds(), m_flipType);
        newMask->unfreeze();

        // Change the current mask and generate the new boundaries.
        api.copyToCurrentMask(newMask);

        document->generateMaskBoundaries();
      }
    }
    else {
      // get all sprite cels
      CelList cels;
      sprite->getCels(cels);

      // for each cel...
      for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
        Cel* cel = *it;
        Image* image = cel->image();

        api.setCelPosition
          (sprite, cel,
           (m_flipType == doc::algorithm::FlipHorizontal ?
            sprite->width() - image->width() - cel->x():
            cel->x()),
           (m_flipType == doc::algorithm::FlipVertical ?
            sprite->height() - image->height() - cel->y():
            cel->y()));

        api.flipImage(image, image->bounds(), m_flipType);
      }
    }

    undoTransaction.commit();
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createFlipCommand()
{
  return new FlipCommand;
}

} // namespace app
