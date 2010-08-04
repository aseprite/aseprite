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
#include <stdlib.h>
#include <string.h>

#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"
#include "jinete/jtheme.h"

#include "core/color.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/paledit.h"

#define COLOR_SIZE	(m_boxsize)

static int paledit_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

PalEdit::PalEdit(bool editable)
  : Widget(paledit_type())
{
  m_editable = editable;
  m_range_type = PALETTE_EDITOR_RANGE_NONE;
  m_columns = 16;
  m_boxsize = 6;
  m_color[0] = -1;
  m_color[1] = -1;

  jwidget_focusrest(this, true);

  this->border_width.l = this->border_width.r = 1 * jguiscale();
  this->border_width.t = this->border_width.b = 1 * jguiscale();
  this->child_spacing = 1 * jguiscale();
}

int PalEdit::getRangeType()
{
  return m_range_type;
}

int PalEdit::getColumns()
{
  return m_columns;
}

void PalEdit::setColumns(int columns)
{
  int old_columns = m_columns;

  ASSERT(columns >= 1 && columns <= 256);
  m_columns = columns;

  if (m_columns != old_columns) {
    Widget* view = jwidget_get_view(this);
    if (view)
      jview_update(view);

    jwidget_dirty(this);
  }
}

void PalEdit::setBoxSize(int boxsize)
{
  m_boxsize = boxsize;
}

void PalEdit::selectColor(int index)
{
  ASSERT(index >= 0 && index <= 255);

  if ((m_color[0] != index) ||
      (m_color[1] != index) ||
      (m_range_type != PALETTE_EDITOR_RANGE_NONE)) {
    m_color[0] = index;
    m_color[1] = index;
    m_range_type = PALETTE_EDITOR_RANGE_NONE;

    if ((index >= 0) && (index <= 255))
      update_scroll(index);

    jwidget_dirty(this);
  }
}

void PalEdit::selectRange(int begin, int end, int range_type)
{
/*   ASSERT(begin >= 0 && begin <= 255); */
/*   ASSERT(end >= 0 && end <= 255); */

  m_color[0] = begin;
  m_color[1] = end;
  m_range_type = range_type;

  update_scroll(end);
  jwidget_dirty(this);
}

static void swap_color(Palette* palette, int i1, int i2)
{
  ase_uint32 c1 = palette->getEntry(i1);
  ase_uint32 c2 = palette->getEntry(i2);

  palette->setEntry(i2, c1);
  palette->setEntry(i1, c2);
}

void PalEdit::moveSelection(int x, int y)
{
  if (!m_editable)
    return;

  switch (m_range_type) {

    case PALETTE_EDITOR_RANGE_LINEAL: {
      int c1 = MIN(m_color[0], m_color[1]);
      int c2 = MAX(m_color[0], m_color[1]);
      int c;

      /* left */
      if (x < 0) {
	if (c1 > 0) {
	  for (c=c1; c<=c2; c++)
	    swap_color(get_current_palette(), c, c-1);

	  m_color[0]--;
	  m_color[1]--;
	}
      }
      /* right */
      else if (x > 0) {
	if (c2 < 255) {
	  for (c=c2; c>=c1; c--)
	    swap_color(get_current_palette(), c, c+1);

	  m_color[0]++;
	  m_color[1]++;
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
      int cols = m_columns;
      int index1 = m_color[0];
      int index2 = m_color[1];
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
	      swap_color(get_current_palette(),
			 (v*cols + u),
			 (v*cols + (u-1)));

	  m_color[0]--;
	  m_color[1]--;
	}
      }
      /* right */
      else if (x > 0) {
	if ((x2 < cols-1) && ((y2*cols+x2+1) <= 255)) {
	  for (v=y1; v<=y2; v++)
	    for (u=x2; u>=x1; u--)
	      swap_color(get_current_palette(),
			 (v*cols + u),
			 (v*cols + (u+1)));

	  m_color[0]++;
	  m_color[1]++;
	}
      }
      /* up */
      else if (y < 0) {
	if (((y1-1)*cols+x1) >= 0) {
	  for (v=y1; v<=y2; v++)
	    for (u=x1; u<=x2; u++)
	      swap_color(get_current_palette(),
			 (v*cols + u),
			 ((v-1)*cols + u));

	  m_color[0] -= cols;
	  m_color[1] -= cols;
	}
      }
      /* down */
      else if (y > 0) {
	if (((y2+1)*cols+x2) <= 255) {
	  for (v=y2; v>=y1; v--)
	    for (u=x1; u<=x2; u++)
	      swap_color(get_current_palette(),
			 (v*cols + u),
			 ((v+1)*cols + u));

	  m_color[0] += cols;
	  m_color[1] += cols;
	}
      }
      break;
    }
  }

  /* fixup the scroll */
  update_scroll(m_color[1]);

  /* set the palette */
  //set_current_palette(m_palette, false);

  /* refresh the screen */
  jmanager_refresh_screen();
}

int PalEdit::get1stColor()
{
  return m_color[0];
}

int PalEdit::get2ndColor()
{
  return m_color[1];
}

void PalEdit::getSelectedEntries(bool array[256])
{
  memset(array, false, sizeof(bool)*256);

  switch (m_range_type) {

    case PALETTE_EDITOR_RANGE_NONE:
      if (m_color[1] >= 0)
	array[m_color[1]] = true;
      break;

    case PALETTE_EDITOR_RANGE_LINEAL: {
      int c1 = MIN(m_color[0], m_color[1]);
      int c2 = MAX(m_color[0], m_color[1]);
      int c;

      for (c=c1; c<=c2; c++)
	array[c] = true;
      break;
    }

    case PALETTE_EDITOR_RANGE_RECTANGULAR: {
      int cols = m_columns;
      int index1 = m_color[0];
      int index2 = m_color[1];
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
	  array[y*cols + x] = true;
      break;
    }
  }
}

bool PalEdit::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      request_size(&msg->reqsize.w, &msg->reqsize.h);
      return true;

    // case JM_KEYPRESSED:
    //   if (jwidget_has_focus(this)) {
    // 	/* other keys */
    // 	if ((m_color[1] >= 0) && (m_color[1] <= 255)) {
    // 	  switch (msg->key.scancode) {
    // 	    case KEY_LEFT:  moveSelection(-1, 0); return true;
    // 	    case KEY_RIGHT: moveSelection(+1, 0); return true;
    // 	    case KEY_UP:    moveSelection(0, -1); return true;
    // 	    case KEY_DOWN:  moveSelection(0, +1); return true;
    // 	  }
    // 	}
    //   }
    //   break;

    case JM_DRAW: {
      div_t d = div(256, m_columns);
      int cols = m_columns;
      int rows = d.quot + ((d.rem)? 1: 0);
      int x1, y1, x2, y2;
      int x, y, u, v;
      int c, color;
      BITMAP *bmp;
      Palette* palette = get_current_palette();

      bmp = create_bitmap(jrect_w(this->rc), jrect_h(this->rc));
      clear_to_color(bmp, makecol(0 , 0, 0));

      y = this->border_width.t;
      c = 0;

      for (v=0; v<rows; v++) {
	x = this->border_width.l;

	for (u=0; u<cols; u++) {
	  if (c >= palette->size())
	    break;

	  if (bitmap_color_depth(ji_screen) == 8)
	    color = c;
	  else
	    color = makecol_depth
	      (bitmap_color_depth(ji_screen),
	       _rgba_getr(palette->getEntry(c)),
	       _rgba_getg(palette->getEntry(c)),
	       _rgba_getb(palette->getEntry(c)));

	  rectfill(bmp, x, y, x+COLOR_SIZE-1, y+COLOR_SIZE-1, color);

	  x += COLOR_SIZE+this->child_spacing;
	  c++;
	}

	y += COLOR_SIZE+this->child_spacing;
      }

      /* draw the edges in the selected color */
      if (m_color[0] >= 0) {
	int index1 = m_color[0];
	int index2 = m_color[1];
	int bl = this->border_width.l;
	int bt = this->border_width.t;
	int cs = this->child_spacing;
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
	if (m_range_type != PALETTE_EDITOR_RANGE_LINEAL) {
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
	   0, 0, this->rc->x1, this->rc->y1, bmp->w, bmp->h);
      destroy_bitmap(bmp);
      break;
    }

    case JM_BUTTONPRESSED:
      captureMouse();
      /* continue... */

    case JM_MOTION:
      if (hasCapture()) {
	JRect cpos = jwidget_get_child_rect(this);
	div_t d = div(256, m_columns);
	int cols = m_columns;
	int rows = d.quot + ((d.rem)? 1: 0);
	int mouse_x, mouse_y;
	int req_w, req_h;
	int x, y, u, v;
	int c;
	Palette* palette = get_current_palette();

	request_size(&req_w, &req_h);

	mouse_x = MID(cpos->x1, msg->mouse.x,
		      cpos->x1+req_w-this->border_width.r-1);
	mouse_y = MID(cpos->y1, msg->mouse.y,
		      cpos->y1+req_h-this->border_width.b-1);

	y = cpos->y1;
	c = 0;

	for (v=0; v<rows; v++) {
	  x = cpos->x1;

	  for (u=0; u<cols; u++) {
	    if (c >= palette->size())
	      break;

	    if ((mouse_x >= x) && (mouse_x <= x+COLOR_SIZE) &&
		(mouse_y >= y) && (mouse_y <= y+COLOR_SIZE) &&
		(c != m_color[1])) {
	      if (msg->any.shifts & KB_SHIFT_FLAG)
		selectRange(m_color[0], c, PALETTE_EDITOR_RANGE_LINEAL);
	      else if (msg->any.shifts & KB_CTRL_FLAG)
		selectRange(m_color[0], c, PALETTE_EDITOR_RANGE_RECTANGULAR);
	      else
		selectColor(c);

	      update_scroll(c);

	      jwidget_emit_signal(this, SIGNAL_PALETTE_EDITOR_CHANGE);
	      c = 256;
	      break;
	    }

	    x += COLOR_SIZE+this->child_spacing;
	    c++;
	  }

	  y += COLOR_SIZE+this->child_spacing;
	}

	jrect_free(cpos);
	return true;
      }
      break;

    case JM_BUTTONRELEASED:
      releaseMouse();
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void PalEdit::request_size(int* w, int* h)
{
  div_t d = div(256, m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem)? 1: 0);

  *w = this->border_width.l + this->border_width.r +
    + cols*COLOR_SIZE + (cols-1)*this->child_spacing;

  *h = this->border_width.t + this->border_width.b +
    + rows*COLOR_SIZE + (rows-1)*this->child_spacing;
}

void PalEdit::update_scroll(int color)
{
  Widget* view = jwidget_get_view(this);
  if (view != NULL) {
    JRect vp = jview_get_viewport_position(view);
    int scroll_x, scroll_y;
    int x, y, cols;
    div_t d;

    jview_get_scroll(view, &scroll_x, &scroll_y);

    d = div(256, m_columns);
    cols = m_columns;

    y = (COLOR_SIZE+this->child_spacing) * (color / cols);
    x = (COLOR_SIZE+this->child_spacing) * (color % cols);

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
