/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"

//////////////////////////////////////////////////////////////////////
// merge_down_layer

class MergeDownLayerCommand : public Command
{
public:
  MergeDownLayerCommand();
  Command* clone() { return new MergeDownLayerCommand(*this); }

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
  ActiveDocumentWriter document(context);
  Sprite* sprite(document ? document->getSprite(): 0);
  if (!sprite)
    return false;

  Layer* src_layer = sprite->getCurrentLayer();
  if (!src_layer || !src_layer->is_image())
    return false;

  Layer* dst_layer = sprite->getCurrentLayer()->get_prev();
  if (!dst_layer || !dst_layer->is_image())
    return false;

  return true;
}

void MergeDownLayerCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  undo::UndoHistory* undo(document->getUndoHistory());
  Layer *src_layer, *dst_layer;
  Cel *src_cel, *dst_cel;
  Image *src_image, *dst_image;
  int frpos, index;

  src_layer = sprite->getCurrentLayer();
  dst_layer = sprite->getCurrentLayer()->get_prev();

  if (undo->isEnabled()) {
    undo->setLabel("Merge Down Layer");
    undo->setModification(undo::ModifyDocument);
    undo->undo_open();
  }

  for (frpos=0; frpos<sprite->getTotalFrames(); ++frpos) {
    /* get frames */
    src_cel = static_cast<LayerImage*>(src_layer)->getCel(frpos);
    dst_cel = static_cast<LayerImage*>(dst_layer)->getCel(frpos);

    /* get images */
    if (src_cel != NULL)
      src_image = sprite->getStock()->getImage(src_cel->image);
    else
      src_image = NULL;

    if (dst_cel != NULL)
      dst_image = sprite->getStock()->getImage(dst_cel->image);
    else
      dst_image = NULL;

    /* with source image? */
    if (src_image != NULL) {
      /* no destination image */
      if (dst_image == NULL) {	/* only a transparent layer can have a null cel */
	/* copy this cel to the destination layer... */

	/* creating a copy of the image */
	dst_image = image_new_copy(src_image);

	/* adding it in the stock of images */
	index = sprite->getStock()->addImage(dst_image);
	if (undo->isEnabled())
	  undo->undo_add_image(sprite->getStock(), index);

	/* creating a copy of the cell */
	dst_cel = cel_new(frpos, index);
	cel_set_position(dst_cel, src_cel->x, src_cel->y);
	cel_set_opacity(dst_cel, src_cel->opacity);

	if (undo->isEnabled())
	  undo->undo_add_cel(dst_layer, dst_cel);

	static_cast<LayerImage*>(dst_layer)->addCel(dst_cel);
      }
      /* with destination */
      else {
	int x1, y1, x2, y2, bgcolor;
	Image *new_image;

	/* merge down in the background layer */
	if (dst_layer->is_background()) {
	  x1 = 0;
	  y1 = 0;
	  x2 = sprite->getWidth();
	  y2 = sprite->getHeight();
	  bgcolor = app_get_color_to_clear_layer(dst_layer);
	}
	/* merge down in a transparent layer */
	else {
	  x1 = MIN(src_cel->x, dst_cel->x);
	  y1 = MIN(src_cel->y, dst_cel->y);
	  x2 = MAX(src_cel->x+src_image->w-1, dst_cel->x+dst_image->w-1);
	  y2 = MAX(src_cel->y+src_image->h-1, dst_cel->y+dst_image->h-1);
	  bgcolor = 0;
	}

	new_image = image_crop(dst_image,
			       x1-dst_cel->x,
			       y1-dst_cel->y,
			       x2-x1+1, y2-y1+1, bgcolor);

	/* merge src_image in new_image */
	image_merge(new_image, src_image,
		    src_cel->x-x1,
		    src_cel->y-y1,
		    src_cel->opacity,
		    static_cast<LayerImage*>(src_layer)->getBlendMode());

	if (undo->isEnabled()) {
	  undo->undo_int(dst_cel, &dst_cel->x);
	  undo->undo_int(dst_cel, &dst_cel->y);
	}

	cel_set_position(dst_cel, x1, y1);

	if (undo->isEnabled())
	  undo->undo_replace_image(sprite->getStock(), dst_cel->image);
	sprite->getStock()->replaceImage(dst_cel->image, new_image);

	image_free(dst_image);
      }
    }
  }

  if (undo->isEnabled()) {
    undo->undo_set_layer(sprite);
    undo->undo_remove_layer(src_layer);
    undo->undo_close();
  }

  sprite->setCurrentLayer(dst_layer);
  src_layer->get_parent()->remove_layer(src_layer);

  delete src_layer;

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createMergeDownLayerCommand()
{
  return new MergeDownLayerCommand;
}
