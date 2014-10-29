/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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
#include "app/document_range.h"
#include "app/job.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/undo_transaction.h"
#include "app/util/range_utils.h"
#include "base/convert_to.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/ui.h"

#include <allegro/unicode.h>

namespace app {

class RotateCommand : public Command {
public:
  RotateCommand();
  Command* clone() const override { return new RotateCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);
  std::string onGetFriendlyName() const;

private:
  bool m_flipMask;
  int m_angle;
};

class RotateJob : public Job
{
  ContextWriter m_writer;
  Document* m_document;
  Sprite* m_sprite;
  int m_angle;
  CelList m_cels;
  bool m_rotateSprite;

public:

  RotateJob(const ContextReader& reader, int angle, const CelList& cels, bool rotateSprite)
    : Job("Rotate Canvas")
    , m_writer(reader)
    , m_document(m_writer.document())
    , m_sprite(m_writer.sprite())
    , m_cels(cels)
    , m_rotateSprite(rotateSprite)
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

    // for each cel...
    int i = 0;
    for (Cel* cel : m_cels) {
      Image* image = cel->image();
      if (image) {
        // change it location
        switch (m_angle) {
          case 180:
            api.setCelPosition(m_sprite, cel,
              m_sprite->width() - cel->x() - image->width(),
              m_sprite->height() - cel->y() - image->height());
            break;
          case 90:
            api.setCelPosition(m_sprite, cel,
              m_sprite->height() - cel->y() - image->height(),
              cel->x());
            break;
          case -90:
            api.setCelPosition(m_sprite, cel,
              cel->y(),
              m_sprite->width() - cel->x() - image->width());
            break;
        }

        // rotate the image
        Image* new_image = Image::create(image->pixelFormat(),
          m_angle == 180 ? image->width(): image->height(),
          m_angle == 180 ? image->height(): image->width());
        raster::rotate_image(image, new_image, m_angle);

        api.replaceStockImage(m_sprite, cel->imageIndex(), new_image);
      }

      jobProgress((float)i / m_cels.size());
      ++i;

      // cancel all the operation?
      if (isCanceled())
        return;        // UndoTransaction destructor will undo all operations
    }

    // rotate mask
    if (m_document->isMaskVisible()) {
      Mask* origMask = m_document->mask();
      base::UniquePtr<Mask> new_mask(new Mask());
      const gfx::Rect& origBounds = origMask->bounds();
      int x = 0, y = 0;

      switch (m_angle) {
        case 180:
          x = m_sprite->width() - origBounds.x - origBounds.w;
          y = m_sprite->height() - origBounds.y - origBounds.h;
          break;
        case 90:
          x = m_sprite->height() - origBounds.y - origBounds.h;
          y = origBounds.x;
          break;
        case -90:
          x = origBounds.y;
          y = m_sprite->width() - origBounds.x - origBounds.w;
          break;
      }

      // create the new rotated mask
      new_mask->replace(x, y,
                        m_angle == 180 ? origBounds.w: origBounds.h,
                        m_angle == 180 ? origBounds.h: origBounds.w);
      raster::rotate_image(origMask->bitmap(), new_mask->bitmap(), m_angle);

      // Copy new mask
      api.copyToCurrentMask(new_mask);

      // Regenerate mask
      m_document->resetTransformation();
      m_document->generateMaskBoundaries();
    }

    // change the sprite's size
    if (m_rotateSprite && m_angle != 180)
      api.setSpriteSize(m_sprite, m_sprite->height(), m_sprite->width());

    // commit changes
    undoTransaction.commit();
  }

};

RotateCommand::RotateCommand()
  : Command("Rotate",
            "Rotate Canvas",
            CmdRecordableFlag)
{
  m_flipMask = false;
  m_angle = 0;
}

void RotateCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  m_flipMask = (target == "mask");

  if (params->has_param("angle")) {
    m_angle = ustrtol(params->get("angle").c_str(), NULL, 10);
  }
}

bool RotateCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void RotateCommand::onExecute(Context* context)
{
  ContextReader reader(context);
  {
    CelList cels;
    bool rotateSprite = false;

    // Flip the mask or current cel
    if (m_flipMask) {
      DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
      if (range.enabled())
        cels = get_cels_in_range(reader.sprite(), range);
      else if (reader.cel())
        cels.push_back(reader.cel());
    }
    // Flip the whole sprite
    else if (reader.sprite()) {
      reader.sprite()->getCels(cels);
      rotateSprite = true;
    }

    if (cels.empty())           // Nothing to do
      return;

    RotateJob job(reader, m_angle, cels, rotateSprite);
    job.startJob();
    job.waitJob();
  }
  reader.document()->generateMaskBoundaries();
  update_screen_for_document(reader.document());
}

std::string RotateCommand::onGetFriendlyName() const
{
  std::string text = "Rotate";

  if (m_flipMask)
    text += " Selection";
  else
    text += " Sprite";

  text += " " + base::convert_to<std::string>(m_angle) + "\xc2\xb0";

  return text;
}

Command* CommandFactory::createRotateCommand()
{
  return new RotateCommand;
}

} // namespace app
