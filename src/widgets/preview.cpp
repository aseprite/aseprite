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

#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jwidget.h"

#include "effect/effect.h"
#include "raster/sprite.h"
#include "util/render.h"
#include "widgets/preview.h"

typedef struct Preview
{
  Effect *effect;
  int timer_id;
} Preview;

static bool preview_msg_proc(JWidget widget, JMessage msg);

/**
 * Invisible widget to control a effect-preview.
 */
JWidget preview_new(Effect *effect)
{
  Widget* widget = new Widget(preview_type());
  Preview *preview = jnew(Preview, 1);

  preview->effect = effect;
  preview->timer_id = -1;

  jwidget_add_hook(widget, preview_type(), preview_msg_proc, preview);
  widget->setVisible(false);

  return widget;
}

int preview_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

void preview_restart(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  Preview* preview = reinterpret_cast<Preview*>(jwidget_get_data(widget, preview_type()));

  effect_begin_for_preview(preview->effect);

  if (preview->timer_id < 0)
    preview->timer_id = jmanager_add_timer(widget, 1);
  jmanager_start_timer(preview->timer_id);
}

Effect *preview_get_effect(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  Preview* preview = reinterpret_cast<Preview*>(jwidget_get_data(widget, preview_type()));
  return preview->effect;
}

static bool preview_msg_proc(JWidget widget, JMessage msg)
{
  Preview* preview = reinterpret_cast<Preview*>(jwidget_get_data(widget, preview_type()));
  Effect* effect = preview_get_effect(widget);

  switch (msg->type) {

    case JM_DESTROY:
      if (preview->timer_id >= 0)
	jmanager_remove_timer(preview->timer_id);
      jfree(preview);
      break;
      
    case JM_OPEN:
      RenderEngine::setPreviewImage(effect->sprite->getCurrentLayer(), effect->dst);
      break;

    case JM_CLOSE:
      RenderEngine::setPreviewImage(NULL, NULL);
      /* stop the preview timer */
      jmanager_stop_timer(preview->timer_id);
      break;

    case JM_TIMER:
      if (effect) {
	if (effect_apply_step(effect))
	  effect_flush(effect);
	else
	  jmanager_stop_timer(preview->timer_id);
      }
      break;
  }

  return false;
}
