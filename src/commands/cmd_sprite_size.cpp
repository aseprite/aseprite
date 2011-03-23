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

#include "base/bind.h"
#include "commands/command.h"
#include "core/cfg.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "job.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "ui_context.h"
#include "undo_transaction.h"

#include <allegro/unicode.h>

#define PERC_FORMAT	"%.1f%%"

class SpriteSizeJob : public Job
{
  DocumentWriter m_document;
  Sprite* m_sprite;
  int m_new_width;
  int m_new_height;
  ResizeMethod m_resize_method;

  inline int scale_x(int x) const { return x * m_new_width / m_sprite->getWidth(); }
  inline int scale_y(int y) const { return y * m_new_height / m_sprite->getHeight(); }

public:

  SpriteSizeJob(const DocumentReader& document, int new_width, int new_height, ResizeMethod resize_method)
    : Job("Sprite Size")
    , m_document(document)
    , m_sprite(m_document->getSprite())
  {
    m_new_width = new_width;
    m_new_height = new_height;
    m_resize_method = resize_method;
  }

protected:

  /**
   * [working thread]
   */
  virtual void onJob()
  {
    UndoTransaction undoTransaction(m_document, "Sprite Size");

    // Get all sprite cels
    CelList cels;
    m_sprite->getCels(cels);

    // For each cel...
    int progress = 0;
    for (CelIterator it = cels.begin(); it != cels.end(); ++it, ++progress) {
      Cel* cel = *it;

      // Change its location
      undoTransaction.setCelPosition(cel, scale_x(cel->x), scale_y(cel->y));

      // Get cel's image
      Image* image = m_sprite->getStock()->getImage(cel->image);
      if (!image)
	continue;

      // Resize the image
      int w = scale_x(image->w);
      int h = scale_y(image->h);
      Image* new_image = image_new(image->imgtype, MAX(1, w), MAX(1, h));

      image_fixup_transparent_colors(image);
      image_resize(image, new_image,
		   m_resize_method,
		   m_sprite->getPalette(cel->frame),
		   m_sprite->getRgbMap(cel->frame));

      undoTransaction.replaceStockImage(cel->image, new_image);

      jobProgress((float)progress / cels.size());

      // cancel all the operation?
      if (isCanceled())
	return;	       // UndoTransaction destructor will undo all operations
    }

    // Resize mask
    if (m_document->isMaskVisible()) {
      Image* old_bitmap = image_crop(m_document->getMask()->bitmap, -1, -1,
				     m_document->getMask()->bitmap->w+2,
				     m_document->getMask()->bitmap->h+2, 0);

      int w = scale_x(old_bitmap->w);
      int h = scale_y(old_bitmap->h);
      Mask* new_mask = mask_new();
      mask_replace(new_mask,
		   scale_x(m_document->getMask()->x-1),
		   scale_y(m_document->getMask()->y-1), MAX(1, w), MAX(1, h));
      image_resize(old_bitmap, new_mask->bitmap,
		   m_resize_method,
		   m_sprite->getCurrentPalette(), // Ignored
		   m_sprite->getRgbMap());	  // Ignored
      image_free(old_bitmap);

      // reshrink
      mask_intersect(new_mask,
		     new_mask->x, new_mask->y,
		     new_mask->w, new_mask->h);

      // copy new mask
      undoTransaction.copyToCurrentMask(new_mask);
      mask_free(new_mask);

      // regenerate mask
      m_document->generateMaskBoundaries();
    }

    // resize sprite
    undoTransaction.setSpriteSize(m_new_width, m_new_height);

    // commit changes
    undoTransaction.commit();
  }

};

//////////////////////////////////////////////////////////////////////
// sprite_size

static bool width_px_change_hook(JWidget widget, void *data);
static bool height_px_change_hook(JWidget widget, void *data);
static bool width_perc_change_hook(JWidget widget, void *data);
static bool height_perc_change_hook(JWidget widget, void *data);

class SpriteSizeCommand : public Command

{
public:
  SpriteSizeCommand();
  Command* clone() { return new SpriteSizeCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  void onLockRatioClick();

  CheckBox* m_lockRatio;
};

SpriteSizeCommand::SpriteSizeCommand()
  : Command("SpriteSize",
	    "Sprite Size",
	    CmdRecordableFlag)
{
}

bool SpriteSizeCommand::onEnabled(Context* context)
{
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);
  return sprite != NULL;
}

void SpriteSizeCommand::onExecute(Context* context)
{
  JWidget width_px, height_px, width_perc, height_perc, ok;
  ComboBox* method;
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);

  // load the window widget
  FramePtr window(load_widget("sprite_size.xml", "sprite_size"));
  get_widgets(window,
	      "width_px", &width_px,
	      "height_px", &height_px,
	      "width_perc", &width_perc,
	      "height_perc", &height_perc,
	      "lock_ratio", &m_lockRatio,
	      "method", &method,
	      "ok", &ok, NULL);

  width_px->setTextf("%d", sprite->getWidth());
  height_px->setTextf("%d", sprite->getHeight());

  m_lockRatio->Click.connect(Bind<void>(&SpriteSizeCommand::onLockRatioClick, this));

  HOOK(width_px, JI_SIGNAL_ENTRY_CHANGE, width_px_change_hook, 0);
  HOOK(height_px, JI_SIGNAL_ENTRY_CHANGE, height_px_change_hook, 0);
  HOOK(width_perc, JI_SIGNAL_ENTRY_CHANGE, width_perc_change_hook, 0);
  HOOK(height_perc, JI_SIGNAL_ENTRY_CHANGE, height_perc_change_hook, 0);

  method->addItem("Nearest-neighbor");
  method->addItem("Bilinear");
  method->setSelectedItem(get_config_int("SpriteSize", "Method", RESIZE_METHOD_NEAREST_NEIGHBOR));

  window->remap_window();
  window->center_window();

  load_window_pos(window, "SpriteSize");
  window->setVisible(true);
  window->open_window_fg();
  save_window_pos(window, "SpriteSize");

  if (window->get_killer() == ok) {
    int new_width = width_px->getTextInt();
    int new_height = height_px->getTextInt();
    ResizeMethod resize_method =
      (ResizeMethod)method->getSelectedItem();

    set_config_int("SpriteSize", "Method", resize_method);

    {
      SpriteSizeJob job(document, new_width, new_height, resize_method);
      job.startJob();
    }

    update_screen_for_document(document);
  }
}

void SpriteSizeCommand::onLockRatioClick()
{
  const ActiveDocumentReader document(UIContext::instance()); // TODO use the context in sprite size command

  if (m_lockRatio->isSelected())
    width_px_change_hook(m_lockRatio->findSibling("width_px"), NULL);
}

static bool width_px_change_hook(JWidget widget, void *data)
{
  const ActiveDocumentReader document(UIContext::instance()); // TODO use the context in sprite size command
  const Sprite* sprite(document->getSprite());
  int width = widget->getTextInt();
  double perc = 100.0 * width / sprite->getWidth();

  widget->findSibling("width_perc")->setTextf(PERC_FORMAT, perc);

  if (widget->findSibling("lock_ratio")->isSelected()) {
    widget->findSibling("height_perc")->setTextf(PERC_FORMAT, perc);
    widget->findSibling("height_px")->setTextf("%d", sprite->getHeight() * width / sprite->getWidth());
  }

  return true;
}

static bool height_px_change_hook(JWidget widget, void *data)
{
  const ActiveDocumentReader document(UIContext::instance()); // TODO use the context in sprite size command
  const Sprite* sprite(document->getSprite());
  int height = widget->getTextInt();
  double perc = 100.0 * height / sprite->getHeight();

  widget->findSibling("height_perc")->setTextf(PERC_FORMAT, perc);

  if (widget->findSibling("lock_ratio")->isSelected()) {
    widget->findSibling("width_perc")->setTextf(PERC_FORMAT, perc);
    widget->findSibling("width_px")->setTextf("%d", sprite->getWidth() * height / sprite->getHeight());
  }

  return true;
}

static bool width_perc_change_hook(JWidget widget, void *data)
{
  const ActiveDocumentReader document(UIContext::instance()); // TODO use the context in sprite size command
  const Sprite* sprite(document->getSprite());
  double width = widget->getTextDouble();

  widget->findSibling("width_px")->setTextf("%d", (int)(sprite->getWidth() * width / 100));

  if (widget->findSibling("lock_ratio")->isSelected()) {
    widget->findSibling("height_px")->setTextf("%d", (int)(sprite->getHeight() * width / 100));
    widget->findSibling("height_perc")->setText(widget->getText());
  }

  return true;
}

static bool height_perc_change_hook(JWidget widget, void *data)
{
  const ActiveDocumentReader document(UIContext::instance()); // TODO use the context in sprite size command
  const Sprite* sprite(document->getSprite());
  double height = widget->getTextDouble();

  widget->findSibling("height_px")->setTextf("%d", (int)(sprite->getHeight() * height / 100));

  if (widget->findSibling("lock_ratio")->isSelected()) {
    widget->findSibling("width_px")->setTextf("%d", (int)(sprite->getWidth() * height / 100));
    widget->findSibling("width_perc")->setText(widget->getText());
  }

  return true;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createSpriteSizeCommand()
{
  return new SpriteSizeCommand;
}
