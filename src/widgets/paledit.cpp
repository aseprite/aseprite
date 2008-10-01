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

#include <allegro.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

#include "core/color.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/paledit.h"

/* #define COLOR_SIZE	6 */
#define COLOR_SIZE	(paledit->boxsize)

typedef struct PalEdit
{
  JWidget widget;
  Palette *palette;
  bool editable;
  unsigned range_type;
  unsigned columns;
  int boxsize;
  int color[2];
} PalEdit;

static PalEdit *paledit_data(JWidget widget);
static bool paledit_msg_proc(JWidget widget, JMessage msg);
static void paledit_request_size(JWidget widget, int *w, int *h);
static void paledit_update_scroll(JWidget widget, int color);

JWidget paledit_new(Palette *palette, bool editable, int boxsize)
{
  JWidget widget = jwidget_new(paledit_type());
  PalEdit *paledit = jnew(PalEdit, 1);

  paledit->widget = widget;
  paledit->palette = palette;
  paledit->editable = editable;
  paledit->range_type = PALETTE_EDITOR_RANGE_NONE;
  paledit->columns = 16;
  paledit->boxsize = boxsize;
  paledit->color[0] = -1;
  paledit->color[1] = -1;

  jwidget_add_hook(widget, paledit_type(), paledit_msg_proc, paledit);
  jwidget_focusrest(widget, TRUE);

  widget->border_width.l = widget->border_width.r = 1;
  widget->border_width.t = widget->border_width.b = 1;
  widget->child_spacing = 1;

  return widget;
}

int paledit_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

Palette *paledit_get_palette(JWidget widget)
{
  PalEdit *paledit = paledit_data(widget);

  return paledit->palette;
}

int paledit_get_range_type(JWidget widget)
{
  PalEdit *paledit = paledit_data(widget);

  return paledit->range_type;
}

int paledit_get_columns(JWidget widget)
{
  PalEdit *paledit = paledit_data(widget);

  return paledit->columns;
}

void paledit_set_columns(JWidget widget, int columns)
{
  PalEdit *paledit = paledit_data(widget);
  int old_columns = paledit->columns;

  assert(columns >= 1 && columns <= 256);
  paledit->columns = columns;

  if (paledit->columns != old_columns) {
    JWidget view = jwidget_get_view(widget);
    if (view)
      jview_update(view);

    jwidget_dirty(widget);
  }
}

void paledit_set_boxsize(JWidget widget, int boxsize)
{
  PalEdit *paledit = paledit_data(widget);

  paledit->boxsize = boxsize;
}

void paledit_select_color(JWidget widget, int index)
{
  PalEdit *paledit = paledit_data(widget);

  assert(index >= 0 && index <= 255);

  if ((paledit->color[0] != index) ||
      (paledit->color[1] != index) ||
      (paledit->range_type != PALETTE_EDITOR_RANGE_NONE)) {
    paledit->color[0] = index;
    paledit->color[1] = index;
    paledit->range_type = PALETTE_EDITOR_RANGE_NONE;

    if ((index >= 0) && (index <= 255))
      paledit_update_scroll(widget, index);

    jwidget_dirty(widget);
  }
}

void paledit_select_range(JWidget widget, int begin, int end, int range_type)
{
  PalEdit *paledit = paledit_data(widget);

/*   assert(begin >= 0 && begin <= 255); */
/*   assert(end >= 0 && end <= 255); */

  paledit->color[0] = begin;
  paledit->color[1] = end;
  paledit->range_type = range_type;

  paledit_update_scroll(widget, end);
  jwidget_dirty(widget);
}

static void swap_color(Palette *palette, int i1, int i2)
{
  ase_uint32 c1 = palette_get_entry(palette, i1);
  ase_uint32 c2 = palette_get_entry(palette, i2);

  palette_set_entry(palette, i2, c1);
  palette_set_entry(palette, i1, c2);
}

void paledit_move_selection(JWidget widget, int x, int y)
{
  PalEdit *paledit = paledit_data(widget);

  if (!paledit->editable)
    return;

  switch (paledit->range_type) {

    case PALETTE_EDITOR_RANGE_LINEAL: {
      int c1 = MIN(paledit->color[0], paledit->color[1]);
      int c2 = MAX(paledit->color[0], paledit->color[1]);
      int c;

      /* left */
      if (x < 0) {
	if (c1 > 0) {
	  for (c=c1; c<=c2; c++)
	    swap_color(paledit->palette, c, c-1);

	  paledit->color[0]--;
	  paledit->color[1]--;
	}
      }
      /* right */
      else if (x > 0) {
	if (c2 < 255) {
	  for (c=c2; c>=c1; c--)
	    swap_color(paledit->palette, c, c+1);

	  paledit->color[0]++;
	  paledit->color[1]++;
	}
      }
      /* up */
      else if (y < 0) {
	/* TODO this should be implemented? */
      }
      /* down */
      else if (y > 0) {
	/* TODO this should be implemented? */
      }
      break;
    }

    case PALETTE_EDITOR_RANGE_NONE:
    case PALETTE_EDITOR_RANGE_RECTANGULAR: {
      int cols = paledit->columns;
      int index1 = paledit->color[0];
      int index2 = paledit->color[1];
      int c, u, v, x1, y1, x2, y2;

      /* swap */
      if (index1 > index2) {
	c = index1;
	index1 = index2;
	index2 = c;
      }

      x1 = index1 % cols;
      y1 = index1 / cols;
      x2 = index2 % cols;
      y2 = index2 / cols;

      if (x2 < x1) { c = x2; x2 = x1; x1 = c; }
      if (y2 < y1) { c = y2; y2 = y1; y1 = c; }

      /* left */
      if (x < 0) {
	if ((x1 > 0) && ((y1*cols+x1-1) >= 0)) {
	  for (v=y1; v<=y2; v++)
	    for (u=x1; u<=x2; u++)
	      swap_color(paledit->palette,
			 (v*cols + u),
			 (v*cols + (u-1)));

	  paledit->color[0]--;
	  paledit->color[1]--;
	}
      }
      /* right */
      else if (x > 0) {
	if ((x2 < cols-1) && ((y2*cols+x2+1) <= 255)) {
	  for (v=y1; v<=y2; v++)
	    for (u=x2; u>=x1; u--)
	      swap_color(paledit->palette,
			 (v*cols + u),
			 (v*cols + (u+1)));

	  paledit->color[0]++;
	  paledit->color[1]++;
	}
      }
      /* up */
      else if (y < 0) {
	if (((y1-1)*cols+x1) >= 0) {
	  for (v=y1; v<=y2; v++)
	    for (u=x1; u<=x2; u++)
	      swap_color(paledit->palette,
			 (v*cols + u),
			 ((v-1)*cols + u));

	  paledit->color[0] -= cols;
	  paledit->color[1] -= cols;
	}
      }
      /* down */
      else if (y > 0) {
	if (((y2+1)*cols+x2) <= 255) {
	  for (v=y2; v>=y1; v--)
	    for (u=x1; u<=x2; u++)
	      swap_color(paledit->palette,
			 (v*cols + u),
			 ((v+1)*cols + u));

	  paledit->color[0] += cols;
	  paledit->color[1] += cols;
	}
      }
      break;
    }
  }

  /* fixup the scroll */
  paledit_update_scroll(widget, paledit->color[1]);

  /* set the palette */
  set_current_palette(paledit->palette, FALSE);

  /* refresh the screen */
  jmanager_refresh_screen();
}

int paledit_get_1st_color(JWidget widget)
{
  PalEdit *paledit = paledit_data(widget);
  return paledit->color[0];
}

int paledit_get_2nd_color(JWidget widget)
{
  PalEdit *paledit = paledit_data(widget);
  return paledit->color[1];
}

void paledit_get_selected_entries(JWidget widget, bool array[256])
{
  PalEdit *paledit = paledit_data(widget);

  memset(array, FALSE, sizeof(bool)*256);

  switch (paledit->range_type) {

    case PALETTE_EDITOR_RANGE_NONE:
      if (paledit->color[1] >= 0)
	array[paledit->color[1]] = TRUE;
      break;

    case PALETTE_EDITOR_RANGE_LINEAL: {
      int c1 = MIN(paledit->color[0], paledit->color[1]);
      int c2 = MAX(paledit->color[0], paledit->color[1]);
      int c;

      for (c=c1; c<=c2; c++)
	array[c] = TRUE;
      break;
    }

    case PALETTE_EDITOR_RANGE_RECTANGULAR: {
      int cols = paledit->columns;
      int index1 = paledit->color[0];
      int index2 = paledit->color[1];
      int c, x, y, x1, y1, x2, y2;

      /* swap */
      if (index1 > index2) {
	c = index1;
	index1 = index2;
	index2 = c;
      }

      x1 = index1 % cols;
      y1 = index1 / cols;
      x2 = index2 % cols;
      y2 = index2 / cols;

      if (x2 < x1) { c = x2; x2 = x1; x1 = c; }
      if (y2 < y1) { c = y2; y2 = y1; y1 = c; }

      for (y=y1; y<=y2; y++)
	for (x=x1; x<=x2; x++)
	  array[y*cols + x] = TRUE;
      break;
    }
  }
}

static PalEdit *paledit_data(JWidget widget)
{
  return reinterpret_cast<PalEdit*>(jwidget_get_data(widget, paledit_type()));
}

static bool paledit_msg_proc(JWidget widget, JMessage msg)
{
  PalEdit *paledit = paledit_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree (paledit);
      break;

    case JM_REQSIZE:
      paledit_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_KEYPRESSED:
      if (jwidget_has_focus(widget)) {
	/* other keys */
	if ((paledit->color[1] >= 0) &&
	    (paledit->color[1] <= 255)) {
	  switch (msg->key.scancode) {
	    case KEY_LEFT:  paledit_move_selection(widget, -1, 0); break;
	    case KEY_RIGHT: paledit_move_selection(widget, +1, 0); break;
	    case KEY_UP:    paledit_move_selection(widget, 0, -1); break;
	    case KEY_DOWN:  paledit_move_selection(widget, 0, +1); break;

	    default:
	      return FALSE;
	  }
	  return TRUE;
	}
      }
      break;

    case JM_DRAW: {
      div_t d = div(256, paledit->columns);
      int cols = paledit->columns;
      int rows = d.quot + ((d.rem)? 1: 0);
      int x1, y1, x2, y2;
      int x, y, u, v;
      int c, color;
      BITMAP *bmp;

      bmp = create_bitmap (jrect_w(widget->rc), jrect_h(widget->rc));
      clear_to_color (bmp, makecol(0 , 0, 0));

      y = widget->border_width.t;
      c = 0;

      for (v=0; v<rows; v++) {
	x = widget->border_width.l;

	for (u=0; u<cols; u++) {
	  if (c >= 256)
	    break;

	  if (bitmap_color_depth(ji_screen) == 8)
	    color = _index_cmap[c];
	  else
	    color = makecol_depth
	      (bitmap_color_depth(ji_screen),
	       _rgba_getr(paledit->palette->color[c]),
	       _rgba_getg(paledit->palette->color[c]),
	       _rgba_getb(paledit->palette->color[c]));

	  rectfill(bmp, x, y, x+COLOR_SIZE-1, y+COLOR_SIZE-1, color);

	  x += COLOR_SIZE+widget->child_spacing;
	  c++;
	}

	y += COLOR_SIZE+widget->child_spacing;
      }

      /* draw the edges in the selected color */
      if (paledit->color[0] >= 0) {
	int index1 = paledit->color[0];
	int index2 = paledit->color[1];
	int bl = widget->border_width.l;
	int bt = widget->border_width.t;
	int cs = widget->child_spacing;
	int color = makecol (255, 255, 255);

	/* swap */
	if (index1 > index2) {
	  c = index1;
	  index1 = index2;
	  index2 = c;
	}

	/* selection position */
	x1 = index1 % cols;
	y1 = index1 / cols;
	x2 = index2 % cols;
	y2 = index2 / cols;

	if (y2 < y1) { c = y2; y2 = y1; y1 = c; }

	/* draw the rectangular gamma or just the cursor */
	if (paledit->range_type != PALETTE_EDITOR_RANGE_LINEAL) {
	  if (x2 < x1) { c = x2; x2 = x1; x1 = c; }

	  rect(bmp,
	       bl + x1*(cs+COLOR_SIZE)-1,
	       bt + y1*(cs+COLOR_SIZE)-1,
	       bl + x2*(cs+COLOR_SIZE)+COLOR_SIZE,
	       bt + y2*(cs+COLOR_SIZE)+COLOR_SIZE, color);
	}
	/* draw the linear gamma */
	else {
	  c = 0;
	  for (y=0; y<rows; y++) {
	    for (x=0; x<cols; x++) {
	      if ((c == index1) || ((x == 0) && (y > y1) && (y <= y2))) {
		vline(bmp,
		      bl + x*(cs+COLOR_SIZE)-1,
		      bt + y*(cs+COLOR_SIZE)-1,
		      bt + y*(cs+COLOR_SIZE)+COLOR_SIZE, color);
	      }

	      if ((c == index2) || ((x == cols-1) && (y >= y1) && (y < y2))) {
		vline(bmp,
		      bl + x*(cs+COLOR_SIZE)+COLOR_SIZE,
		      bt + y*(cs+COLOR_SIZE)-1,
		      bt + y*(cs+COLOR_SIZE)+COLOR_SIZE, color);
	      }

	      if (y == y1) {
		if (x >= x1) {
		  if ((y < y2) || (x <= x2))
		    hline(bmp,
			  bl + x*(cs+COLOR_SIZE)-1,
			  bt + y*(cs+COLOR_SIZE)-1,
			  bl + x*(cs+COLOR_SIZE)+COLOR_SIZE, color);
		}
		else if (y < y2) {
		  if ((y+1 < y2) || (x <= x2))
		    hline(bmp,
			  bl + x*(cs+COLOR_SIZE)-1,
			  bt + y*(cs+COLOR_SIZE)+COLOR_SIZE,
			  bl + x*(cs+COLOR_SIZE)+COLOR_SIZE, color);
		}
	      }

	      if (y == y2) {
		if (x <= x2) {
		  if ((y > y1) || (x >= x1))
		    hline(bmp,
			  bl + x*(cs+COLOR_SIZE)-1,
			  bt + y*(cs+COLOR_SIZE)+COLOR_SIZE,
			  bl + x*(cs+COLOR_SIZE)+COLOR_SIZE, color);
		}
		else if (y > y1) {
		  if ((y-1 > y1) || (x >= x1))	
		    hline(bmp,
			  bl + x*(cs+COLOR_SIZE)-1,
			  bt + y*(cs+COLOR_SIZE)-1,
			  bl + x*(cs+COLOR_SIZE)+COLOR_SIZE, color);
		}
	      }

	      c++;
	    }
	  }
	}
      }

      blit(bmp, ji_screen,
	   0, 0, widget->rc->x1, widget->rc->y1, bmp->w, bmp->h);
      destroy_bitmap(bmp);
      break;
    }

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse(widget);
      /* continue... */

    case JM_MOTION:
      if (jwidget_has_capture(widget)) {
	JRect cpos = jwidget_get_child_rect(widget);
	div_t d = div (256, paledit->columns);
	int cols = paledit->columns;
	int rows = d.quot + ((d.rem)? 1: 0);
	int mouse_x, mouse_y;
	int req_w, req_h;
	int x, y, u, v;
	int c;

	paledit_request_size(widget, &req_w, &req_h);

	mouse_x = MID(cpos->x1, msg->mouse.x,
		      cpos->x1+req_w-widget->border_width.r-1);
	mouse_y = MID(cpos->y1, msg->mouse.y,
		      cpos->y1+req_h-widget->border_width.b-1);

	y = cpos->y1;
	c = 0;

	for (v=0; v<rows; v++) {
	  x = cpos->x1;

	  for (u=0; u<cols; u++) {
	    if (c >= 256)
	      break;

	    if ((mouse_x >= x) && (mouse_x <= x+COLOR_SIZE) &&
		(mouse_y >= y) && (mouse_y <= y+COLOR_SIZE) &&
		(c != paledit->color[1])) {
	      if (msg->any.shifts & KB_SHIFT_FLAG)
		paledit_select_range(widget,
				     paledit->color[0], c,
				     PALETTE_EDITOR_RANGE_LINEAL);
	      else if (msg->any.shifts & KB_CTRL_FLAG)
		paledit_select_range(widget,
				     paledit->color[0], c,
				     PALETTE_EDITOR_RANGE_RECTANGULAR);
	      else
		paledit_select_color(widget, c);

	      paledit_update_scroll(widget, c);

	      jwidget_emit_signal(widget, SIGNAL_PALETTE_EDITOR_CHANGE);
	      c = 256;
	      break;
	    }

	    x += COLOR_SIZE+widget->child_spacing;
	    c++;
	  }

	  y += COLOR_SIZE+widget->child_spacing;
	}

	jrect_free(cpos);
	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      jwidget_release_mouse(widget);
      return TRUE;
  }

  return FALSE;
}

static void paledit_request_size(JWidget widget, int *w, int *h)
{
  PalEdit *paledit = paledit_data(widget);
  div_t d = div(256, paledit->columns);
  int cols = paledit->columns;
  int rows = d.quot + ((d.rem)? 1: 0);

  *w = widget->border_width.l + widget->border_width.r +
    + cols*COLOR_SIZE + (cols-1)*widget->child_spacing;

  *h = widget->border_width.t + widget->border_width.b +
    + rows*COLOR_SIZE + (rows-1)*widget->child_spacing;
}

static void paledit_update_scroll(JWidget widget, int color)
{
  JWidget view = jwidget_get_view(widget);
  if (view != NULL) {
    PalEdit *paledit = paledit_data(widget);
    JRect vp = jview_get_viewport_position(view);
    int scroll_x, scroll_y;
    int x, y, cols;
    div_t d;

    jview_get_scroll(view, &scroll_x, &scroll_y);

    d = div(256, paledit->columns);
    cols = paledit->columns;

    y = (COLOR_SIZE+widget->child_spacing) * (color / cols);
    x = (COLOR_SIZE+widget->child_spacing) * (color % cols);

    if (scroll_x > x)
      scroll_x = x;
    else if (scroll_x+jrect_w(vp)-COLOR_SIZE-2 < x)
      scroll_x = x-jrect_w(vp)+COLOR_SIZE+2;

    if (scroll_y > y)
      scroll_y = y;
    else if (scroll_y+jrect_h(vp)-COLOR_SIZE-2 < y)
      scroll_y = y-jrect_h(vp)+COLOR_SIZE+2;

    jview_set_scroll(view, scroll_x, scroll_y);

    jrect_free(vp);
  }
}
