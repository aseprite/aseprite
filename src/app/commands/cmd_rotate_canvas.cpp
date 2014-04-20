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

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/undo_transaction.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/ui.h"

#include <allegro/unicode.h>

namespace app {

class RotateCanvasCommand : public Command {
  int m_angle;

public:
  RotateCanvasCommand();
  Command* clone() const OVERRIDE { return new RotateCanvasCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

class RotateCanvasJob : public Job
{
  ContextWriter m_writer;
  Document* m_document;
  Sprite* m_sprite;
  int m_angle;

public:

  RotateCanvasJob(const ContextReader& reader, int angle)
    : Job("Rotate Canvas")
    , m_writer(reader)
    , m_document(m_writer.document())
    , m_sprite(m_writer.sprite())
  {
    m_angle = angle;
  }

protected:

  /**
   * [working thread]
   */
  virtual void onJob()
  {
    UndoTransaction undoTransaction(m_writer.context(), "Rotate Canvas");
    DocumentApi api = m_document->getApi();

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
          api.setCelPosition(m_sprite, cel,
                             m_sprite->getWidth() - cel->getX() - image->getWidth(),
                             m_sprite->getHeight() - cel->getY() - image->getHeight());
          break;
        case 90:
          api.setCelPosition(m_sprite, cel,
                             m_sprite->getHeight() - cel->getY() - image->getHeight(),
                             cel->getX());
          break;
        case -90:
          api.setCelPosition(m_sprite, cel,
                             cel->getY(),
                             m_sprite->getWidth() - cel->getX() - image->getWidth());
          break;
      }
    }

    // for each stock's image
    for (int i=0; i<m_sprite->getStock()->size(); ++i) {
      Image* image = m_sprite->getStock()->getImage(i);
      if (!image)
        continue;

      // rotate the image
      Image* new_image = Image::create(image->getPixelFormat(),
                                       m_angle == 180 ? image->getWidth(): image->getHeight(),
                                       m_angle == 180 ? image->getHeight(): image->getWidth());
      raster::rotate_image(image, new_image, m_angle);

      api.replaceStockImage(m_sprite, i, new_image);

      jobProgress((float)i / m_sprite->getStock()->size());

      // cancel all the operation?
      if (isCanceled())
        return;        // UndoTransaction destructor will undo all operations
    }

    // rotate mask
    if (m_document->isMaskVisible()) {
      Mask* origMask = m_document->getMask();
      base::UniquePtr<Mask> new_mask(new Mask());
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
      raster::rotate_image(origMask->getBitmap(), new_mask->getBitmap(), m_angle);

      // Copy new mask
      api.copyToCurrentMask(new_mask);

      // Regenerate mask
      m_document->resetTransformation();
      m_document->generateMaskBoundaries();
    }

    // change the sprite's size
    if (m_angle != 180)
      api.setSpriteSize(m_sprite, m_sprite->getHeight(), m_sprite->getWidth());

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
  ContextReader reader(context);
  {
    RotateCanvasJob job(reader, m_angle);
    job.startJob();
  }
  reader.document()->generateMaskBoundaries();
  update_screen_for_document(reader.document());
}

Command* CommandFactory::createRotateCanvasCommand()
{
  return new RotateCanvasCommand;
}

} // namespace app
