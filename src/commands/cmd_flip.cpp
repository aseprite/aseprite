/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "gfx/size.h"
#include "gui/list.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"
#include "undo_transaction.h"
#include "util/misc.h"

#include <allegro/unicode.h>

class FlipCommand : public Command
{
public:
  FlipCommand();
  Command* clone() const { return new FlipCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  static char* read_authors_txt(const char *filename);

  bool m_flipMask;
  raster::algorithm::FlipType m_flipType;
};

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
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();

  {
    UndoTransaction undoTransaction(document,
                                    m_flipMask ?
                                    (m_flipType == raster::algorithm::FlipHorizontal ?
                                     "Flip Horizontal":
                                     "Flip Vertical"):
                                    (m_flipType == raster::algorithm::FlipHorizontal ?
                                     "Flip Canvas Horizontal":
                                     "Flip Canvas Vertical"));

    if (m_flipMask) {
      Image* image;
      int x, y;
      image = sprite->getCurrentImage(&x, &y);
      if (!image)
        return;

      // This variable will be the area to be flipped inside the image.
      gfx::Rect bounds(gfx::Point(0, 0),
                       gfx::Size(image->w, image->h));

      // If there is some portion of sprite selected, we flip the
      // selected region only. If the mask isn't visible, we flip the
      // whole image.
      if (document->isMaskVisible()) {
        Mask* mask = document->getMask();
        gfx::Rect maskBounds = mask->getBounds();

        // Adjust the mask depending on the cel position.
        maskBounds.offset(-x, -y);

        // Intersect the full area of the image with the mask's
        // bounds, so we don't request to flip an area outside the
        // image's bounds.
        bounds = bounds.createIntersect(maskBounds);
      }

      // Flip the portion of image specified by "bounds" variable.
      undoTransaction.flipImage(image, bounds, m_flipType);
    }
    else {
      // get all sprite cels
      CelList cels;
      sprite->getCels(cels);

      // for each cel...
      for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
        Cel* cel = *it;
        Image* image = sprite->getStock()->getImage(cel->getImage());

        undoTransaction.setCelPosition
          (cel,
           (m_flipType == raster::algorithm::FlipHorizontal ?
            sprite->getWidth() - image->w - cel->getX():
            cel->getX()),
           (m_flipType == raster::algorithm::FlipVertical ?
            sprite->getHeight() - image->h - cel->getY():
            cel->getY()));

        undoTransaction.flipImage(image, gfx::Rect(0, 0, image->w, image->h), m_flipType);
      }
    }

    undoTransaction.commit();
  }

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createFlipCommand()
{
  return new FlipCommand;
}
