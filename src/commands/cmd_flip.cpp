/* ASE - Allegro Sprite Editor
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
  bool m_flip_mask;
  bool m_flip_horizontal;
  bool m_flip_vertical;

public:
  FlipCommand();
  Command* clone() const { return new FlipCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  static char* read_authors_txt(const char *filename);
};

FlipCommand::FlipCommand()
  : Command("Flip",
            "Flip",
            CmdRecordableFlag)
{
  m_flip_mask = false;
  m_flip_horizontal = false;
  m_flip_vertical = false;
}

void FlipCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  m_flip_mask = (target == "mask");

  std::string orientation = params->get("orientation");
  m_flip_horizontal = (orientation == "horizontal");
  m_flip_vertical = (orientation == "vertical");
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
                                    m_flip_mask ?
                                    (m_flip_horizontal ? "Flip Horizontal":
                                                         "Flip Vertical"):
                                    (m_flip_horizontal ? "Flip Canvas Horizontal":
                                                         "Flip Canvas Vertical"));

    if (m_flip_mask) {
      Image* image;
      int x1, y1, x2, y2;
      int x, y;

      image = sprite->getCurrentImage(&x, &y);
      if (!image)
        return;

      // Mask is empty?
      if (!document->isMaskVisible()) {
        // so we flip the entire image
        x1 = 0;
        y1 = 0;
        x2 = image->w-1;
        y2 = image->h-1;
      }
      else {
        // apply the cel offset
        x1 = document->getMask()->x - x;
        y1 = document->getMask()->y - y;
        x2 = document->getMask()->x + document->getMask()->w - 1 - x;
        y2 = document->getMask()->y + document->getMask()->h - 1 - y;

        // clip
        x1 = MID(0, x1, image->w-1);
        y1 = MID(0, y1, image->h-1);
        x2 = MID(0, x2, image->w-1);
        y2 = MID(0, y2, image->h-1);
      }

      undoTransaction.flipImage(image, x1, y1, x2, y2,
                                m_flip_horizontal, m_flip_vertical);
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
           m_flip_horizontal ? sprite->getWidth() - image->w - cel->getX(): cel->getX(),
           m_flip_vertical ? sprite->getHeight() - image->h - cel->getY(): cel->getY());

        undoTransaction.flipImage(image, 0, 0, image->w-1, image->h-1,
                                  m_flip_horizontal, m_flip_vertical);
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
