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

#include <allegro.h>
#include <string.h>

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jhook.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

#include "core/cfg.h"
#include "effect/effect.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "widgets/target.h"

static bool channel_change_hook(JWidget widget, void *data);
static bool images_change_hook(JWidget widget, void *data);
static int get_target_image_gfx(int target);

/**
 * Creates a new button to handle "targets" to apply some effect in
 * the sprite
 *
 * user_data[0] = target flags (TARGET_RED_CHANNEL, etc.)
 */
JWidget target_button_new(int imgtype, bool with_channels)
{
#define ADD(box, widget, hook)						\
  if (widget) {								\
    jwidget_set_border(widget, 2 * jguiscale());			\
    jwidget_add_child(box, widget);					\
    HOOK(widget,							\
	 widget->type == JI_BUTTON ?					\
	 JI_SIGNAL_BUTTON_SELECT: JI_SIGNAL_CHECK_CHANGE, hook, vbox);	\
  }

  int default_targets = 0;
  JWidget vbox, hbox;
  JWidget r=NULL, g=NULL, b=NULL, k=NULL, a=NULL, index=NULL, images=NULL;

  vbox = jbox_new(JI_VERTICAL);
  hbox = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);

  jwidget_noborders(vbox);
  jwidget_noborders(hbox);

  if (with_channels) {
    switch (imgtype) {

      case IMAGE_RGB:
      case IMAGE_INDEXED:
	r = check_button_new("R", 2, 0, 0, 0);
	g = check_button_new("G", 0, 0, 0, 0);
	b = check_button_new("B", 0, (imgtype == IMAGE_RGB) ? 0: 2, 0, 0);

	r->setName("r");
	g->setName("g");
	b->setName("b");
	
	if (imgtype == IMAGE_RGB) {
	  a = check_button_new("A", 0, 2, 0, 0);
	  a->setName("a");
	}
	else {
	  index = check_button_new("Index", 0, 0, 0, 0);
	  index->setName("i");
	}
	break;

      case IMAGE_GRAYSCALE:
	k = check_button_new("K", 2, 0, 0, 0);
	a = check_button_new("A", 0, 2, 0, 0);

	k->setName("k");
	a->setName("a");
	break;
    }
  }

  /* create the button to select "image" target */
  images = jbutton_new(NULL);
  jbutton_set_bevel(images,
		    with_channels ? 0: 2,
		    with_channels ? 0: 2, 2, 2);
  setup_mini_look(images);
  add_gfxicon_to_button(images, get_target_image_gfx(default_targets),
			JI_CENTER | JI_MIDDLE);

  /* make hierarchy */
  ADD(hbox, r, channel_change_hook);
  ADD(hbox, g, channel_change_hook);
  ADD(hbox, b, channel_change_hook);
  ADD(hbox, k, channel_change_hook);
  ADD(hbox, a, channel_change_hook);

  if (with_channels)
    jwidget_add_child(vbox, hbox);
  else
    jwidget_free(hbox);

  ADD(vbox, index, channel_change_hook);
  ADD(vbox, images, images_change_hook);

  vbox->user_data[0] = (void *)default_targets;
  return vbox;
}

int target_button_get_target(JWidget widget)
{
  return (size_t)widget->user_data[0];
}

void target_button_set_target(JWidget widget, int target)
{
  JWidget w;

#define ACTIVATE_TARGET(name, TARGET)					\
  w = jwidget_find_name(widget, name);					\
  if (w != NULL) {							\
    w->setSelected((target & TARGET) == TARGET);			\
  }

  ACTIVATE_TARGET("r", TARGET_RED_CHANNEL);
  ACTIVATE_TARGET("g", TARGET_GREEN_CHANNEL);
  ACTIVATE_TARGET("b", TARGET_BLUE_CHANNEL);
  ACTIVATE_TARGET("a", TARGET_ALPHA_CHANNEL);
  ACTIVATE_TARGET("k", TARGET_GRAY_CHANNEL);
  ACTIVATE_TARGET("i", TARGET_INDEX_CHANNEL);

  widget->user_data[0] = (void *)target;
}

static bool channel_change_hook(JWidget widget, void *data)
{
  JWidget target_button = (JWidget)data;
  int target = (size_t)target_button->user_data[0];
  int flag = 0;
  
  switch (*widget->name) {
    case 'r': flag = TARGET_RED_CHANNEL; break;
    case 'g': flag = TARGET_GREEN_CHANNEL; break;
    case 'b': flag = TARGET_BLUE_CHANNEL; break;
    case 'k': flag = TARGET_GRAY_CHANNEL; break;
    case 'a': flag = TARGET_ALPHA_CHANNEL; break;
    case 'i': flag = TARGET_INDEX_CHANNEL; break;
    default:
      return true;
  }

  if (widget->isSelected())
    target |= flag;
  else
    target &= ~flag;

  target_button->user_data[0] = (void *)target;
  
  jwidget_emit_signal(target_button, SIGNAL_TARGET_BUTTON_CHANGE);
  return true;
}

static bool images_change_hook(JWidget widget, void *data)
{
  JWidget target_button = (JWidget)data;
  int target = (size_t)target_button->user_data[0];

  /* rotate target */
  if (target & TARGET_ALL_FRAMES) {
    target &= ~TARGET_ALL_FRAMES;

    if (target & TARGET_ALL_LAYERS)
      target &= ~TARGET_ALL_LAYERS;
    else
      target |= TARGET_ALL_LAYERS;
  }
  else {
    target |= TARGET_ALL_FRAMES;
  }

  set_gfxicon_in_button(widget, get_target_image_gfx(target));

  target_button->user_data[0] = (void *)target;
  jwidget_emit_signal(target_button, SIGNAL_TARGET_BUTTON_CHANGE);
  return true;
}

static int get_target_image_gfx(int target)
{
  if (target & TARGET_ALL_FRAMES) {
    return target & TARGET_ALL_LAYERS ?
      GFX_TARGET_FRAMES_LAYERS:
      GFX_TARGET_FRAMES;
  }
  else {
    return target & TARGET_ALL_LAYERS ?
      GFX_TARGET_LAYERS:
      GFX_TARGET_ONE;
  }
}
