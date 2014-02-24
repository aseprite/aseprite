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
#include "app/modules/gui.h"
#include "app/undo_transaction.h"
#include "gfx/size.h"
#include "raster/algorithm/flip_image.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"

namespace app {

FlipCommand::FlipCommand()
  : Command("Flip",
            "Flip",
            CmdRecordableFlag)
{
  m_flipMask = false;
  m_flipType = raster::algorithm::FlipHorizontal;
}

void FlipCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  m_flipMask = (target == "mask");

  std::string orientation = params->get("orientation");
  m_flipType = (orientation == "vertical" ? raster::algorithm::FlipVertical:
                                            raster::algorithm::FlipHorizontal);
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
                                    (m_flipType == raster::algorithm::FlipHorizontal ?
                                     "Flip Horizontal":
                                     "Flip Vertical"):
                                    (m_flipType == raster::algorithm::FlipHorizontal ?
                                     "Flip Canvas Horizontal":
                                     "Flip Canvas Vertical"));

    if (m_flipMask) {
      int x, y;
      Image* image = writer.image(&x, &y);
      if (!image)
        return;

      Mask* mask = NULL;
      bool alreadyFlipped = false;

      // This variable will be the area to be flipped inside the image.
      gfx::Rect bounds(image->getBounds());

      // If there is some portion of sprite selected, we flip the
      // selected region only. If the mask isn't visible, we flip the
      // whole image.
      if (document->isMaskVisible()) {
        mask = document->getMask();

        // Intersect the full area of the image with the mask's
        // bounds, so we don't request to flip an area outside the
        // image's bounds.
        bounds = bounds.createIntersect(gfx::Rect(mask->getBounds()).offset(-x, -y));

        // If the mask isn't a rectangular area, we've to flip the mask too.
        if (mask->getBitmap() != NULL && !mask->isRectangular()) {
          int bgcolor = app_get_color_to_clear_layer(writer.layer());

          // Flip the portion of image specified by the mask.
          mask->offsetOrigin(-x, -y);
          api.flipImageWithMask(image, mask, m_flipType, bgcolor);
          mask->offsetOrigin(x, y);
          alreadyFlipped = true;

          // Flip the mask.
          Image* maskBitmap = mask->getBitmap();
          if (maskBitmap != NULL) {
            // Create a flipped copy of the current mask.
            base::UniquePtr<Mask> newMask(new Mask(*mask));
            newMask->freeze();
            raster::algorithm::flip_image(newMask->getBitmap(),
              maskBitmap->getBounds(), m_flipType);
            newMask->unfreeze();

            // Change the current mask and generate the new boundaries.
            api.copyToCurrentMask(newMask);

            document->generateMaskBoundaries();
          }
        }
      }

      // Flip the portion of image specified by "bounds" variable.
      if (!alreadyFlipped) {
        api.flipImage(image, bounds, m_flipType);
      }
    }
    else {
      // get all sprite cels
      CelList cels;
      sprite->getCels(cels);

      // for each cel...
      for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
        Cel* cel = *it;
        Image* image = sprite->getStock()->getImage(cel->getImage());

        api.setCelPosition
          (sprite, cel,
           (m_flipType == raster::algorithm::FlipHorizontal ?
            sprite->getWidth() - image->getWidth() - cel->getX():
            cel->getX()),
           (m_flipType == raster::algorithm::FlipVertical ?
            sprite->getHeight() - image->getHeight() - cel->getY():
            cel->getY()));

        api.flipImage(image, image->getBounds(), m_flipType);
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
