/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "app.h"
#include "app/color.h"
#include "gfx/point.h"
#include "gui/manager.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/system.h"
#include "gui/theme.h"
#include "gui/view.h"
#include "gui/widget.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "widgets/palette_view.h"
#include "widgets/statebar.h"

int palette_view_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

PaletteView::PaletteView(bool editable)
  : Widget(palette_view_type())
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

int PaletteView::getRangeType()
{
  return m_range_type;
}

int PaletteView::getColumns()
{
  return m_columns;
}

void PaletteView::setColumns(int columns)
{
  int old_columns = m_columns;

  ASSERT(columns >= 1 && columns <= 256);
  m_columns = columns;

  if (m_columns != old_columns) {
    View* view = View::getView(this);
    if (view)
      view->updateView();

    invalidate();
  }
}

void PaletteView::setBoxSize(int boxsize)
{
  m_boxsize = boxsize;
}

void PaletteView::selectColor(int index)
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

    invalidate();
  }
}

void PaletteView::selectRange(int begin, int end, int range_type)
{
/*   ASSERT(begin >= 0 && begin <= 255); */
/*   ASSERT(end >= 0 && end <= 255); */

  m_color[0] = begin;
  m_color[1] = end;
  m_range_type = range_type;

  update_scroll(end);
  invalidate();
}

static void swap_color(Palette* palette, int i1, int i2)
{
  uint32_t c1 = palette->getEntry(i1);
  uint32_t c2 = palette->getEntry(i2);

  palette->setEntry(i2, c1);
  palette->setEntry(i1, c2);
}

void PaletteView::moveSelection(int x, int y)
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

int PaletteView::get1stColor()
{
  return m_color[0];
}

int PaletteView::get2ndColor()
{
  return m_color[1];
}

void PaletteView::getSelectedEntries(bool array[256])
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

Color PaletteView::getColorByPosition(int target_x, int target_y)
{
  Palette* palette = get_current_palette();
  JRect cpos = jwidget_get_child_rect(this);
  div_t d = div(256, m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem)? 1: 0);
  int req_w, req_h;
  int x, y, u, v;
  int c;

  request_size(&req_w, &req_h);

  y = cpos->y1;
  c = 0;

  for (v=0; v<rows; v++) {
    x = cpos->x1;

    for (u=0; u<cols; u++) {
      if (c >= palette->size())
	break;

      if ((target_x >= x) && (target_x <= x+m_boxsize) &&
	  (target_y >= y) && (target_y <= y+m_boxsize))
	return Color::fromIndex(c);

      x += m_boxsize+this->child_spacing;
      c++;
    }

    y += m_boxsize+this->child_spacing;
  }

  jrect_free(cpos);
  return Color::fromMask();
}

bool PaletteView::onProcessMessage(JMessage msg)
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

	  rectfill(bmp, x, y, x+m_boxsize-1, y+m_boxsize-1, color);

	  x += m_boxsize+this->child_spacing;
	  c++;
	}

	y += m_boxsize+this->child_spacing;
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
	       bl + x1*(cs+m_boxsize)-1,
	       bt + y1*(cs+m_boxsize)-1,
	       bl + x2*(cs+m_boxsize)+m_boxsize,
	       bt + y2*(cs+m_boxsize)+m_boxsize, color);
	}
	/* draw the linear gamma */
	else {
	  c = 0;
	  for (y=0; y<rows; y++) {
	    for (x=0; x<cols; x++) {
	      if ((c == index1) || ((x == 0) && (y > y1) && (y <= y2))) {
		vline(bmp,
		      bl + x*(cs+m_boxsize)-1,
		      bt + y*(cs+m_boxsize)-1,
		      bt + y*(cs+m_boxsize)+m_boxsize, color);
	      }

	      if ((c == index2) || ((x == cols-1) && (y >= y1) && (y < y2))) {
		vline(bmp,
		      bl + x*(cs+m_boxsize)+m_boxsize,
		      bt + y*(cs+m_boxsize)-1,
		      bt + y*(cs+m_boxsize)+m_boxsize, color);
	      }

	      if (y == y1) {
		if (x >= x1) {
		  if ((y < y2) || (x <= x2))
		    hline(bmp,
			  bl + x*(cs+m_boxsize)-1,
			  bt + y*(cs+m_boxsize)-1,
			  bl + x*(cs+m_boxsize)+m_boxsize, color);
		}
		else if (y < y2) {
		  if ((y+1 < y2) || (x <= x2))
		    hline(bmp,
			  bl + x*(cs+m_boxsize)-1,
			  bt + y*(cs+m_boxsize)+m_boxsize,
			  bl + x*(cs+m_boxsize)+m_boxsize, color);
		}
	      }

	      if (y == y2) {
		if (x <= x2) {
		  if ((y > y1) || (x >= x1))
		    hline(bmp,
			  bl + x*(cs+m_boxsize)-1,
			  bt + y*(cs+m_boxsize)+m_boxsize,
			  bl + x*(cs+m_boxsize)+m_boxsize, color);
		}
		else if (y > y1) {
		  if ((y-1 > y1) || (x >= x1))	
		    hline(bmp,
			  bl + x*(cs+m_boxsize)-1,
			  bt + y*(cs+m_boxsize)-1,
			  bl + x*(cs+m_boxsize)+m_boxsize, color);
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
      return true;
    }

    case JM_BUTTONPRESSED:
      captureMouse();
      /* continue... */

    case JM_MOTION: {
      JRect cpos = jwidget_get_child_rect(this);

      int req_w, req_h;
      request_size(&req_w, &req_h);

      int mouse_x = MID(cpos->x1, msg->mouse.x, cpos->x1+req_w-this->border_width.r-1);
      int mouse_y = MID(cpos->y1, msg->mouse.y, cpos->y1+req_h-this->border_width.b-1);

      jrect_free(cpos);

      Color color = getColorByPosition(mouse_x, mouse_y);
      if (color.getType() == Color::IndexType) {
	app_get_statusbar()->showColor(0, "", color, 255);

	if (hasCapture() && color.getIndex() != m_color[1]) {
	  int idx = color.getIndex();

	  if (msg->any.shifts & KB_SHIFT_FLAG)
	    selectRange(m_color[0], idx, PALETTE_EDITOR_RANGE_LINEAL);
	  else if (msg->any.shifts & KB_CTRL_FLAG)
	    selectRange(m_color[0], idx, PALETTE_EDITOR_RANGE_RECTANGULAR);
	  else
	    selectColor(idx);

	  update_scroll(idx);

	  // Emit signals
	  jwidget_emit_signal(this, SIGNAL_PALETTE_EDITOR_CHANGE);
	  IndexChange(idx);
	}
      }

      if (hasCapture())
	return true;

      break;
    }

    case JM_BUTTONRELEASED:
      releaseMouse();
      return true;

    case JM_WHEEL: {
      View* view = View::getView(this);
      if (view) {
	gfx::Point scroll = view->getViewScroll();
	scroll.y += (jmouse_z(1)-jmouse_z(0)) * 3 * m_boxsize;
	view->setViewScroll(scroll);
      }
      break;
    }

    case JM_MOUSELEAVE:
      app_get_statusbar()->clearText();
      break;

  }

  return Widget::onProcessMessage(msg);
}

void PaletteView::request_size(int* w, int* h)
{
  div_t d = div(256, m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem)? 1: 0);

  *w = this->border_width.l + this->border_width.r +
    + cols*m_boxsize + (cols-1)*this->child_spacing;

  *h = this->border_width.t + this->border_width.b +
    + rows*m_boxsize + (rows-1)*this->child_spacing;
}

void PaletteView::update_scroll(int color)
{
  View* view = View::getView(this);
  if (!view)
    return;

  gfx::Rect vp = view->getViewportBounds();
  gfx::Point scroll;
  int x, y, cols;
  div_t d;

  scroll = view->getViewScroll();

  d = div(256, m_columns);
  cols = m_columns;

  y = (m_boxsize+this->child_spacing) * (color / cols);
  x = (m_boxsize+this->child_spacing) * (color % cols);

  if (scroll.x > x)
    scroll.x = x;
  else if (scroll.x+vp.w-m_boxsize-2 < x)
    scroll.x = x-vp.w+m_boxsize+2;

  if (scroll.y > y)
    scroll.y = y;
  else if (scroll.y+vp.h-m_boxsize-2 < y)
    scroll.y = y-vp.h+m_boxsize+2;

  view->setViewScroll(scroll);
}
