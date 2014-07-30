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
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/undo_transaction.h"
#include "app/undoers/add_cel.h"
#include "app/undoers/add_image.h"
#include "app/undoers/remove_layer.h"
#include "app/undoers/replace_image.h"
#include "app/undoers/set_cel_position.h"
#include "base/unique_ptr.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/primitives.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui/ui.h"

namespace app {

class MergeDownLayerCommand : public Command {
public:
  MergeDownLayerCommand();
  Command* clone() const OVERRIDE { return new MergeDownLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

MergeDownLayerCommand::MergeDownLayerCommand()
  : Command("MergeDownLayer",
            "Merge Down Layer",
            CmdRecordableFlag)
{
}

bool MergeDownLayerCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Sprite* sprite(writer.sprite());
  if (!sprite)
    return false;

  Layer* src_layer = writer.layer();
  if (!src_layer || !src_layer->isImage())
    return false;

  Layer* dst_layer = src_layer->getPrevious();
  if (!dst_layer || !dst_layer->isImage())
    return false;

  return true;
}

void MergeDownLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  UndoTransaction undo(writer.context(), "Merge Down Layer", undo::ModifyDocument);
  Layer* src_layer = writer.layer();
  Layer* dst_layer = src_layer->getPrevious();
  int index;

  for (FrameNumber frpos(0); frpos<sprite->totalFrames(); ++frpos) {
    // Get frames
    Cel* src_cel = static_cast<LayerImage*>(src_layer)->getCel(frpos);
    Cel* dst_cel = static_cast<LayerImage*>(dst_layer)->getCel(frpos);

    // Get images
    Image* src_image;
    if (src_cel != NULL)
      src_image = src_cel->image();
    else
      src_image = NULL;

    Image* dst_image;
    if (dst_cel != NULL)
      dst_image = dst_cel->image();
    else
      dst_image = NULL;

    // With source image?
    if (src_image != NULL) {
      // No destination image
      if (dst_image == NULL) {  // Only a transparent layer can have a null cel
        // Copy this cel to the destination layer...

        // Creating a copy of the image
        dst_image = Image::createCopy(src_image);

        // Adding it in the stock of images
        index = sprite->stock()->addImage(dst_image);
        if (undo.isEnabled())
          undo.pushUndoer(new undoers::AddImage(
              undo.getObjects(), sprite->stock(), index));

        // Creating a copy of the cell
        dst_cel = new Cel(frpos, index);
        dst_cel->setPosition(src_cel->x(), src_cel->y());
        dst_cel->setOpacity(src_cel->opacity());

        if (undo.isEnabled())
          undo.pushUndoer(new undoers::AddCel(undo.getObjects(), dst_layer, dst_cel));

        static_cast<LayerImage*>(dst_layer)->addCel(dst_cel);
      }
      // With destination
      else {
        int x1, y1, x2, y2;

        // Merge down in the background layer
        if (dst_layer->isBackground()) {
          x1 = 0;
          y1 = 0;
          x2 = sprite->width();
          y2 = sprite->height();
        }
        // Merge down in a transparent layer
        else {
          x1 = MIN(src_cel->x(), dst_cel->x());
          y1 = MIN(src_cel->y(), dst_cel->y());
          x2 = MAX(src_cel->x()+src_image->width()-1, dst_cel->x()+dst_image->width()-1);
          y2 = MAX(src_cel->y()+src_image->height()-1, dst_cel->y()+dst_image->height()-1);
        }

        raster::color_t bgcolor = app_get_color_to_clear_layer(dst_layer);

        Image* new_image = raster::crop_image(dst_image,
          x1-dst_cel->x(),
          y1-dst_cel->y(),
          x2-x1+1, y2-y1+1, bgcolor);

        // Merge src_image in new_image
        raster::composite_image(new_image, src_image,
                                src_cel->x()-x1,
                                src_cel->y()-y1,
                                src_cel->opacity(),
                                static_cast<LayerImage*>(src_layer)->getBlendMode());

        if (undo.isEnabled())
          undo.pushUndoer(new undoers::SetCelPosition(undo.getObjects(), dst_cel));

        dst_cel->setPosition(x1, y1);

        if (undo.isEnabled())
          undo.pushUndoer(new undoers::ReplaceImage(undo.getObjects(),
              sprite->stock(), dst_cel->imageIndex()));

        sprite->stock()->replaceImage(dst_cel->imageIndex(), new_image);
        delete dst_image;
      }
    }
  }

  document->notifyLayerMergedDown(src_layer, dst_layer);
  document->getApi().removeLayer(src_layer); // src_layer is deleted inside removeLayer()

  undo.commit();
  update_screen_for_document(document);
}

Command* CommandFactory::createMergeDownLayerCommand()
{
  return new MergeDownLayerCommand;
}

} // namespace app
