/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>
#include <stdlib.h>
#include <string.h>

#include "app/app.h"
#include "app/color.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/palette_view.h"
#include "app/ui/status_bar.h"
#include "gfx/point.h"
#include "raster/blend.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "ui/graphics.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

namespace app {

using namespace ui;

WidgetType palette_view_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

PaletteView::PaletteView(bool editable)
  : Widget(palette_view_type())
  , m_currentEntry(-1)
  , m_rangeAnchor(-1)
  , m_selectedEntries(Palette::MaxColors, false)
  , m_isUpdatingColumns(false)
{
  setFocusStop(true);
  setDoubleBuffered(true);

  m_editable = editable;
  m_columns = 16;
  m_boxsize = 6*jguiscale();

  this->border_width.l = this->border_width.r = 1 * jguiscale();
  this->border_width.t = this->border_width.b = 1 * jguiscale();
  this->child_spacing = 1 * jguiscale();
}

void PaletteView::setColumns(int columns)
{
  int old_columns = m_columns;

  ASSERT(columns >= 1 && columns <= Palette::MaxColors);
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
  ASSERT(index >= 0 && index < Palette::MaxColors);

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

app::Color PaletteView::getColorByPosition(int target_x, int target_y)
{
  Palette* palette = get_current_palette();
  gfx::Rect cpos = getChildrenBounds();
  div_t d = div(Palette::MaxColors, m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem)? 1: 0);
  int req_w, req_h;
  int x, y, u, v;
  int c;

  request_size(&req_w, &req_h);

  y = cpos.y;
  c = 0;

  for (v=0; v<rows; v++) {
    x = cpos.x;

    for (u=0; u<cols; u++) {
      if (c >= palette->size())
        break;

      if ((target_x >= x) && (target_x <= x+m_boxsize) &&
          (target_y >= y) && (target_y <= y+m_boxsize))
        return app::Color::fromIndex(c);

      x += m_boxsize+this->child_spacing;
      c++;
    }

    y += m_boxsize+this->child_spacing;
  }

  return app::Color::fromMask();
}

bool PaletteView::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      captureMouse();
      /* continue... */

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      gfx::Rect cpos = getChildrenBounds();

      int req_w, req_h;
      request_size(&req_w, &req_h);

      int mouse_x = MID(cpos.x, mouseMsg->position().x, cpos.x+req_w-this->border_width.r-1);
      int mouse_y = MID(cpos.y, mouseMsg->position().y, cpos.y+req_h-this->border_width.b-1);

      app::Color color = getColorByPosition(mouse_x, mouse_y);
      if (color.getType() == app::Color::IndexType) {
        int idx = color.getIndex();

        StatusBar::instance()->showColor(0, "", color, 255);

        if (hasCapture() && idx != m_currentEntry) {
          if (!msg->ctrlPressed())
            clearSelection();

          if (msg->shiftPressed())
            selectRange(m_rangeAnchor, idx);
          else
            selectColor(idx);

          // Emit signal
          IndexChange(idx);
        }
      }

      if (hasCapture())
        return true;

      break;
    }

    case kMouseUpMessage:
      releaseMouse();
      return true;

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->getViewScroll();
        scroll.y += -static_cast<MouseMessage*>(msg)->wheelDelta() * 3 * m_boxsize;
        view->setViewScroll(scroll);
      }
      break;
    }

    case kMouseLeaveMessage:
      StatusBar::instance()->clearText();
      break;

  }

  return Widget::onProcessMessage(msg);
}

void PaletteView::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.getGraphics();
  gfx::Rect bounds = getClientBounds();
  div_t d = div(Palette::MaxColors, m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem)? 1: 0);
  int x, y, u, v;
  int c, color;
  Palette* palette = get_current_palette();
  int bordercolor = makecol(255, 255, 255);

  g->fillRect(ui::rgba(0 , 0, 0), bounds);

  y = bounds.y + this->border_width.t;
  c = 0;

  for (v=0; v<rows; v++) {
    x = bounds.x + this->border_width.l;

    for (u=0; u<cols; u++) {
      if (c >= palette->size())
        break;

      color = ui::rgba(
        rgba_getr(palette->getEntry(c)),
        rgba_getg(palette->getEntry(c)),
        rgba_getb(palette->getEntry(c)));

      g->fillRect(color, gfx::Rect(x, y, m_boxsize, m_boxsize));

      if (m_selectedEntries[c]) {
        const int max = Palette::MaxColors;
        bool top    = (c >= m_columns            && c-m_columns >= 0  ? m_selectedEntries[c-m_columns]: false);
        bool bottom = (c < max-m_columns         && c+m_columns < max ? m_selectedEntries[c+m_columns]: false);
        bool left   = ((c%m_columns)>0           && c-1         >= 0  ? m_selectedEntries[c-1]: false);
        bool right  = ((c%m_columns)<m_columns-1 && c+1         < max ? m_selectedEntries[c+1]: false);

        if (!top   ) g->drawHLine(bordercolor, x-1, y-1, m_boxsize+2);
        if (!bottom) g->drawHLine(bordercolor, x-1, y+m_boxsize, m_boxsize+2);
        if (!left  ) g->drawVLine(bordercolor, x-1, y-1, m_boxsize+2);
        if (!right ) g->drawVLine(bordercolor, x+m_boxsize, y-1, m_boxsize+2);
      }

      x += m_boxsize+this->child_spacing;
      c++;
    }

    y += m_boxsize+this->child_spacing;
  }
}

void PaletteView::onResize(ui::ResizeEvent& ev)
{
  if (!m_isUpdatingColumns) {
    m_isUpdatingColumns = true;
    View* view = View::getView(this);
    if (view) {
      int columns =
        (view->getViewportBounds().w-this->child_spacing*2)
        / (m_boxsize+this->child_spacing);
      setColumns(MID(1, columns, Palette::MaxColors));
    }
    m_isUpdatingColumns = false;
  }

  Widget::onResize(ev);
}

void PaletteView::onPreferredSize(ui::PreferredSizeEvent& ev)
{
  gfx::Size sz;
  request_size(&sz.w, &sz.h);
  ev.setPreferredSize(sz);
}

void PaletteView::request_size(int* w, int* h)
{
  div_t d = div(Palette::MaxColors, m_columns);
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

  d = div(Palette::MaxColors, m_columns);
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

} // namespace app
