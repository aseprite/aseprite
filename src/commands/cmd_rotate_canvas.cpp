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

#include "app.h"
#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "job.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo_transaction.h"
#include "widgets/color_bar.h"

#include <allegro/unicode.h>

//////////////////////////////////////////////////////////////////////
// rotate_canvas

class RotateCanvasCommand : public Command
{
  int m_angle;

public:
  RotateCanvasCommand();
  Command* clone() { return new RotateCanvasCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

class RotateCanvasJob : public Job
{
  DocumentWriter m_document;
  Sprite* m_sprite;
  int m_angle;

public:

  RotateCanvasJob(const DocumentReader& document, int angle)
    : Job("Rotate Canvas")
    , m_document(document)
    , m_sprite(m_document->getSprite())
  {
    m_angle = angle;
  }

protected:

  /**
   * [working thread]
   */
  virtual void onJob()
  {
    UndoTransaction undoTransaction(m_document, "Rotate Canvas");

    // get all sprite cels
    CelList cels;
    m_sprite->getCels(cels);

    // for each cel...
    for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
      Cel* cel = *it;
      Image* image = m_sprite->getStock()->getImage(cel->getImage());

      // change it location
      switch (m_angle) {
        case 180:
          undoTransaction.setCelPosition(cel,
                                         m_sprite->getWidth() - cel->getX() - image->w,
                                         m_sprite->getHeight() - cel->getY() - image->h);
          break;
        case 90:
          undoTransaction.setCelPosition(cel, m_sprite->getHeight() - cel->getY() - image->h, cel->getX());
          break;
        case -90:
          undoTransaction.setCelPosition(cel, cel->getY(), m_sprite->getWidth() - cel->getX() - image->w);
          break;
      }
    }

    // for each stock's image
    for (int i=0; i<m_sprite->getStock()->size(); ++i) {
      Image* image = m_sprite->getStock()->getImage(i);
      if (!image)
        continue;

      // rotate the image
      Image* new_image = Image::create(image->imgtype,
                                       m_angle == 180 ? image->w: image->h,
                                       m_angle == 180 ? image->h: image->w);
      image_rotate(image, new_image, m_angle);

      undoTransaction.replaceStockImage(i, new_image);

      jobProgress((float)i / m_sprite->getStock()->size());

      // cancel all the operation?
      if (isCanceled())
        return;        // UndoTransaction destructor will undo all operations
    }

    // rotate mask
    if (m_document->isMaskVisible()) {
      Mask* origMask = m_document->getMask();
      UniquePtr<Mask> new_mask(new Mask());
      const gfx::Rect& origBounds = origMask->getBounds();
      int x = 0, y = 0;

      switch (m_angle) {
        case 180:
          x = m_sprite->getWidth() - origBounds.x - origBounds.w;
          y = m_sprite->getHeight() - origBounds.y - origBounds.h;
          break;
        case 90:
          x = m_sprite->getHeight() - origBounds.y - origBounds.h;
          y = origBounds.x;
          break;
        case -90:
          x = origBounds.y;
          y = m_sprite->getWidth() - origBounds.x - origBounds.w;
          break;
      }

      // create the new rotated mask
      new_mask->replace(x, y,
                        m_angle == 180 ? origBounds.w: origBounds.h,
                        m_angle == 180 ? origBounds.h: origBounds.w);
      image_rotate(origMask->getBitmap(), new_mask->getBitmap(), m_angle);

      // Copy new mask
      undoTransaction.copyToCurrentMask(new_mask);

      // Regenerate mask
      m_document->resetTransformation();
      m_document->generateMaskBoundaries();
    }

    // change the sprite's size
    if (m_angle != 180)
      undoTransaction.setSpriteSize(m_sprite->getHeight(), m_sprite->getWidth());

    // commit changes
    undoTransaction.commit();
  }

};

RotateCanvasCommand::RotateCanvasCommand()
  : Command("RotateCanvas",
            "Rotate Canvas",
            CmdRecordableFlag)
{
  m_angle = 0;
}

void RotateCanvasCommand::onLoadParams(Params* params)
{
  if (params->has_param("angle")) {
    m_angle = ustrtol(params->get("angle").c_str(), NULL, 10);
  }
}

bool RotateCanvasCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void RotateCanvasCommand::onExecute(Context* context)
{
  ActiveDocumentReader document(context);
  {
    RotateCanvasJob job(document, m_angle);
    job.startJob();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createRotateCanvasCommand()
{
  return new RotateCanvasCommand;
}
