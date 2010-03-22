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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "Vaca/Bind.h"
#include "jinete/jinete.h"

#include "app.h"
#include "commands/commands.h"
#include "core/core.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "sprite_wrappers.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "util/misc.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

enum {
  ACTION_LAYER,
  ACTION_FIRST,
  ACTION_PREV,
  ACTION_PLAY,
  ACTION_NEXT,
  ACTION_LAST,
};

static bool statusbar_msg_proc(JWidget widget, JMessage msg);
static bool tipwindow_msg_proc(JWidget widget, JMessage msg);

static bool slider_change_hook(JWidget widget, void *data);
static void button_command(JWidget widget, void *data);

static void update_from_layer(StatusBar *statusbar);

static void on_current_tool_change(JWidget widget)
{
  if (jwidget_is_visible(widget)) {
    Tool* currentTool = UIContext::instance()->getSettings()->getCurrentTool();
    if (currentTool)
      statusbar_set_text(widget, 500, "%s selected",
			 currentTool->getText().c_str());
  }
}

JWidget statusbar_new()
{
#define BUTTON_NEW(name, text, data)					\
  {									\
    (name) = jbutton_new(text);						\
    (name)->user_data[0] = statusbar;					\
    setup_mini_look(name);						\
    jbutton_add_command_data((name), button_command, (void *)(data));	\
  }

#define ICON_NEW(name, icon, action)					\
  {									\
    BUTTON_NEW((name), NULL, (action));					\
    add_gfxicon_to_button((name), (icon), JI_CENTER | JI_MIDDLE);	\
  }

  Widget* widget = new Widget(statusbar_type());
  StatusBar* statusbar = jnew(StatusBar, 1);

  jwidget_add_hook(widget, statusbar_type(),
		   statusbar_msg_proc, statusbar);
  jwidget_focusrest(widget, true);

  {
    JWidget box1, box2;

    statusbar->widget = widget;
    statusbar->timeout = 0;
    statusbar->progress = jlist_new();
    statusbar->tipwindow = NULL;

    /* construct the commands box */
    box1 = jbox_new(JI_HORIZONTAL);
    box2 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
    BUTTON_NEW(statusbar->b_layer, "*Current Layer*", ACTION_LAYER);
    statusbar->slider = jslider_new(0, 255, 255);

    setup_mini_look(statusbar->slider);

    ICON_NEW(statusbar->b_first, GFX_ANI_FIRST, ACTION_FIRST);
    ICON_NEW(statusbar->b_prev, GFX_ANI_PREV, ACTION_PREV);
    ICON_NEW(statusbar->b_play, GFX_ANI_PLAY, ACTION_PLAY);
    ICON_NEW(statusbar->b_next, GFX_ANI_NEXT, ACTION_NEXT);
    ICON_NEW(statusbar->b_last, GFX_ANI_LAST, ACTION_LAST);

    HOOK(statusbar->slider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
    jwidget_set_min_size(statusbar->slider, JI_SCREEN_W/5, 0);

    jwidget_set_border(box1, 2*jguiscale(), 1*jguiscale(), 2*jguiscale(), 2*jguiscale());
    jwidget_noborders(box2);

    jwidget_expansive(statusbar->b_layer, true);
    jwidget_add_child(box1, statusbar->b_layer);
    jwidget_add_child(box1, statusbar->slider);
    jwidget_add_child(box2, statusbar->b_first);
    jwidget_add_child(box2, statusbar->b_prev);
    jwidget_add_child(box2, statusbar->b_play);
    jwidget_add_child(box2, statusbar->b_next);
    jwidget_add_child(box2, statusbar->b_last);
    jwidget_add_child(box1, box2);

    statusbar->commands_box = box1;

    App::instance()->CurrentToolChange.connect(Vaca::Bind<void>(&on_current_tool_change, widget));
  }

  return widget;
}

int statusbar_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

StatusBar *statusbar_data(JWidget widget)
{
  return reinterpret_cast<StatusBar*>(jwidget_get_data(widget, statusbar_type()));
}

void statusbar_set_text(JWidget widget, int msecs, const char *format, ...)
{
  StatusBar *statusbar = statusbar_data(widget);

  if ((ji_clock > statusbar->timeout) || (msecs > 0)) {
    char buf[256];		/* TODO warning buffer overflow */
    va_list ap;

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    widget->setText(buf);
    statusbar->timeout = ji_clock + msecs;
    jwidget_dirty(widget);
  }
}

void statusbar_show_tip(JWidget widget, int msecs, const char *format, ...)
{
  StatusBar *statusbar = statusbar_data(widget);
  Frame* tipwindow = statusbar->tipwindow;
  char buf[256];		/* TODO warning buffer overflow */
  va_list ap;
  int x, y;

  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  if (tipwindow == NULL) {
    tipwindow = new TipWindow(buf);
    tipwindow->user_data[0] = (void *)jmanager_add_timer(tipwindow, msecs);
    tipwindow->user_data[1] = statusbar;
    jwidget_add_hook(tipwindow, -1, tipwindow_msg_proc, NULL);

    statusbar->tipwindow = tipwindow;
  }
  else {
    tipwindow->setText(buf);

    jmanager_set_timer_interval((size_t)tipwindow->user_data[0], msecs);
  }

  if (jwidget_is_visible(tipwindow))
    tipwindow->closeWindow(NULL);

  tipwindow->open_window();
  tipwindow->remap_window();

  x = widget->rc->x2 - jrect_w(tipwindow->rc);
  y = widget->rc->y1 - jrect_h(tipwindow->rc);
  tipwindow->position_window(x, y);

  jmanager_start_timer((size_t)tipwindow->user_data[0]);
}

void statusbar_show_color(JWidget widget, int msecs, int imgtype, color_t color)
{
  char buf[128];		/* TODO warning buffer overflow */
  color_to_formalstring(imgtype, color, buf, sizeof(buf), true);
  statusbar_set_text(widget, msecs, "%s %s", _("Color"), buf);
}

Progress *progress_new(JWidget widget)
{
  Progress *progress = jnew(Progress, 1);
  if (!progress)
    return NULL;

  progress->statusbar = widget;
  progress->pos = 0.0f;

  jlist_append(statusbar_data(widget)->progress, progress);
  jwidget_dirty(widget);

  return progress;
}

void progress_free(Progress *progress)
{
  jlist_remove(statusbar_data(progress->statusbar)->progress,
	       progress);
  jwidget_dirty(progress->statusbar);

  jfree(progress);
}

void progress_update(Progress *progress, float progress_pos)
{
  if (progress->pos != progress_pos) {
    progress->pos = progress_pos;
    jwidget_dirty(progress->statusbar);
  }
}

static bool statusbar_msg_proc(JWidget widget, JMessage msg)
{
  StatusBar *statusbar = statusbar_data(widget);

  switch (msg->type) {

    case JM_DESTROY: {
      JLink link;

      JI_LIST_FOR_EACH(statusbar->progress, link) {
	jfree(link->data);
      }
      jlist_free(statusbar->progress);

      if (statusbar->tipwindow != NULL)
	jwidget_free(statusbar->tipwindow);

      jfree(statusbar);
      break;
    }

    case JM_REQSIZE:
      msg->reqsize.w = msg->reqsize.h =
	4*jguiscale()
	+ jwidget_get_text_height(widget)
	+ 4*jguiscale();
      return true;

    case JM_SETPOS:
      jrect_copy(widget->rc, &msg->setpos.rect);
      jwidget_set_rect(statusbar->commands_box, widget->rc);
      return true;

    case JM_CLOSE:
      if (!jwidget_has_child(widget, statusbar->commands_box)) {
	/* append the "commands_box" to destroy it in the jwidget_free */
	jwidget_add_child(widget, statusbar->commands_box);
      }
      break;

    case JM_DRAW: {
      JRect rc = jwidget_get_rect(widget);
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      jrect_displace(rc,
		     -msg->draw.rect.x1,
		     -msg->draw.rect.y1);

      clear_to_color(doublebuffer, ji_color_face());

      rc->x1 += 2*jguiscale();
      rc->y1 += 1*jguiscale();
      rc->x2 -= 2*jguiscale();
      rc->y2 -= 2*jguiscale();

      /* status bar text */
      if (widget->getText()) {
	rectfill(doublebuffer,
		 rc->x1, rc->y1, rc->x2-1, rc->y2-1, ji_color_face());

	textout_ex(doublebuffer, widget->getFont(), widget->getText(),
		   rc->x1+2,
		   (rc->y1+rc->y2)/2-text_height(widget->getFont())/2,
		   ji_color_foreground(), -1);
      }

      /* draw progress bar */
      if (!jlist_empty(statusbar->progress)) {
	int width = 64;
	int y1, y2;
	int x = rc->x2 - (width+4);
	JLink link;

	y1 = rc->y1;
	y2 = rc->y2-1;

	JI_LIST_FOR_EACH(statusbar->progress, link) {
	  Progress* progress = reinterpret_cast<Progress*>(link->data);

	  draw_progress_bar(doublebuffer,
			    x, y1, x+width-1, y2,
			    progress->pos);

	  x -= width+4;
	}
      }
      /* draw current sprite size in memory */
      else {
	char buf[1024];
	try {
	  const CurrentSpriteReader sprite(UIContext::instance());
	  if (sprite) {
	    ustrcpy(buf, "Sprite:");
	    get_pretty_memsize(sprite_get_memsize(sprite),
			       buf+ustrsize(buf),
			       sizeof(buf)-ustrsize(buf));

	    ustrcat(buf, " Undo:");
	    get_pretty_memsize(undo_get_memsize(sprite->undo),
			       buf+ustrsize(buf),
			       sizeof(buf)-ustrsize(buf));
	  }
	  else {
	    ustrcpy(buf, "No Sprite");
	  }
	}
	catch (locked_sprite_exception&) {
	  ustrcpy(buf, "Sprite is Locked");
	}

	textout_right_ex(doublebuffer, widget->getFont(), buf,
			 rc->x2-2,
			 (rc->y1+rc->y2)/2-text_height(widget->getFont())/2,
			 ji_color_foreground(), -1);
      }

      jrect_free(rc);

      blit(doublebuffer, ji_screen, 0, 0,
	   msg->draw.rect.x1,
	   msg->draw.rect.y1,
	   doublebuffer->w,
	   doublebuffer->h);
      destroy_bitmap(doublebuffer);
      return true;
    }

    case JM_MOUSEENTER:
      if (!jwidget_has_child(widget, statusbar->commands_box)) {
	bool state = (UIContext::instance()->get_current_sprite() != NULL);

	statusbar->b_first->setEnabled(state);
	statusbar->b_prev->setEnabled(state);
	statusbar->b_play->setEnabled(state);
	statusbar->b_next->setEnabled(state);
	statusbar->b_last->setEnabled(state);

	update_from_layer(statusbar);

	jwidget_add_child(widget, statusbar->commands_box);
	jwidget_dirty(widget);
      }
      break;

    case JM_MOUSELEAVE:
      if (jwidget_has_child(widget, statusbar->commands_box)) {
	/* if we want restore the state-bar and the slider doesn't have
	   the capture... */
	if (jmanager_get_capture() != statusbar->slider) {
	  /* exit from command mode */
	  jmanager_free_focus();

	  jwidget_remove_child(widget, statusbar->commands_box);
	  jwidget_dirty(widget);
	}
      }
      break;
  }

  return false;
}

static bool tipwindow_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY:
      jmanager_remove_timer((size_t)widget->user_data[0]);
      break;

    case JM_TIMER:
      static_cast<Frame*>(widget)->closeWindow(NULL);
      break;
  }

  return false;
}

static bool slider_change_hook(JWidget widget, void *data)
{
  try {
    CurrentSpriteWriter sprite(UIContext::instance());
    if (sprite) {
      if ((sprite->layer) &&
	  (sprite->layer->is_image())) {
	Cel* cel = ((LayerImage*)sprite->layer)->get_cel(sprite->frame);
	if (cel) {
	  // update the opacity
	  cel->opacity = jslider_get_value(widget);

	  // update the editors
	  update_screen_for_sprite(sprite);
	}
      }
    }
  }
  catch (locked_sprite_exception&) {
    // do nothing
  }
  return false;
}

static void button_command(JWidget widget, void *data)
{
  Command* cmd = NULL;

  switch ((size_t)data) {
    case ACTION_LAYER: cmd = CommandsModule::instance()->get_command_by_name(CommandId::layer_properties); break;
    case ACTION_FIRST: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_first_frame); break;
    case ACTION_PREV: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_previous_frame); break;
    case ACTION_PLAY: cmd = CommandsModule::instance()->get_command_by_name(CommandId::play_animation); break;
    case ACTION_NEXT: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_next_frame); break;
    case ACTION_LAST: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_last_frame); break;
  }

  if (cmd)
    UIContext::instance()->execute_command(cmd);
}

static void update_from_layer(StatusBar *statusbar)
{
  try {
    const CurrentSpriteReader sprite(UIContext::instance());
    Cel *cel;

    /* layer button */
    if (sprite && sprite->layer) {
      statusbar->b_layer->setTextf("[%d] %s", sprite->frame, sprite->layer->get_name().c_str());
      jwidget_enable(statusbar->b_layer);
    }
    else {
      statusbar->b_layer->setText("Nothing");
      jwidget_disable(statusbar->b_layer);
    }

    /* opacity layer */
    if (sprite &&
	sprite->layer &&
	sprite->layer->is_image() &&
	!sprite->layer->is_background() &&
	(cel = ((LayerImage*)sprite->layer)->get_cel(sprite->frame))) {
      jslider_set_value(statusbar->slider, MID(0, cel->opacity, 255));
      jwidget_enable(statusbar->slider);
    }
    else {
      jslider_set_value(statusbar->slider, 255);
      jwidget_disable(statusbar->slider);
    }
  }
  catch (locked_sprite_exception&) {
    // disable all
    jwidget_disable(statusbar->b_layer);
    jwidget_disable(statusbar->slider);
  }
}
