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

#include "jinete/message.h"
#include "jinete/widget.h"

#include "effect/effect.h"
#include "modules/render.h"
#include "raster/sprite.h"
#include "widgets/preview.h"

#endif

typedef struct Preview
{
  Effect *effect;
} Preview;

static bool preview_msg_proc (JWidget widget, JMessage msg);

JWidget preview_new (Effect *effect)
{
  JWidget widget = jwidget_new (preview_type ());
  Preview *preview = jnew (Preview, 1);

  preview->effect = effect;

  jwidget_add_hook (widget, preview_type (), preview_msg_proc, preview);
  jwidget_hide (widget);

  return widget;
}

int preview_type (void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

void preview_restart (JWidget widget)
{
  effect_begin_for_preview (preview_get_effect (widget));
}

Effect *preview_get_effect (JWidget widget)
{
  return ((Preview *)jwidget_get_data (widget, preview_type ()))->effect;
}

static bool preview_msg_proc (JWidget widget, JMessage msg)
{
  Effect *effect = preview_get_effect (widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree (jwidget_get_data (widget, preview_type ()));
      break;

    case JM_OPEN:
      set_preview_image (effect->sprite->layer, effect->dst);
      break;

    case JM_CLOSE:
      set_preview_image (NULL, NULL);
      break;

    case JM_IDLE:
      if (effect) {
	if (effect_apply_step (effect))
	  effect_flush (effect);
      }
      break;
  }

  return FALSE;
}
