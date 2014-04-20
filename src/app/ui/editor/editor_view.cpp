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

#include "app/ui/editor/editor_view.h"

#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"

#include <allegro.h>

namespace app {

using namespace app::skin;
using namespace ui;

EditorView::EditorView(EditorView::Type type)
  : View()
  , m_type(type)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->w;
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->h;
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->w;
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->h;

  setBorder(gfx::Border(l, t, r, b));
  setupScrollbars();

  UIContext::instance()->settings()->addObserver(this);
}

EditorView::~EditorView()
{
  UIContext::instance()->settings()->removeObserver(this);
}

void EditorView::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  Widget* viewport = getViewport();
  Widget* child = UI_FIRST_WIDGET(viewport->getChildren());
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  bool selected = false;

  switch (m_type) {

    // Only show the view selected if it is the current editor
    case CurrentEditorMode:
      selected = (child == current_editor);
      break;

      // Always show selected
    case AlwaysSelected:
      selected = true;
      break;

  }

  theme->draw_bounds_nw(g, getClientBounds(),
    selected ? PART_EDITOR_SELECTED_NW:
    PART_EDITOR_NORMAL_NW,
    ColorNone);
}

void EditorView::onResize(ResizeEvent& ev)
{
  // This avoid the displacement of the widgets in the viewport

  setBoundsQuietly(ev.getBounds());
  updateView();
}

void EditorView::onSetShowSpriteEditorScrollbars(bool state)
{
  setupScrollbars();
}

void EditorView::setupScrollbars()
{
  if (m_type == AlwaysSelected ||
      !UIContext::instance()->settings()->getShowSpriteEditorScrollbars())
    hideScrollBars();
  else {
    // TODO hardcoded scroll bar width should be get from skin.xml file
    getHorizontalBar()->setBarWidth(6*jguiscale());
    getVerticalBar()->setBarWidth(6*jguiscale());

    setup_mini_look(getHorizontalBar());
    setup_mini_look(getVerticalBar());

    showScrollBars();
  }
}

} // namespace app
