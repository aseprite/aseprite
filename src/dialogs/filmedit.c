/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "commands/commands.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/celmove.h"
#include "util/thmbnail.h"

#endif

#define THUMBSIZE	(32)
#define FRMSIZE		(3 + THUMBSIZE + 2)
#define LAYSIZE		(3 + MAX(text_height(widget->text_font), THUMBSIZE) + 2)

enum {
  STATE_NONE,
  STATE_SCROLLING,
  STATE_MOVING,
  STATE_SELECTING,
};

struct CelBox;

typedef struct LayerBox {
  JWidget widget;
  struct CelBox *cel_box;
  /* for layer movement */
  Layer *layer;
  struct jrect rect;
  void *rect_data;
} LayerBox;

typedef struct CelBox {
  JWidget widget;
  LayerBox *layer_box;
  /* for cel movement */
  Layer *layer;
  Cel *cel;
  struct jrect rect;
  void *rect_data;
} CelBox;

static int state = STATE_NONE;

static JWidget layer_box_new(void);
static int layer_box_type(void);
static LayerBox *layer_box_data(JWidget widget);
static void layer_box_request_size(JWidget widget, int *w, int *h);
static bool layer_box_msg_proc(JWidget widget, JMessage msg);

static JWidget cel_box_new(void);
static int cel_box_type(void);
static CelBox *cel_box_data(JWidget widget);
static void cel_box_request_size(JWidget widget, int *w, int *h);
static bool cel_box_msg_proc(JWidget widget, JMessage msg);

static Layer *select_prev_layer(Layer *layer, int enter_in_sets);
static Layer *select_next_layer(Layer *layer, int enter_in_sets);
static int count_layers(Layer *layer);
static int get_layer_pos(Layer *layer, Layer *current, int *pos);
static Layer *get_layer_in_pos(Layer *layer, int pos);

static void update_after_command(LayerBox *layer_box);
static void select_frame_motion(JWidget widget);
static void select_layer_motion(JWidget widget, LayerBox *layer_box, CelBox *cel_box);
static void control_scroll_motion(JWidget widget, LayerBox *layer_box, CelBox *cel_box);
static void get_cel_rect(JWidget widget, JRect rect);

bool is_movingcel(void)
{
  return (state == STATE_MOVING)? TRUE: FALSE;
}

void switch_between_film_and_sprite_editor(void)
{
  Sprite *sprite = current_sprite;
  JWidget window, box1, panel1;
  JWidget layer_view, frame_view;
  JWidget layer_box, cel_box;

  if (!is_interactive ())
    return;

  window = jwindow_new_desktop();
  box1 = jbox_new(JI_VERTICAL);
  panel1 = jpanel_new(JI_HORIZONTAL);
  layer_view = jview_new();
  frame_view = jview_new();
  layer_box = layer_box_new();
  cel_box = cel_box_new();

  layer_box_data(layer_box)->cel_box = cel_box_data(cel_box);
  cel_box_data(cel_box)->layer_box = layer_box_data(layer_box);

  jwidget_expansive(panel1, TRUE);

  jview_attach(layer_view, layer_box);
  jview_attach(frame_view, cel_box);
/*   jview_without_bars(layer_view); */
/*   jview_without_bars(frame_view); */

  jwidget_add_child(panel1, layer_view);
  jwidget_add_child(panel1, frame_view);
  jwidget_add_child(box1, panel1);
  jwidget_add_child(window, box1);

  /* load window configuration */
  jpanel_set_pos(panel1, get_config_float("LayerWindow", "PanelPos", 50));
  jwindow_remap(window);

  /* center scrolls in selected layer/frpos */
  if (sprite->layer) {
    JWidget widget = layer_view;
    JRect vp1 = jview_get_viewport_position(layer_view);
    JRect vp2 = jview_get_viewport_position(frame_view);
    int y, pos;

    get_layer_pos(sprite->set, sprite->layer, &pos);
    y = LAYSIZE+LAYSIZE*pos+LAYSIZE/2-jrect_h(vp1)/2;

    jview_set_scroll(layer_view, 0, y);
    jview_set_scroll(frame_view,
		     sprite->frame*FRMSIZE+FRMSIZE/2-jrect_w(vp2)/2, y);

    jrect_free(vp1);
    jrect_free(vp2);
  }

  /* show the window */
  jwindow_open_fg(window);

  /* save window configuration */
  set_config_float("LayerWindow", "PanelPos", jpanel_get_pos(panel1));

  /* destroy the window */
  jwidget_free(window);

  /* destroy thumbnails */
  destroy_thumbnails();

  update_screen_for_sprite(sprite);
}

/***********************************************************************
			       LayerBox
 ***********************************************************************/

static JWidget layer_box_new(void)
{
  JWidget widget = jwidget_new(layer_box_type());
  LayerBox *layer_box = jnew(LayerBox, 1);

  layer_box->widget = widget;

  jwidget_add_hook(widget, layer_box_type(),
		   layer_box_msg_proc, layer_box);
  jwidget_focusrest(widget, TRUE);

  return widget;
}

static int layer_box_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static LayerBox *layer_box_data(JWidget widget)
{
  return jwidget_get_data(widget, layer_box_type());
}

static void layer_box_request_size(JWidget widget, int *w, int *h)
{
  Sprite *sprite = current_sprite;

  *w = 256;
  *h = LAYSIZE*(count_layers(sprite->set)+1);

  *w = MAX(*w, JI_SCREEN_W);
  *h = MAX(*h, JI_SCREEN_H);
}

static bool layer_box_msg_proc(JWidget widget, JMessage msg)
{
  LayerBox *layer_box = layer_box_data(widget);
  Sprite *sprite = current_sprite;

  switch (msg->type) {

    case JM_DESTROY:
      jfree(layer_box);
      break;

    case JM_REQSIZE:
      layer_box_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SETPOS: {
      JWidget view1 = jwidget_get_view(layer_box->widget);
      JWidget view2 = jwidget_get_view(layer_box->cel_box->widget);
      int scroll1_x, scroll1_y;
      int scroll2_x, scroll2_y;

      jwidget_signal_off(widget);
      jview_get_scroll(view1, &scroll1_x, &scroll1_y);
      jview_get_scroll(view2, &scroll2_x, &scroll2_y);
      jview_set_scroll(view2, scroll2_x, scroll1_y);
      jwidget_signal_on(widget);
      break;
    }

    case JM_DRAW: {
      JWidget view = jwidget_get_view(widget);
      JRect vp = jview_get_viewport_position(view);
      BITMAP *bmp = create_bitmap(jrect_w(vp), jrect_h(vp));
      BITMAP *icon;
      int scroll_x, scroll_y;
      bool selected_layer;
      int pos, layers;
      Layer *layer; 
      int y, h, y_mid;

      jview_get_scroll(view, &scroll_x, &scroll_y);

      text_mode(-1);
      clear_to_color(bmp, makecol(255, 255, 255));

      h = LAYSIZE;
      y = h-scroll_y;

      hline(bmp, 0, h, bmp->w-1, makecol(0, 0, 0));
      set_clip(bmp, 0, h+1, bmp->w-1, bmp->h-1);

      layers = count_layers(sprite->set);
      for (pos=0; pos<layers; pos++) {
	layer = get_layer_in_pos(sprite->set, pos);
	y_mid = y+h/2;

	selected_layer = 
	  (state != STATE_MOVING) ? (sprite->layer == layer):
				    (layer == layer_box->layer);

	if (selected_layer)
	  rectfill(bmp, 0, y, bmp->w-1, y+h-1, makecol(44, 76, 145));

	if (state == STATE_MOVING && sprite->layer == layer) {
	  rectdotted(bmp, 0, y+1, bmp->w-1, y+2,
		     makecol(0, 0, 0), makecol(255, 255, 255));
	}

	hline(bmp, 0, y, bmp->w-1, makecol(0, 0, 0));
	hline(bmp, 0, y+h, bmp->w-1, makecol(0, 0, 0));

	/* draw the eye (readable flag) */
	icon = get_gfx(layer_is_readable(layer) ? GFX_BOX_SHOW:
						  GFX_BOX_HIDE);
	if (selected_layer)
	  jdraw_inverted_sprite(bmp, icon, 2, y_mid-4);
	else
	  draw_sprite(bmp, icon, 2, y_mid-4);

	/* draw the padlock (writable flag) */
	icon = get_gfx(layer_is_writable(layer) ? GFX_BOX_UNLOCK:
						  GFX_BOX_LOCK);
	if (selected_layer)
	  jdraw_inverted_sprite(bmp, icon, 2+8+2, y_mid-4);
	else
	  draw_sprite(bmp, icon, 2+8+2, y_mid-4);

#if 0
	{
	  int count, pos;
	  count = count_layers(sprite->set);
	  get_layer_pos(sprite->set, layer, &pos);
	  textprintf(bmp, widget->text_font,
		     2, y+h/2-text_height(widget->text_font)/2,
		     selected_layer ?
		     makecol(255, 255, 255): makecol(0, 0, 0),
		     "%d/%d", pos, count);
	}
#elif 0
	textout(bmp, widget->text_font, layer->name,
		2, y+h/2-text_height(widget->text_font)/2,
		selected_layer ?
		makecol(255, 255, 255): makecol(0, 0, 0));
#else
	{
	  int tabs = -2;
	  Layer *l = layer;

	  while (l->gfxobj.type != GFXOBJ_SPRITE) {
	    if (++tabs > 0) {
/* 	      JList item = jlist_find(((Layer *)l->parent)->layers, l); */
	      int y1 = y_mid-LAYSIZE/2;
	      int y2 = y_mid+LAYSIZE/2;
/* 	      int y2 = item->prev ? y_mid+LAYSIZE/2 : y_mid; */
	      vline(bmp, tabs*16-1, y1, y2, makecol(0, 0, 0));
	    }
	    l = (Layer *)l->parent;
	  }

	  /* draw the layer name */
	  textout(bmp, widget->text_font, layer->name,
		  2+8+2+8+2+8+tabs*16,
		  y_mid-text_height(widget->text_font)/2,
		  selected_layer ?
		  makecol(255, 255, 255): makecol(0, 0, 0));
	}
#endif
	y += h;
      }

      blit(bmp, ji_screen, 0, 0, vp->x1, vp->y1, jrect_w(vp), jrect_h(vp));
      destroy_bitmap(bmp);
      jrect_free(vp);
      return TRUE;
    }

    case JM_CHAR: {
      Command *command = command_get_by_key(msg);

      /* close film editor */
      if ((command && (strcmp(command->name, CMD_FILM_EDITOR) == 0)) ||
	  (msg->key.scancode == KEY_ESC)) {
	jwidget_close_window(widget);
	return TRUE;
      }

      /* undo */
      if (command && strcmp(command->name, CMD_UNDO) == 0) {
	if (undo_can_undo(sprite->undo)) {
	  undo_undo(sprite->undo);
	  update_after_command(layer_box);
	}
	return TRUE;
      }

      /* redo */
      if (command && strcmp(command->name, CMD_REDO) == 0) {
	if (undo_can_redo(sprite->undo)) {
	  undo_redo(sprite->undo);
	  update_after_command(layer_box);
	}
	return TRUE;
      }

      /* new_frame, remove_frame, new_cel, remove_cel */
      if (command &&
	  (strcmp(command->name, CMD_NEW_CEL) == 0 ||
	   strcmp(command->name, CMD_NEW_FRAME) == 0 ||
	   strcmp(command->name, CMD_NEW_LAYER) == 0 ||
	   strcmp(command->name, CMD_REMOVE_CEL) == 0 ||
	   strcmp(command->name, CMD_REMOVE_FRAME) == 0 ||
	   strcmp(command->name, CMD_REMOVE_LAYER) == 0)) {
	command_execute(command, NULL);
	update_after_command(layer_box);
	return TRUE;
      }

/* 	case KEY_UP: */
/* 	  select_next_layer(sprite->layer, TRUE); */
/* 	  update_screen_for_sprite(sprite); */
/* 	  break; */
/* 	case KEY_DOWN: */
/* 	  select_prev_layer(sprite->layer, TRUE); */
/* 	  update_screen_for_sprite(sprite); */
/* 	  break; */
      break;
    }

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse(widget);

      /* scroll */
      if (msg->any.shifts & KB_SHIFT_FLAG ||
	  msg->mouse.middle) {
	state = STATE_SCROLLING;
	jmouse_set_cursor(JI_CURSOR_MOVE);
      }
      else {
	select_layer_motion(widget, layer_box, layer_box->cel_box);
	layer_box->layer = current_sprite->layer;
/* 	layer_box->rect = rect; */
/* 	layer_box->rect_data = NULL; */

	/* toggle icon status (eye or the padlock) */
	if (msg->mouse.x < 2+8+2+8+2) {
	  if (msg->mouse.x <= 2+8+1) {
	    layer_box->layer->readable = !layer_box->layer->readable;
	  }
	  else {
	    layer_box->layer->writable = !layer_box->layer->writable;
	  }
	  jwidget_dirty(widget);
	}
	/* move */
	else {
	  state = STATE_MOVING;
	  jmouse_set_cursor(JI_CURSOR_MOVE);
	}
      }

    case JM_MOTION:
      if (jwidget_has_capture(widget)) {
	/* scroll */
	if (state == STATE_SCROLLING)
	  control_scroll_motion(widget, layer_box, layer_box->cel_box);
	/* move */
	else if (state == STATE_MOVING) {
	  select_layer_motion(widget, layer_box, layer_box->cel_box);
	}
	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget)) {
	jwidget_release_mouse(widget);

	if ((state == STATE_SCROLLING) || (state == STATE_MOVING))
	  jmouse_set_cursor(JI_CURSOR_NORMAL);

	if (state == STATE_MOVING) {
	  /* layer popup menu */
	  if (msg->mouse.right) {
	    JWidget popup_menu = get_layer_popup_menu();

	    if (popup_menu) {
	      jmenu_popup(popup_menu, msg->mouse.x, msg->mouse.y);

	      jview_update(jwidget_get_view(layer_box->widget));
	      jview_update(jwidget_get_view(layer_box->cel_box->widget));
	    }
	  }
	  /* move */
	  else if (layer_box->layer != current_sprite->layer) {
	    layer_move_layer((Layer *)layer_box->layer->parent,
			     layer_box->layer, current_sprite->layer);

	    current_sprite->layer = layer_box->layer;

	    jview_update(jwidget_get_view(layer_box->widget));
	    jview_update(jwidget_get_view(layer_box->cel_box->widget));
	  }
	  else
	    jwidget_dirty(widget);
	}

	state = STATE_NONE;
      }
      break;
  }

  return FALSE;
}

/***********************************************************************
			       CelBox
 ***********************************************************************/

static JWidget cel_box_new(void)
{
  JWidget widget = jwidget_new(cel_box_type());
  CelBox *cel_box = jnew(CelBox, 1);

  cel_box->widget = widget;

  jwidget_add_hook(widget, cel_box_type(), cel_box_msg_proc, cel_box);
  jwidget_focusrest(widget, TRUE);

  return widget;
}

static int cel_box_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static CelBox *cel_box_data(JWidget widget)
{
  return jwidget_get_data(widget, cel_box_type());
}

static void cel_box_request_size(JWidget widget, int *w, int *h)
{
  Sprite *sprite = current_sprite;

  *w = FRMSIZE*sprite->frames-1;
  *h = LAYSIZE*(count_layers(sprite->set)+1);

  *w = MAX(*w, JI_SCREEN_W);
  *h = MAX(*h, JI_SCREEN_H);
}

static bool cel_box_msg_proc(JWidget widget, JMessage msg)
{
  CelBox *cel_box = cel_box_data(widget);
  Sprite *sprite = current_sprite;

  switch (msg->type) {

    case JM_DESTROY:
      jfree(cel_box);
      break;

    case JM_REQSIZE:
      cel_box_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SETPOS: {
      JWidget view1 = jwidget_get_view(cel_box->layer_box->widget);
      JWidget view2 = jwidget_get_view(cel_box->widget);
      int scroll1_x, scroll1_y;
      int scroll2_x, scroll2_y;

      jwidget_signal_off(widget);
      jview_get_scroll(view1, &scroll1_x, &scroll1_y);
      jview_get_scroll(view2, &scroll2_x, &scroll2_y);
      jview_set_scroll(view1, scroll1_x, scroll2_y);
      jwidget_signal_on(widget);
      break;
    }

    case JM_DRAW: {
      JWidget view = jwidget_get_view(widget);
      JRect vp = jview_get_viewport_position(view);
      BITMAP *bmp = create_bitmap(jrect_w(vp), jrect_h(vp));
      BITMAP *thumbnail;
      int scroll_x, scroll_y;
      int c, x, y, h;
      int x1, y1, x2, y2;
      Layer *layer; 
      int pos, layers;
      int offset_x;
      int first_viewable_frame;
      int last_viewable_frame;

      jview_get_scroll(view, &scroll_x, &scroll_y);

      text_mode(-1);

      /* all to white */
      clear_to_color(bmp, makecol(255, 255, 255));

      /* selected frame (draw the blue-column) */
      rectfill(bmp,
	       -scroll_x+sprite->frame*FRMSIZE, 0,
	       -scroll_x+sprite->frame*FRMSIZE+FRMSIZE-1, bmp->h-1,
	       makecol(44, 76, 145));

      /* draw frames numbers (and vertical separators) */
      y = 0;
      h = LAYSIZE;
      offset_x = scroll_x % FRMSIZE;

      for (x=0; x<bmp->w+FRMSIZE; x += FRMSIZE) {
	vline(bmp, x-offset_x-1, 0, bmp->h, makecol(0, 0, 0));

	c = (x+scroll_x)/FRMSIZE;

	if (c >= sprite->frames) {
	  rectfill(bmp, x-offset_x, 0, x-offset_x+FRMSIZE-1, bmp->h-1,
		   makecol(128, 128, 128));
	}

	/* frame */
	textprintf_centre
	  (bmp, widget->text_font,
	   x-offset_x+FRMSIZE/2, y+h/2-text_height(widget->text_font)/2,
	   c == sprite->frame ? makecol(255, 255, 255): makecol(0, 0, 0),
	   "%d", c+1);

	/* frlen */
	textprintf_right
	  (bmp, widget->text_font,
	   x-offset_x+FRMSIZE-1, y+h-text_height(widget->text_font),
	   c == sprite->frame ? makecol(255, 255, 255): makecol(0, 0, 0),
	   "%dms", sprite_get_frlen(sprite, c));
      }

      /* draw layers keyframes */

      y = h-scroll_y;
      first_viewable_frame = (scroll_x)/FRMSIZE;
      last_viewable_frame = (bmp->w+FRMSIZE+scroll_x)/FRMSIZE;

      hline(bmp, 0, h, bmp->w-1, makecol(0, 0, 0));
      set_clip(bmp, 0, h+1, bmp->w-1, bmp->h-1);

      layers = count_layers(sprite->set);
      for (pos=0; pos<layers; pos++) {
	layer = get_layer_in_pos(sprite->set, pos);

	if (sprite->layer == layer)
	  rectfill(bmp, 0, y, bmp->w-1, y+h-1, makecol(44, 76, 145));
	else if (layer->gfxobj.type == GFXOBJ_LAYER_SET)
	  rectfill(bmp, 0, y, bmp->w-1, y+h-1, makecol(128, 128, 128));

	hline(bmp, 0, y, bmp->w-1, makecol(0, 0, 0));
	hline(bmp, 0, y+h, bmp->w-1, makecol(0, 0, 0));

	/* draw thumbnail of cels */

	switch (layer->gfxobj.type) {
	  case GFXOBJ_LAYER_SET:
	    break;
	  case GFXOBJ_LAYER_IMAGE: {
	    Cel *cel, *cel_link;
	    int k_frame, k_image;
	    JLink link;

	    JI_LIST_FOR_EACH(layer->cels, link) {
	      cel = link->data;
	      k_frame = cel->frame;

	      if (k_frame < first_viewable_frame ||
		  k_frame > last_viewable_frame)
		continue;

	      k_image = cel->image;

	      cel_link = cel_is_link(cel, layer);
	      thumbnail = generate_thumbnail(cel_link ? cel_link: cel, sprite);

	      x1 = k_frame*FRMSIZE-scroll_x+FRMSIZE/2-THUMBSIZE/2;
	      y1 = y+3;
	      x2 = x1+THUMBSIZE-1;
	      y2 = y1+THUMBSIZE-1;

	      _ji_theme_rectedge(bmp, x1-1, y1-1, x2+1, y2+1,
				 makecol(128, 128, 128),
				 makecol(196, 196, 196));

	      if (thumbnail)
		draw_sprite(bmp, thumbnail, x1, y1);

	      if (cel_link) {
		BITMAP *sprite = get_gfx(GFX_LINKFRAME);
		draw_sprite(bmp, sprite, x1, y2-sprite->h+1);
	      }
	    }
	    break;
	  }
	}

	y += h;
      }

      blit(bmp, ji_screen, 0, 0, vp->x1, vp->y1, jrect_w(vp), jrect_h(vp));
      destroy_bitmap(bmp);
      jrect_free(vp);
      return TRUE;
    }

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse(widget);

      if (msg->any.shifts & KB_SHIFT_FLAG) {
	state = STATE_SCROLLING;
	jmouse_set_cursor(JI_CURSOR_MOVE);
      }
      else {
	Cel *cel;

	state = STATE_SELECTING; /* by default we will be selecting
				    frames */

	select_frame_motion(widget);
	select_layer_motion(widget, cel_box->layer_box, cel_box);

	if (msg->mouse.y >= FRMSIZE) {
	  /* was the button pressed in a cel? */
	  cel = layer_get_cel(current_sprite->layer,
			      current_sprite->frame);
	  if (cel) {
	    state = STATE_MOVING; /* now we will moving a cel */
	    jmouse_set_cursor(JI_CURSOR_MOVE);

	    cel_box->layer = current_sprite->layer;
	    cel_box->cel = cel;
	    cel_box->rect_data = NULL;

	    get_cel_rect(widget, &cel_box->rect);

	    jmouse_hide();
	    cel_box->rect_data = rectsave(ji_screen,
					  cel_box->rect.x1, cel_box->rect.y1,
					  cel_box->rect.x2-1, cel_box->rect.y2-1);
	    rectdotted(ji_screen,
		       cel_box->rect.x1, cel_box->rect.y1,
		       cel_box->rect.x2-1, cel_box->rect.y2-1,
		       makecol(0, 0, 0), makecol(255, 255, 255));
	    jmouse_show();
	  }
	}
	/* show the dialog to change the frlen (frame duration time)? */
	else if (msg->mouse.right) {
	  jwidget_release_mouse(widget);
	  JWidget popup_menu = get_frame_popup_menu();

	  if (popup_menu) {
	    /* show the frame pop-up menu */
	    jmenu_popup(popup_menu, msg->mouse.x, msg->mouse.y);

	    jview_update(jwidget_get_view(cel_box->widget));
	    jview_update(jwidget_get_view(cel_box->layer_box->widget));
	  }

	  return TRUE;
	}
      }

    case JM_MOTION:
      if (jwidget_has_capture (widget)) {
	/* scroll */
	if (state == STATE_SCROLLING)
	  control_scroll_motion(widget, cel_box->layer_box, cel_box);
	/* move */
	else if (state == STATE_MOVING) {
	  jmouse_hide();

	  rectrestore(cel_box->rect_data);
	  rectdiscard(cel_box->rect_data);

	  select_frame_motion(widget);
	  select_layer_motion(widget, cel_box->layer_box, cel_box);

	  get_cel_rect(widget, &cel_box->rect);

	  jwidget_flush_redraw(widget);

	  cel_box->rect_data = rectsave(ji_screen,
					cel_box->rect.x1, cel_box->rect.y1,
					cel_box->rect.x2-1, cel_box->rect.y2-1);

	  rectdotted(ji_screen,
		     cel_box->rect.x1, cel_box->rect.y1,
		     cel_box->rect.x2-1, cel_box->rect.y2-1,
		     makecol(0, 0, 0), makecol(255, 255, 255));

	  jmouse_show();
	}
	/* select */
	else if (state == STATE_SELECTING) {
	  select_frame_motion(widget);
	  select_layer_motion(widget, cel_box->layer_box, cel_box);
	}
	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget)) {
	jwidget_release_mouse(widget);

	if ((state == STATE_SCROLLING) || (state == STATE_MOVING))
	  jmouse_set_cursor(JI_CURSOR_NORMAL);

	if ((state == STATE_SELECTING) || (state == STATE_MOVING)) {
	  if (state == STATE_MOVING) {
	    jmouse_hide();
	    rectrestore(cel_box->rect_data);
	    rectdiscard(cel_box->rect_data);
	    jmouse_show();
	  }

	  set_cel_to_handle(cel_box->layer, cel_box->cel);

	  if (msg->mouse.right) {
	    JWidget popup_menu = get_cel_popup_menu();

	    if (popup_menu) {
	      /* show the cel pop-up menu */
	      jmenu_popup(popup_menu, msg->mouse.x, msg->mouse.y);

	      destroy_thumbnails();

	      jview_update(jwidget_get_view(cel_box->widget));
	      jview_update(jwidget_get_view(cel_box->layer_box->widget));
	    }
	  }
	  else if (state == STATE_MOVING) {
	    move_cel();
	    destroy_thumbnails();
	    jwidget_dirty(widget);
	  }
	}

	state = STATE_NONE;
      }
      break;
  }

  return FALSE;
}

/***********************************************************************
			    Extra Routines
 ***********************************************************************/

static Layer *select_prev_layer(Layer *layer, int enter_in_sets)
{
  GfxObj *parent = layer->parent;

  if (enter_in_sets && layer->gfxobj.type == GFXOBJ_LAYER_SET) {
    if (!jlist_empty(layer->layers))
      layer = jlist_last_data(layer->layers);
  }
  else if (parent->type == GFXOBJ_LAYER_SET) {
    JList list = ((Layer *)parent)->layers;
    JLink link = jlist_find(list, layer);

    if (link != list->end) {
      if (link->prev != list->end)
	layer = link->prev->data;
      else
	layer = select_prev_layer((Layer *)parent, FALSE);
    }
  }

  return layer;
}

static Layer *select_next_layer(Layer *layer, int enter_in_sets)
{
  GfxObj *parent = layer->parent;

  if (enter_in_sets && layer->gfxobj.type == GFXOBJ_LAYER_SET) {
    if (!jlist_empty(layer->layers))
      layer = jlist_first_data(layer->layers);
  }
  else if (parent->type == GFXOBJ_LAYER_SET) {
    JList list = ((Layer *)parent)->layers;
    JLink link = jlist_find(list, layer);

    if (link != list->end) {
      if (link->next != list->end)
	layer = link->next->data;
      else
	layer = select_next_layer((Layer *)parent, FALSE);
    }
  }

  return layer;
}

static int count_layers(Layer *layer)
{
  int count;

  if (layer->parent->type == GFXOBJ_SPRITE)
    count = 0;
  else
    count = 1;

  if (layer->gfxobj.type == GFXOBJ_LAYER_SET) {
    JLink link;
    JI_LIST_FOR_EACH(layer->layers, link)
      count += count_layers(link->data);
  }

  return count;
}

static int get_layer_pos(Layer *layer, Layer *current, int *pos)
{
  if (layer->parent->type == GFXOBJ_SPRITE)
    *pos = 0;

  if (layer == current)
    return TRUE;

  if (layer->parent->type != GFXOBJ_SPRITE)
    (*pos)++;

  if (layer->gfxobj.type == GFXOBJ_LAYER_SET) {
    JLink link;
    JI_LIST_FOR_EACH_BACK(layer->layers, link)
      if (get_layer_pos(link->data, current, pos))
	return TRUE;
  }

  return FALSE;
}

static Layer *get_layer_in_pos(Layer *layer, int pos)
{
  static int internal_pos;

  if (layer->parent->type == GFXOBJ_SPRITE)
    internal_pos = 0;
  else {
    if (internal_pos == pos)
      return layer;

    internal_pos++;
  }

  if (layer->gfxobj.type == GFXOBJ_LAYER_SET) {
    Layer *child;
    JLink link;
    JI_LIST_FOR_EACH_BACK(layer->layers, link)
      if ((child = get_layer_in_pos(link->data, pos)))
	return child;
  }

  return NULL;
}

static void update_after_command(LayerBox *layer_box)
{
  jview_update(jwidget_get_view(layer_box->widget));
  jview_update(jwidget_get_view(layer_box->cel_box->widget));

  destroy_thumbnails();
  jmanager_refresh_screen();
}

/* select the frame depending of mouse motion (also adjust the scroll) */
static void select_frame_motion(JWidget widget)
{
  Sprite *sprite = current_sprite;
  int frame = (jmouse_x(0) - widget->rc->x1) / FRMSIZE;

  frame = MID(0, frame, sprite->frames-1);

  /* the frame change */
  if (sprite->frame != frame) {
    JWidget view = jwidget_get_view(widget);
    JRect vp = jview_get_viewport_position(view);
    int scroll_x, scroll_y, scroll_change;

    sprite->frame = frame;

    jview_get_scroll(view, &scroll_x, &scroll_y);

    if (widget->rc->x1+frame*FRMSIZE < vp->x1) {
      jview_set_scroll(view, frame*FRMSIZE, scroll_y);
      scroll_change = TRUE;
    }
    else if (widget->rc->x1+frame*FRMSIZE+FRMSIZE-1 > vp->x2-1) {
      jview_set_scroll(view,
		       frame*FRMSIZE+FRMSIZE-1-jrect_w(vp), scroll_y);
      scroll_change = TRUE;
    }
    else
      scroll_change = FALSE;

    if (scroll_change)
      jmouse_set_position(widget->rc->x1+frame*FRMSIZE+FRMSIZE/2, jmouse_y(0));

    jwidget_dirty(widget);
    jrect_free(vp);
  }
}

/* select the layer depending of mouse motion (also adjust the scroll) */
static void select_layer_motion(JWidget widget,
				LayerBox *layer_box, CelBox *cel_box)
{
  Sprite *sprite = current_sprite;
  int layer, current_layer;

  /* get current layer pos */
  get_layer_pos(sprite->set, sprite->layer, &current_layer);

  /* get new selected pos */
  layer = (jmouse_y(0) - (widget->rc->y1 + LAYSIZE)) / LAYSIZE;
  layer = MID(0, layer, count_layers(sprite->set)-1);

  /* the layer change? */
  if (layer != current_layer) {
    JWidget view1 = jwidget_get_view(layer_box->widget);
    JWidget view2 = jwidget_get_view(cel_box->widget);
    JRect vp = jview_get_viewport_position(jwidget_get_view(widget));
    int scroll1_x, scroll1_y, scroll_change;
    int scroll2_x, scroll2_y;

    sprite->layer = get_layer_in_pos(sprite->set, layer);

    jview_get_scroll(view1, &scroll1_x, &scroll1_y);
    jview_get_scroll(view2, &scroll2_x, &scroll2_y);

    if (widget->rc->y1+layer*LAYSIZE < vp->y1) {
      jview_set_scroll(view1, scroll1_x, layer*LAYSIZE);
      jview_set_scroll(view2, scroll2_x, layer*LAYSIZE);
      scroll_change = TRUE;
    }
    else if (widget->rc->y1+LAYSIZE+layer*LAYSIZE+LAYSIZE-1 > vp->y2-1) {
      jview_set_scroll(view1, scroll1_x, layer*LAYSIZE+LAYSIZE*2-1-jrect_h(vp));
      jview_set_scroll(view2, scroll2_x, layer*LAYSIZE+LAYSIZE*2-1-jrect_h(vp));
      scroll_change = TRUE;
    }
    else
      scroll_change = FALSE;

    if (scroll_change)
      jmouse_set_position(jmouse_x(0),
			  widget->rc->y1+LAYSIZE+layer*LAYSIZE+LAYSIZE/2);

    jwidget_dirty(layer_box->widget);
    jwidget_dirty(cel_box->widget);

    jrect_free(vp);
  }
}

/**
 * Controls scroll for "layer_box" and "cel_box" handling horizontal
 * scroll individually and vertical scroll jointly
 */
static void control_scroll_motion(JWidget widget,
				  LayerBox *layer_box, CelBox *cel_box)
{
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(jwidget_get_view(widget));
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);
  jview_set_scroll(view,
		   scroll_x+jmouse_x(1)-jmouse_x(0),
		   scroll_y+jmouse_y(1)-jmouse_y(0));
  
  jmouse_control_infinite_scroll(vp);
  jrect_free(vp);
}

static void get_cel_rect(JWidget widget, JRect rect)
{
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;
  int y, h, x1, y1, pos;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  h = LAYSIZE;
  y = h-scroll_y;

  get_layer_pos(current_sprite->set, current_sprite->layer, &pos);

  x1 = current_sprite->frame*FRMSIZE-scroll_x+FRMSIZE/2-THUMBSIZE/2;
  y1 = y+h*pos+3;

  rect->x1 = vp->x1 + x1 - 1;
  rect->y1 = vp->y1 + y1 - 1;
  rect->x2 = rect->x1 + THUMBSIZE + 2;
  rect->y2 = rect->y1 + THUMBSIZE + 2;

  jrect_free(vp);
}
