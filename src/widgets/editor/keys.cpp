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

#include <allegro/keyboard.h>

#include "app.h"
#include "app/color.h"
#include "gui/rect.h"
#include "gui/system.h"
#include "gui/view.h"
#include "gui/widget.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "settings/settings.h"
#include "ui_context.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"

bool Editor::editor_keys_toset_zoom(int scancode)
{
  if ((m_sprite) &&
      (this->hasMouse()) &&
      !key[KEY_LSHIFT] &&
      !key[KEY_RSHIFT] &&
      !key[KEY_LCONTROL] &&
      !key[KEY_RCONTROL] &&
      !key[KEY_ALT]) {
    View* view = View::getView(this);
    gfx::Rect vp = view->getViewportBounds();
    int x, y, zoom;

    x = 0;
    y = 0;
    zoom = -1;

    switch (scancode) { // TODO make these keys configurable
      case KEY_1: zoom = 0; break;
      case KEY_2: zoom = 1; break;
      case KEY_3: zoom = 2; break;
      case KEY_4: zoom = 3; break;
      case KEY_5: zoom = 4; break;
      case KEY_6: zoom = 5; break;
    }

    // Change zoom
    if (zoom >= 0) {
      editor_set_zoom_and_center_in_mouse(zoom, jmouse_x(0), jmouse_y(0));
      return true;
    }
  }

  return false;
}
