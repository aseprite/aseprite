/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
  , m_currentEntry(-1)
  , m_rangeAnchor(-1)
  , m_selectedEntries(256, false)
{
  m_editable = editable;
  m_columns = 16;
  m_boxsize = 6;

  jwidget_focusrest(this, true);

  this->border_width.l = this->border_width.r = 1 * jguiscale();
  this->border_width.t = this->border_width.b = 1 * jguiscale();
  this->child_spacing = 1 * jguiscale();
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

void PaletteView::clearSelection()
{
  std::fill(m_selectedEntries.begin(),
            m_selectedEntries.end(), false);
}

void PaletteView::selectColor(int index)
{
  ASSERT(index >= 0 && index <= 255);

  if (m_currentEntry != index || !m_selectedEntries[index]) {
    m_currentEntry = index;
    m_rangeAnchor = index;
    m_selectedEntries[index] = true;

    update_scroll(m_currentEntry);
    invalidate();
  }
}

void PaletteView::selectRange(int index1, int index2)
{
  m_rangeAnchor = index1;
  m_currentEntry = index2;

  std::fill(m_selectedEntries.begin()+std::min(index1, index2),
            m_selectedEntries.begin()+std::max(index1, index2)+1, true);

  update_scroll(index2);
  invalidate();
}

int PaletteView::getSelectedEntry() const
{
  return m_currentEntry;
}

bool PaletteView::getSelectedRange(int& index1, int& index2) const
{
  int i, i2, j;

  // Find the first selected entry
  for (i=0; i<(int)m_selectedEntries.size(); ++i)
    if (m_selectedEntries[i])
      break;

  // Find the first unselected entry after i
  for (i2=i+1; i2<(int)m_selectedEntries.size(); ++i2)
    if (!m_selectedEntries[i2])
      break;

  // Find the last selected entry
  for (j=m_selectedEntries.size()-1; j>=0; --j)
    if (m_selectedEntries[j])
      break;

  if (j-i+1 == i2-i) {
    index1 = i;
    index2 = j;
    return true;
  }
  else
    return false;
}

void PaletteView::getSelectedEntries(SelectedEntries& entries) const
{
  entries = m_selectedEntries;
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

bool PaletteView::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      request_size(&msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_DRAW: {
      div_t d = div(256, m_columns);
      int cols = m_columns;
      int rows = d.quot + ((d.rem)? 1: 0);
      int x, y, u, v;
      int c, color;
      BITMAP *bmp;
      Palette* palette = get_current_palette();
      int bordercolor = makecol(255, 255, 255);

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

          if (m_selectedEntries[c]) {
            bool top    = (c >= m_columns            && c-m_columns >= 0  ? m_selectedEntries[c-m_columns]: false);
            bool bottom = (c < 256-m_columns         && c+m_columns < 256 ? m_selectedEntries[c+m_columns]: false);
            bool left   = ((c%m_columns)>0           && c-1         >= 0  ? m_selectedEntries[c-1]: false);
            bool right  = ((c%m_columns)<m_columns-1 && c+1         < 256 ? m_selectedEntries[c+1]: false);

            if (!top) hline(bmp, x-1, y-1, x+m_boxsize, bordercolor);
            if (!bottom) hline(bmp, x-1, y+m_boxsize, x+m_boxsize, bordercolor);
            if (!left) vline(bmp, x-1, y-1, y+m_boxsize, bordercolor);
            if (!right) vline(bmp, x+m_boxsize, y-1, y+m_boxsize, bordercolor);
          }

          x += m_boxsize+this->child_spacing;
          c++;
        }

        y += m_boxsize+this->child_spacing;
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
        int idx = color.getIndex();

        app_get_statusbar()->showColor(0, "", color, 255);

        if (hasCapture() && idx != m_currentEntry) {
          if (!(msg->any.shifts & KB_CTRL_FLAG))
            clearSelection();

          if (msg->any.shifts & KB_SHIFT_FLAG)
            selectRange(m_rangeAnchor, idx);
          else
            selectColor(idx);

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
