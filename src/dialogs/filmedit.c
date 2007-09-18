/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#include "jinete.h"
#include "jinete/intern.h"

#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "dialogs/dfrlen.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "raster/frame.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/frmove.h"
#include "util/thmbnail.h"

#endif

#define THUMBSIZE    (32)
#define FRMSIZE      (3 + THUMBSIZE + 2)
#define LAYSIZE      (3 + MAX (text_height (widget->text_font), THUMBSIZE) + 2)

enum {
  STATE_NONE,
  STATE_SCROLLING,
  STATE_MOVING,
  STATE_SELECTING,
};

struct FrameBox;

typedef struct LayerBox {
  JWidget widget;
  struct FrameBox *frame_box;
  /* for layer movement */
  Layer *layer;
  struct jrect rect;
  void *rect_data;
} LayerBox;

typedef struct FrameBox {
  JWidget widget;
  LayerBox *layer_box;
  /* for frame movement */
  Layer *layer;
  Frame *frame;
  struct jrect rect;
  void *rect_data;
} FrameBox;

static int state = STATE_NONE;

static JWidget layer_box_new (void);
static int layer_box_type (void);
static LayerBox *layer_box_data (JWidget widget);
static void layer_box_request_size (JWidget widget, int *w, int *h);
static bool layer_box_msg_proc (JWidget widget, JMessage msg);

static JWidget frame_box_new (void);
static int frame_box_type (void);
static FrameBox *frame_box_data (JWidget widget);
static void frame_box_request_size (JWidget widget, int *w, int *h);
static bool frame_box_msg_proc (JWidget widget, JMessage msg);

static Layer *select_prev_layer(Layer *layer, int enter_in_sets);
static Layer *select_next_layer(Layer *layer, int enter_in_sets);
static int count_layers(Layer *layer);
static int get_layer_pos(Layer *layer, Layer *current, int *pos);
static Layer *get_layer_in_pos(Layer *layer, int pos);

static void select_frpos_motion (JWidget widget);
static void select_layer_motion (JWidget widget, LayerBox *layer_box, FrameBox *frame_box);
static void control_scroll_motion (JWidget widget, LayerBox *layer_box, FrameBox *frame_box);
static void get_frame_rect (JWidget widget, JRect rect);

bool is_movingframe (void)
{
  return (state == STATE_MOVING)? TRUE: FALSE;
}

void switch_between_film_and_sprite_editor (void)
{
  Sprite *sprite = current_sprite;
  JWidget window, box1, panel1;
  JWidget layer_view, frame_view;
  JWidget layer_box, frame_box;

  if (!is_interactive ())
    return;

  window = jwindow_new_desktop ();
  box1 = jbox_new (JI_VERTICAL);
  panel1 = ji_panel_new (JI_HORIZONTAL);
  layer_view = jview_new ();
  frame_view = jview_new ();
  layer_box = layer_box_new ();
  frame_box = frame_box_new ();

  layer_box_data (layer_box)->frame_box = frame_box_data (frame_box);
  frame_box_data (frame_box)->layer_box = layer_box_data (layer_box);

  jwidget_expansive (panel1, TRUE);

  jview_attach (layer_view, layer_box);
  jview_attach (frame_view, frame_box);
  jview_without_bars (layer_view);
  jview_without_bars (frame_view);

  jwidget_add_child (panel1, layer_view);
  jwidget_add_child (panel1, frame_view);
  jwidget_add_child (box1, panel1);
  jwidget_add_child (window, box1);

  /* load window configuration */
  ji_panel_set_pos (panel1, get_config_float ("LayerWindow", "PanelPos", 50));
  jwindow_remap (window);

  /* center scrolls in selected layer/frpos */
  if (sprite->layer) {
    JWidget widget = layer_view;
    JRect vp1 = jview_get_viewport_position (layer_view);
    JRect vp2 = jview_get_viewport_position (frame_view);
    int y, pos;

    get_layer_pos (sprite->set, sprite->layer, &pos);
    y = LAYSIZE+LAYSIZE*pos+LAYSIZE/2-jrect_h(vp1)/2;

    jview_set_scroll (layer_view, 0, y);
    jview_set_scroll (frame_view,
			sprite->frpos*FRMSIZE+FRMSIZE/2-jrect_w(vp2)/2, y);

    jrect_free (vp1);
    jrect_free (vp2);
  }

  /* show the window */
  jwindow_open_fg (window);

  /* save window configuration */
  set_config_float ("LayerWindow", "PanelPos", ji_panel_get_pos (panel1));

  /* destroy the window */
  jwidget_free (window);

  /* destroy thumbnails */
  destroy_thumbnails();

  GUI_Refresh(sprite);
}

/***********************************************************************
			       LayerBox
 ***********************************************************************/

static JWidget layer_box_new (void)
{
  JWidget widget = jwidget_new (layer_box_type ());
  LayerBox *layer_box = jnew (LayerBox, 1);

  layer_box->widget = widget;

  jwidget_add_hook (widget, layer_box_type (),
		    layer_box_msg_proc, layer_box);
  jwidget_focusrest (widget, TRUE);

  return widget;
}

static int layer_box_type (void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

static LayerBox *layer_box_data (JWidget widget)
{
  return jwidget_get_data (widget, layer_box_type ());
}

static void layer_box_request_size (JWidget widget, int *w, int *h)
{
  Sprite *sprite = current_sprite;

  *w = 256;
  *h = LAYSIZE*(count_layers (sprite->set)+1);
}

static bool layer_box_msg_proc (JWidget widget, JMessage msg)
{
  LayerBox *layer_box = layer_box_data (widget);
  Sprite *sprite = current_sprite;

  switch (msg->type) {

    case JM_DESTROY:
      jfree (layer_box);
      break;

    case JM_REQSIZE:
      layer_box_request_size (widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_DRAW: {
      JWidget view = jwidget_get_view (widget);
      JRect vp = jview_get_viewport_position (view);
      BITMAP *bmp = create_bitmap (jrect_w(vp), jrect_h(vp));
      int scroll_x, scroll_y;
      bool selected_layer;
      int pos, layers;
      Layer *layer; 
      int y, h;

      jview_get_scroll (view, &scroll_x, &scroll_y);

      text_mode (-1);
      clear_to_color (bmp, makecol (255, 255, 255));

      h = LAYSIZE;
      y = h-scroll_y;

      hline (bmp, 0, h, bmp->w-1, makecol (0, 0, 0));
      set_clip (bmp, 0, h+1, bmp->w-1, bmp->h-1);

      layers = count_layers (sprite->set);
      for (pos=0; pos<layers; pos++) {
	layer = get_layer_in_pos (sprite->set, pos);
	
	selected_layer = 
	  (state != STATE_MOVING) ? (sprite->layer == layer):
				    (layer == layer_box->layer);

	if (selected_layer)
	  rectfill (bmp, 0, y, bmp->w-1, y+h-1, makecol (44, 76, 145));

	if (state == STATE_MOVING && sprite->layer == layer) {
	  rectdotted (bmp, 0, y+1, bmp->w-1, y+2,
		      makecol (0, 0, 0), makecol (255, 255, 255));
	}

	hline (bmp, 0, y, bmp->w-1, makecol (0, 0, 0));
	hline (bmp, 0, y+h, bmp->w-1, makecol (0, 0, 0));

#if 0
	{
	  int count, pos;
	  count = count_layers (sprite->set);
	  get_layer_pos (sprite->set, layer, &pos);
	  textprintf (bmp, widget->text_font,
		      2, y+h/2-text_height (widget->text_font)/2,
		      selected_layer ?
		      makecol (255, 255, 255): makecol (0, 0, 0),
		      "%d/%d", pos, count);
	}
#elif 0
	textout (bmp, widget->text_font, layer->name,
		 2, y+h/2-text_height (widget->text_font)/2,
		 selected_layer ?
		 makecol (255, 255, 255): makecol (0, 0, 0));
#else
	{
	  int tabs = -2;
	  int final_y = y+h/2;
	  Layer *l = layer;
	  while (l->gfxobj.type != GFXOBJ_SPRITE) {
	    if (++tabs > 0) {
/* 	      JList item = jlist_find (((Layer *)l->parent)->layers, l); */
	      int y1 = final_y-LAYSIZE/2;
	      int y2 = final_y+LAYSIZE/2;
/* 	      int y2 = item->prev ? final_y+LAYSIZE/2 : final_y; */
	      vline (bmp, tabs*16-1, y1, y2, makecol (0, 0, 0));
	    }
	    l = (Layer *)l->parent;
	  }
	  textout (bmp, widget->text_font, layer->name,
		   2+tabs*16, final_y-text_height (widget->text_font)/2,
		   selected_layer ?
		   makecol (255, 255, 255): makecol (0, 0, 0));
	}
#endif
	y += h;
      }

      blit(bmp, ji_screen, 0, 0, vp->x1, vp->y1, jrect_w(vp), jrect_h(vp));
      destroy_bitmap(bmp);
      jrect_free(vp);
      return TRUE;
    }

    case JM_CHAR:
      /* close film editor */
      if (check_for_accel (ACCEL_FOR_FILMEDITOR, msg) ||
	  msg->key.scancode == KEY_ESC) {
	jwidget_close_window (widget);
	return TRUE;
      }

      /* undo */
      if (check_for_accel (ACCEL_FOR_UNDO, msg)) {
	if (undo_can_undo (sprite->undo)) {
	  undo_undo (sprite->undo);
	  destroy_thumbnails ();
	  jmanager_refresh_screen ();
	}
	return TRUE;
      }

      /* redo */
      if (check_for_accel (ACCEL_FOR_REDO, msg)) {
	if (undo_can_redo (sprite->undo)) {
	  undo_redo (sprite->undo);
	  destroy_thumbnails ();
	  jmanager_refresh_screen ();
	}
	return TRUE;
      }

/* 	case KEY_UP: */
/* 	  select_next_layer (sprite->layer, TRUE); */
/* 	  GUI_Refresh (sprite); */
/* 	  break; */
/* 	case KEY_DOWN: */
/* 	  select_prev_layer (sprite->layer, TRUE); */
/* 	  GUI_Refresh (sprite); */
/* 	  break; */
      break;

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse (widget);

      if (msg->any.shifts & KB_SHIFT_FLAG) {
	state = STATE_SCROLLING;
	ji_mouse_set_cursor (JI_CURSOR_MOVE);
      }
      else {
	state = STATE_MOVING;
	ji_mouse_set_cursor (JI_CURSOR_MOVE);

	select_layer_motion (widget, layer_box, layer_box->frame_box);
	layer_box->layer = current_sprite->layer;
/* 	layer_box->rect = rect; */
/* 	layer_box->rect_data = NULL; */
      }

    case JM_MOTION:
      if (jwidget_has_capture (widget)) {
	/* scroll */
	if (state == STATE_SCROLLING)
	  control_scroll_motion (widget, layer_box, layer_box->frame_box);
	/* move */
	else if (state == STATE_MOVING) {
	  select_layer_motion (widget, layer_box, layer_box->frame_box);
	}
	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture (widget)) {
	jwidget_release_mouse (widget);

	if ((state == STATE_SCROLLING) || (state == STATE_MOVING))
	  ji_mouse_set_cursor (JI_CURSOR_NORMAL);

	if (state == STATE_MOVING) {
	  /* layer popup menu */
	  if (msg->mouse.right) {
	    JWidget popup_menuitem = get_layer_popup_menuitem ();

	    if (popup_menuitem && jmenuitem_get_submenu (popup_menuitem)) {
	      jmenu_popup (jmenuitem_get_submenu (popup_menuitem),
			   msg->mouse.x, msg->mouse.y);

	      jview_update (jwidget_get_view (layer_box->widget));
	      jview_update (jwidget_get_view (layer_box->frame_box->widget));
	    }
	  }
	  /* move */
	  else if (layer_box->layer != current_sprite->layer) {
	    layer_move_layer ((Layer *)layer_box->layer->parent,
			      layer_box->layer, current_sprite->layer);

	    current_sprite->layer = layer_box->layer;

	    jview_update (jwidget_get_view (layer_box->widget));
	    jview_update (jwidget_get_view (layer_box->frame_box->widget));
	  }
	  else
	    jwidget_dirty (widget);
	}

	state = STATE_NONE;
      }
      break;
  }

  return FALSE;
}

/***********************************************************************
			       FrameBox
 ***********************************************************************/

static JWidget frame_box_new (void)
{
  JWidget widget = jwidget_new (frame_box_type ());
  FrameBox *frame_box = jnew (FrameBox, 1);

  frame_box->widget = widget;

  jwidget_add_hook (widget, frame_box_type (),
		      frame_box_msg_proc, frame_box);
  jwidget_focusrest (widget, TRUE);

  return widget;
}

static int frame_box_type (void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

static FrameBox *frame_box_data (JWidget widget)
{
  return jwidget_get_data (widget, frame_box_type ());
}

static void frame_box_request_size (JWidget widget, int *w, int *h)
{
  Sprite *sprite = current_sprite;

  *w = FRMSIZE*sprite->frames-1;
  *h = LAYSIZE*(count_layers (sprite->set)+1);
}

static bool frame_box_msg_proc (JWidget widget, JMessage msg)
{
  FrameBox *frame_box = frame_box_data (widget);
  Sprite *sprite = current_sprite;

  switch (msg->type) {

    case JM_DESTROY:
      jfree (frame_box);
      break;

    case JM_REQSIZE:
      frame_box_request_size (widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_DRAW: {
      JWidget view = jwidget_get_view (widget);
      JRect vp = jview_get_viewport_position (view);
      BITMAP *bmp = create_bitmap (jrect_w(vp), jrect_h(vp));
      BITMAP *thumbnail;
      int scroll_x, scroll_y;
      int c, x, y, h;
      int x1, y1, x2, y2;
      Layer *layer; 
      int pos, layers;
      int offset_x;
      int first_viewable_frame;
      int last_viewable_frame;

      jview_get_scroll (view, &scroll_x, &scroll_y);

      text_mode (-1);

      /* all to white */
      clear_to_color (bmp, makecol (255, 255, 255));

      /* selected frame (draw the blue-column) */
      rectfill (bmp,
		-scroll_x+sprite->frpos*FRMSIZE, 0,
		-scroll_x+sprite->frpos*FRMSIZE+FRMSIZE-1, bmp->h-1,
		makecol (44, 76, 145));

      /* draw frames numbers (and vertical separators) */
      y = 0;
      h = LAYSIZE;
      offset_x = scroll_x % FRMSIZE;

      for (x=0; x<bmp->w+FRMSIZE; x += FRMSIZE) {
	vline (bmp, x-offset_x-1, 0, bmp->h, makecol (0, 0, 0));

	c = (x+scroll_x)/FRMSIZE;

	if (c >= sprite->frames) {
	  rectfill (bmp, x-offset_x, 0, x-offset_x+FRMSIZE-1, bmp->h-1,
		    makecol (128, 128, 128));
	}

	/* frpos */
	textprintf_centre
	  (bmp, widget->text_font,
	   x-offset_x+FRMSIZE/2, y+h/2-text_height (widget->text_font)/2,
	   c == sprite->frpos ? makecol (255, 255, 255): makecol (0, 0, 0),
	   "%d", c);

	/* frlen */
	textprintf_right
	  (bmp, widget->text_font,
	   x-offset_x+FRMSIZE-1, y+h-text_height (widget->text_font),
	   c == sprite->frpos ? makecol (255, 255, 255): makecol (0, 0, 0),
	   "%d", sprite_get_frlen(sprite, c));
      }

      /* draw layers keyframes */

      y = h-scroll_y;
      first_viewable_frame = (scroll_x)/FRMSIZE;
      last_viewable_frame = (bmp->w+FRMSIZE+scroll_x)/FRMSIZE;

      hline (bmp, 0, h, bmp->w-1, makecol (0, 0, 0));
      set_clip (bmp, 0, h+1, bmp->w-1, bmp->h-1);

      layers = count_layers (sprite->set);
      for (pos=0; pos<layers; pos++) {
	layer = get_layer_in_pos (sprite->set, pos);

	if (sprite->layer == layer)
	  rectfill (bmp, 0, y, bmp->w-1, y+h-1, makecol (44, 76, 145));
	else if (layer->gfxobj.type == GFXOBJ_LAYER_SET)
	  rectfill (bmp, 0, y, bmp->w-1, y+h-1, makecol (128, 128, 128));

	hline (bmp, 0, y, bmp->w-1, makecol (0, 0, 0));
	hline (bmp, 0, y+h, bmp->w-1, makecol (0, 0, 0));

	/* draw thumbnails */

	switch (layer->gfxobj.type) {
	  case GFXOBJ_LAYER_SET:
	    break;
	  case GFXOBJ_LAYER_IMAGE: {
	    Frame *frame, *frame_link;
	    int k_frpos, k_image;
	    JLink link;

	    JI_LIST_FOR_EACH(layer->frames, link) {
	      frame = link->data;
	      k_frpos = frame->frpos;

	      if (k_frpos < first_viewable_frame ||
		  k_frpos > last_viewable_frame)
		continue;

	      k_image = frame->image;

	      frame_link = frame_is_link(frame, layer);
	      thumbnail = generate_thumbnail(frame_link ? frame_link: frame,
					     layer);

	      x1 = k_frpos*FRMSIZE-scroll_x+FRMSIZE/2-THUMBSIZE/2;
	      y1 = y+3;
	      x2 = x1+THUMBSIZE-1;
	      y2 = y1+THUMBSIZE-1;

	      _ji_theme_rectedge(bmp, x1-1, y1-1, x2+1, y2+1,
				 makecol (128, 128, 128),
				 makecol (196, 196, 196));

	      if (thumbnail)
		draw_sprite(bmp, thumbnail, x1, y1);

	      if (frame_link) {
		BITMAP *sprite = get_gfx(GFX_LINKFRAME);
		draw_sprite(bmp, sprite, x1, y2-sprite->h+1);
	      }
	    }
	    break;
	  }
	  case GFXOBJ_LAYER_TEXT:
	    break;
	}

	y += h;
      }

      blit (bmp, ji_screen, 0, 0, vp->x1, vp->y1, jrect_w(vp), jrect_h(vp));
      destroy_bitmap (bmp);
      jrect_free (vp);
      return TRUE;
    }

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse (widget);

      if (msg->any.shifts & KB_SHIFT_FLAG) {
	state = STATE_SCROLLING;
	ji_mouse_set_cursor (JI_CURSOR_MOVE);
      }
      else {
	Frame *frame;

	state = STATE_SELECTING; /* by default we will be selecting
				    frames */

	select_frpos_motion (widget);
	select_layer_motion (widget, frame_box->layer_box, frame_box);

	/* show the dialog to change the frlen (frame duration time)? */
	if (msg->mouse.y < FRMSIZE) {
	  jwidget_release_mouse (widget);
	  GUI_FrameLength(current_sprite->frpos);
	  jwidget_dirty(widget);
	  return TRUE;
	}

	frame = layer_get_frame (current_sprite->layer,
				 current_sprite->frpos);
	if (frame) {
	  state = STATE_MOVING; /* now we will moving a frame */
	  ji_mouse_set_cursor (JI_CURSOR_MOVE);

	  frame_box->layer = current_sprite->layer;
	  frame_box->frame = frame;
	  frame_box->rect_data = NULL;

	  get_frame_rect (widget, &frame_box->rect);

	  scare_mouse ();
	  frame_box->rect_data = rectsave
	    (ji_screen,
	     frame_box->rect.x1, frame_box->rect.y1,
	     frame_box->rect.x2-1, frame_box->rect.y2-1);
	  rectdotted (ji_screen,
		      frame_box->rect.x1, frame_box->rect.y1,
		      frame_box->rect.x2-1, frame_box->rect.y2-1,
		      makecol (0, 0, 0), makecol (255, 255, 255));
	  unscare_mouse ();
	}
      }

    case JM_MOTION:
      if (jwidget_has_capture (widget)) {
	/* scroll */
	if (state == STATE_SCROLLING)
	  control_scroll_motion (widget, frame_box->layer_box, frame_box);
	/* move */
	else if (state == STATE_MOVING) {
	  scare_mouse ();

	  rectrestore (frame_box->rect_data);
	  rectdiscard (frame_box->rect_data);

	  select_frpos_motion (widget);
	  select_layer_motion (widget, frame_box->layer_box, frame_box);

	  get_frame_rect (widget, &frame_box->rect);

	  jwidget_flush_redraw (widget);

	  frame_box->rect_data = rectsave
	    (ji_screen,
	     frame_box->rect.x1, frame_box->rect.y1,
	     frame_box->rect.x2-1, frame_box->rect.y2-1);

	  rectdotted (ji_screen,
		      frame_box->rect.x1, frame_box->rect.y1,
		      frame_box->rect.x2-1, frame_box->rect.y2-1,
		      makecol (0, 0, 0), makecol (255, 255, 255));

	  unscare_mouse ();
	}
	/* select */
	else if (state == STATE_SELECTING) {
	  select_frpos_motion (widget);
	  select_layer_motion (widget, frame_box->layer_box, frame_box);
	}
	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture (widget)) {
	jwidget_release_mouse (widget);

	if ((state == STATE_SCROLLING) || (state == STATE_MOVING))
	  ji_mouse_set_cursor (JI_CURSOR_NORMAL);

	if ((state == STATE_SELECTING) || (state == STATE_MOVING)) {
	  if (state == STATE_MOVING) {
	    scare_mouse ();
	    rectrestore (frame_box->rect_data);
	    rectdiscard (frame_box->rect_data);
	    unscare_mouse ();
	  }

	  set_frame_to_handle (frame_box->layer, frame_box->frame);

	  if (msg->mouse.right) {
	    JWidget popup_menuitem = get_frame_popup_menuitem ();

	    if (popup_menuitem && jmenuitem_get_submenu (popup_menuitem)) {
	      /* show the frame pop-up menu */
	      jmenu_popup (jmenuitem_get_submenu (popup_menuitem),
			   msg->mouse.x, msg->mouse.y);

	      jview_update (jwidget_get_view (frame_box->widget));
	      jview_update (jwidget_get_view (frame_box->layer_box->widget));
	    }
	  }
	  else if (state == STATE_MOVING) {
	    move_frame ();
	    jwidget_dirty (widget);
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

static Layer *select_prev_layer (Layer *layer, int enter_in_sets)
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

static Layer *select_next_layer (Layer *layer, int enter_in_sets)
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

/* select the frpos depending of mouse motion (also adjust the scroll) */
static void select_frpos_motion(JWidget widget)
{
  Sprite *sprite = current_sprite;
  int frpos = (ji_mouse_x (0) - widget->rc->x1) / FRMSIZE;

  frpos = MID (0, frpos, sprite->frames-1);

  /* the frame change */
  if (sprite->frpos != frpos) {
    JWidget view = jwidget_get_view(widget);
    JRect vp = jview_get_viewport_position(view);
    int scroll_x, scroll_y, scroll_change;

    sprite->frpos = frpos;

    jview_get_scroll(view, &scroll_x, &scroll_y);

    if (widget->rc->x1+frpos*FRMSIZE < vp->x1) {
      jview_set_scroll(view, frpos*FRMSIZE, scroll_y);
      scroll_change = TRUE;
    }
    else if (widget->rc->x1+frpos*FRMSIZE+FRMSIZE-1 > vp->x2-1) {
      jview_set_scroll(view,
			 frpos*FRMSIZE+FRMSIZE-1-jrect_w(vp), scroll_y);
      scroll_change = TRUE;
    }
    else
      scroll_change = FALSE;

    if (scroll_change)
      ji_mouse_set_position(widget->rc->x1+frpos*FRMSIZE+FRMSIZE/2,
			    ji_mouse_y (0));

    jwidget_dirty(widget);
    jrect_free(vp);
  }
}

/* select the layer depending of mouse motion (also adjust the scroll) */
static void select_layer_motion(JWidget widget,
				LayerBox *layer_box, FrameBox *frame_box)
{
  Sprite *sprite = current_sprite;
  int layer, current_layer;

  /* get current layer pos */
  get_layer_pos(sprite->set, sprite->layer, &current_layer);

  /* get new selected pos */
  layer = (ji_mouse_y (0) - (widget->rc->y1 + LAYSIZE)) / LAYSIZE;
  layer = MID(0, layer, count_layers(sprite->set)-1);

  /* the layer change? */
  if (layer != current_layer) {
    JWidget view1 = jwidget_get_view(layer_box->widget);
    JWidget view2 = jwidget_get_view(frame_box->widget);
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
      ji_mouse_set_position(ji_mouse_x (0),
			    widget->rc->y1+LAYSIZE+layer*LAYSIZE+LAYSIZE/2);

    jwidget_dirty(layer_box->widget);
    jwidget_dirty(frame_box->widget);

    jrect_free(vp);
  }
}

/* controls scroll for "layer_box" and "frame_box" handling horizontal
   scroll individually and vertical scroll jointly */
static void control_scroll_motion (JWidget widget,
				   LayerBox *layer_box, FrameBox *frame_box)
{
  JWidget view1 = jwidget_get_view(layer_box->widget);
  JWidget view2 = jwidget_get_view(frame_box->widget);
  JRect vp = jview_get_viewport_position(jwidget_get_view (widget));
  int scroll1_x, scroll1_y;
  int scroll2_x, scroll2_y;

  jview_get_scroll(view1, &scroll1_x, &scroll1_y);
  jview_get_scroll(view2, &scroll2_x, &scroll2_y);

  /* horizontal scroll for layer_box */
  if (widget == layer_box->widget)
    scroll1_x += ji_mouse_x (1)-ji_mouse_x (0);
  /* horizontal scroll for frame_box */
  else
    scroll2_x += ji_mouse_x (1)-ji_mouse_x (0);

  jview_set_scroll(view1,
		     scroll1_x,
		     /* vertical scroll */
		     scroll1_y+ji_mouse_y (1)-ji_mouse_y (0));

  jview_set_scroll(view2,
		     scroll2_x,
		     /* vertical scroll */
		     scroll2_y+ji_mouse_y (1)-ji_mouse_y (0));

  ji_mouse_control_infinite_scroll(vp);
  jrect_free(vp);
}

static void get_frame_rect (JWidget widget, JRect rect)
{
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  int scroll_x, scroll_y;
  int y, h, x1, y1, pos;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  h = LAYSIZE;
  y = h-scroll_y;

  get_layer_pos(current_sprite->set, current_sprite->layer, &pos);

  x1 = current_sprite->frpos*FRMSIZE-scroll_x+FRMSIZE/2-THUMBSIZE/2;
  y1 = y+h*pos+3;

  rect->x1 = vp->x1 + x1 - 1;
  rect->y1 = vp->y1 + y1 - 1;
  rect->x2 = rect->x1 + THUMBSIZE + 2;
  rect->y2 = rect->y1 + THUMBSIZE + 2;

  jrect_free (vp);
}
