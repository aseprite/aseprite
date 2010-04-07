/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "core/cfg.h"
#include "core/job.h"
#include "commands/command.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undoable.h"
#include "ui_context.h"
#include "sprite_wrappers.h"

#define PERC_FORMAT	"%.1f%%"

class SpriteSizeJob : public Job
{
  SpriteWriter m_sprite;
  int m_new_width;
  int m_new_height;
  ResizeMethod m_resize_method;

  inline int scale_x(int x) const { return x * m_new_width / m_sprite->getWidth(); }
  inline int scale_y(int y) const { return y * m_new_height / m_sprite->getHeight(); }

public:

  SpriteSizeJob(const SpriteReader& sprite, int new_width, int new_height, ResizeMethod resize_method)
    : Job("Sprite Size")
    , m_sprite(sprite)
  {
    m_new_width = new_width;
    m_new_height = new_height;
    m_resize_method = resize_method;
  }

protected:

  /**
   * [working thread]
   */
  virtual void on_job()
  {
    Undoable undoable(m_sprite, "Sprite Size");

    // get all sprite cels
    CelList cels;
    m_sprite->getCels(cels);

    // for each cel...
    for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
      Cel* cel = *it;

      // change it location
      undoable.set_cel_position(cel, scale_x(cel->x), scale_y(cel->y));
    }

    // for each stock's image
    for (int i=0; i<m_sprite->getStock()->nimage; ++i) {
      Image* image = stock_get_image(m_sprite->getStock(), i);
      if (!image)
	continue;

      // resize the image
      int w = scale_x(image->w);
      int h = scale_y(image->h);
      Image* new_image = image_new(image->imgtype, MAX(1, w), MAX(1, h));

      image_fixup_transparent_colors(image);
      image_resize(image, new_image,
		   m_resize_method,
		   get_current_palette(),
		   orig_rgb_map);

      undoable.replace_stock_image(i, new_image);

      job_progress((float)i / m_sprite->getStock()->nimage);

      // cancel all the operation?
      if (is_canceled())
	return;	       // Undoable destructor will undo all operations
    }

    // resize mask
    if (m_sprite->getMask()->bitmap) {
      Image* old_bitmap = image_crop(m_sprite->getMask()->bitmap, -1, -1,
				     m_sprite->getMask()->bitmap->w+2,
				     m_sprite->getMask()->bitmap->h+2, 0);

      int w = scale_x(old_bitmap->w);
      int h = scale_y(old_bitmap->h);
      Mask* new_mask = mask_new();
      mask_replace(new_mask,
		   scale_x(m_sprite->getMask()->x-1),
		   scale_y(m_sprite->getMask()->y-1), MAX(1, w), MAX(1, h));
      image_resize(old_bitmap, new_mask->bitmap,
		   m_resize_method,
		   get_current_palette(),
		   orig_rgb_map);
      image_free(old_bitmap);

      // reshrink
      mask_intersect(new_mask,
		     new_mask->x, new_mask->y,
		     new_mask->w, new_mask->h);

      // copy new mask
      undoable.copy_to_current_mask(new_mask);
      mask_free(new_mask);

      // regenerate mask
      m_sprite->generateMaskBoundaries();
    }

    // resize sprite
    undoable.set_sprite_size(m_new_width, m_new_height);

    // commit changes
    undoable.commit();
  }

};

static bool lock_ratio_change_hook(JWidget widget, void *data);
static bool width_px_change_hook(JWidget widget, void *data);
static bool height_px_change_hook(JWidget widget, void *data);
static bool width_perc_change_hook(JWidget widget, void *data);
static bool height_perc_change_hook(JWidget widget, void *data);

//////////////////////////////////////////////////////////////////////
// sprite_size

class SpriteSizeCommand : public Command

{
public:
  SpriteSizeCommand();
  Command* clone() { return new SpriteSizeCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

SpriteSizeCommand::SpriteSizeCommand()
  : Command("sprite_size",
	    "Sprite Size",
	    CmdRecordableFlag)
{
}

bool SpriteSizeCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void SpriteSizeCommand::execute(Context* context)
{
  JWidget width_px, height_px, width_perc, height_perc, lock_ratio, method, ok;
  const CurrentSpriteReader sprite(context);

  // load the window widget
  FramePtr window(load_widget("sprite_size.xml", "sprite_size"));
  get_widgets(window,
	      "width_px", &width_px,
	      "height_px", &height_px,
	      "width_perc", &width_perc,
	      "height_perc", &height_perc,
	      "lock_ratio", &lock_ratio,
	      "method", &method,
	      "ok", &ok, NULL);

  width_px->setTextf("%d", sprite->getWidth());
  height_px->setTextf("%d", sprite->getHeight());

  HOOK(lock_ratio, JI_SIGNAL_CHECK_CHANGE, lock_ratio_change_hook, 0);
  HOOK(width_px, JI_SIGNAL_ENTRY_CHANGE, width_px_change_hook, 0);
  HOOK(height_px, JI_SIGNAL_ENTRY_CHANGE, height_px_change_hook, 0);
  HOOK(width_perc, JI_SIGNAL_ENTRY_CHANGE, width_perc_change_hook, 0);
  HOOK(height_perc, JI_SIGNAL_ENTRY_CHANGE, height_perc_change_hook, 0);

  jcombobox_add_string(method, "Nearest-neighbor", NULL);
  jcombobox_add_string(method, "Bilinear", NULL);
  jcombobox_select_index(method, get_config_int("SpriteSize", "Method", RESIZE_METHOD_NEAREST_NEIGHBOR));

  window->remap_window();
  window->center_window();

  load_window_pos(window, "SpriteSize");
  jwidget_show(window);
  window->open_window_fg();
  save_window_pos(window, "SpriteSize");

  if (window->get_killer() == ok) {
    int new_width = width_px->getTextInt();
    int new_height = height_px->getTextInt();
    ResizeMethod resize_method =
      (ResizeMethod)jcombobox_get_selected_index(method);

    set_config_int("SpriteSize", "Method", resize_method);

    {
      SpriteSizeJob job(sprite, new_width, new_height, resize_method);
      job.do_job();
    }

    update_screen_for_sprite(sprite);
  }
}

static bool lock_ratio_change_hook(JWidget widget, void *data)
{
  const CurrentSpriteReader sprite(UIContext::instance()); // TODO use the context in sprite size command

  if (widget->isSelected())
    width_px_change_hook(widget->findSibling("width_px"), NULL);

  return true;
}

static bool width_px_change_hook(JWidget widget, void *data)
{
  const CurrentSpriteReader sprite(UIContext::instance()); // TODO use the context in sprite size command
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
  const CurrentSpriteReader sprite(UIContext::instance()); // TODO use the context in sprite size command
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
  const CurrentSpriteReader sprite(UIContext::instance()); // TODO use the context in sprite size command
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
  const CurrentSpriteReader sprite(UIContext::instance()); // TODO use the context in sprite size command
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

Command* CommandFactory::create_sprite_size_command()
{
  return new SpriteSizeCommand;
}
