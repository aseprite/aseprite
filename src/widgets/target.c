/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <string.h>

#include "jinete/box.h"
#include "jinete/button.h"
#include "jinete/hook.h"
#include "jinete/widget.h"

#include "core/cfg.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "widgets/target.h"

#endif

static int channel_change(JWidget widget, int user_data);
static int images_change(JWidget widget, int user_data);
static int get_target_image_gfx(void);

/* creates a new button to handle "targets" to apply some effect in
   the sprite  */
JWidget target_button_new(int imgtype, bool with_channels)
{
#define ADD(box, widget, hook)						\
  if (widget) {								\
    jwidget_set_border (widget, 2, 2, 2, 2);				\
    jwidget_add_child (box, widget);					\
    HOOK (widget,							\
	  widget->type == JI_BUTTON ?					\
	  JI_SIGNAL_BUTTON_SELECT: JI_SIGNAL_CHECK_CHANGE, hook, vbox);	\
  }

  JWidget vbox, hbox;
  JWidget r=NULL, g=NULL, b=NULL, k=NULL, a=NULL, index=NULL, images=NULL;

  vbox = jbox_new (JI_VERTICAL);
  hbox = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);

  jwidget_noborders (vbox);
  jwidget_noborders (hbox);

  if (with_channels) {
    switch (imgtype) {

      case IMAGE_RGB:
      case IMAGE_INDEXED:
	r = check_button_new("R", 2, 0, 0, 0);
	g = check_button_new("G", 0, 0, 0, 0);
	b = check_button_new("B", 0, (imgtype == IMAGE_RGB) ? 0: 2, 0, 0);
	if (imgtype == IMAGE_RGB)
	  a = check_button_new("A", 0, 2, 0, 0);
	else
	  index = check_button_new("Index", 0, 0, 0, 0);

	if (get_config_bool("Target", "Red", TRUE)) jwidget_select(r);
	if (get_config_bool("Target", "Green", TRUE)) jwidget_select(g);
	if (get_config_bool("Target", "Blue", TRUE)) jwidget_select(b);
	if (a && get_config_bool("Target", "Alpha", TRUE)) jwidget_select(a);
	if (index && get_config_bool("Target", "Index", TRUE)) jwidget_select(index);
	break;

      case IMAGE_GRAYSCALE:
	k = check_button_new("K", 2, 0, 0, 0);
	a = check_button_new("A", 0, 2, 0, 0);

	if (get_config_bool("Target", "Gray", TRUE)) jwidget_select(k);
	if (get_config_bool("Target", "Alpha", TRUE)) jwidget_select(a);
	break;
    }
  }

  /* reset targets.  (It's important to start without image targets,
     because the most times we just want apply a effect to the current
     image) */
  set_config_int("Target", "Images", 0);

  /* create the button to select "image" target */
  images = jbutton_new(NULL);
  jbutton_set_bevel(images,
		      with_channels ? 0: 2,
		      with_channels ? 0: 2, 2, 2);
  add_gfxicon_to_button (images, get_target_image_gfx (),
			 JI_CENTER | JI_MIDDLE);

  /* make hierarchy */
  ADD(hbox, r, channel_change);
  ADD(hbox, g, channel_change);
  ADD(hbox, b, channel_change);
  ADD(hbox, k, channel_change);
  ADD(hbox, a, channel_change);

  if (with_channels)
    jwidget_add_child (vbox, hbox);
  else
    jwidget_free(hbox);

  ADD(vbox, index, channel_change);
  ADD(vbox, images, images_change);

  return vbox;
}

static int channel_change(JWidget widget, int user_data)
{
  const char *name;

  switch (*widget->text) {
    case 'R': name = "Red"; break;
    case 'G': name = "Green"; break;
    case 'B': name = "Blue"; break;
    case 'K': name = "Gray"; break;
    case 'A': name = "Alpha"; break;
    case 'I': name = "Index"; break;
    default:
      return TRUE;
  }

  set_config_bool("Target", name, jwidget_is_selected (widget));

  jwidget_emit_signal((JWidget)user_data, SIGNAL_TARGET_BUTTON_CHANGE);
  return TRUE;
}

static int images_change(JWidget widget, int user_data)
{
  int images = get_config_int("Target", "Images", 0);
  set_config_int("Target", "Images", (images+1)%4);
  set_gfxicon_in_button(widget, get_target_image_gfx ());

  jwidget_emit_signal((JWidget)user_data, SIGNAL_TARGET_BUTTON_CHANGE);
  return TRUE;
}

static int get_target_image_gfx(void)
{
  int images = get_config_int("Target", "Images", 0);

  switch (images) {
    case 1:
      return GFX_TARGET_FRAMES;
    case 2:
      return GFX_TARGET_LAYERS;
    case 3:
      return GFX_TARGET_FRAMES_LAYERS;
    default:
      return GFX_TARGET_ONE;
  }
}
