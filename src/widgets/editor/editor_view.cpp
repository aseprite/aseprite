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

#include "widgets/editor/editor_view.h"

#include "gui/list.h"
#include "gui/message.h"
#include "modules/editors.h"
#include "skin/skin_theme.h"
#include "widgets/editor/editor.h"

#include <allegro.h>

EditorView::EditorView(EditorView::Type type)
  : View()
  , m_type(type)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  int l = theme->get_part(PART_EDITOR_SELECTED_W)->w;
  int t = theme->get_part(PART_EDITOR_SELECTED_N)->h;
  int r = theme->get_part(PART_EDITOR_SELECTED_E)->w;
  int b = theme->get_part(PART_EDITOR_SELECTED_S)->h;

  jwidget_set_border(this, l, t, r, b);
  hideScrollBars();
}

bool EditorView::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      // This avoid the displacement of the widgets in the viewport

      jrect_copy(this->rc, &msg->setpos.rect);
      updateView();
      return true;

    case JM_DRAW:
      {
        Widget* viewport = getViewport();
        Widget* child = reinterpret_cast<JWidget>(jlist_first_data(viewport->children));
        JRect pos = jwidget_get_rect(this);
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

        theme->draw_bounds_nw(ji_screen,
                              pos->x1, pos->y1,
                              pos->x2-1, pos->y2-1,
                              selected ? PART_EDITOR_SELECTED_NW:
                                         PART_EDITOR_NORMAL_NW, false);

        jrect_free(pos);
      }
      return true;

  }

  return View::onProcessMessage(msg);
}
