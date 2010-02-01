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

#ifndef WIDGETS_EDITOR_H_INCLUDED
#define WIDGETS_EDITOR_H_INCLUDED

#include "jinete/jbase.h"
#include "jinete/jwidget.h"
#include "core/color.h"

#define MIN_ZOOM 0
#define MAX_ZOOM 5

class Sprite;

class Editor : public Widget
{
  // editor states
  enum {
    EDIT_STANDBY,
    EDIT_MOVING_SCROLL,
    EDIT_DRAWING,
  };

  /* main stuff */
  int m_state;
  Sprite* m_sprite;
  int m_zoom;

  /* drawing cursor */
  int m_cursor_thick;
  int m_cursor_screen_x; /* position in the screen (view) */
  int m_cursor_screen_y;
  int m_cursor_editor_x; /* position in the editor (model) */
  int m_cursor_editor_y;
  int m_old_cursor_thick;
  bool m_cursor_candraw : 1;

  bool m_alt_pressed : 1;
  bool m_ctrl_pressed : 1;
  bool m_space_pressed : 1;

  /* offset for the sprite */
  int m_offset_x;
  int m_offset_y;

  /* marching ants stuff */
  int m_mask_timer_id;
  int m_offset_count;

  /* region that must be updated */
  JRegion m_refresh_region;

public:
  // in editor.c

  Editor();
  ~Editor();

  int editor_get_zoom() const { return m_zoom; }
  int editor_get_offset_x() const { return m_offset_x; }
  int editor_get_offset_y() const { return m_offset_y; }
  Sprite* editor_get_sprite() { return m_sprite; }
  int editor_get_cursor_thick() { return m_cursor_thick; }

  void editor_set_zoom(int zoom) { m_zoom = zoom; }
  void editor_set_offset_x(int x) { m_offset_x = x; }
  void editor_set_offset_y(int y) { m_offset_y = y; }
  void editor_set_sprite(Sprite* sprite);

  void editor_set_scroll(int x, int y, int use_refresh_region);
  void editor_update();

  void editor_draw_sprite(int x1, int y1, int x2, int y2);
  void editor_draw_sprite_safe(int x1, int y1, int x2, int y2);

  void editor_draw_mask();
  void editor_draw_mask_safe();

  void screen_to_editor(int xin, int yin, int *xout, int *yout);
  void editor_to_screen(int xin, int yin, int *xout, int *yout);

  void show_drawing_cursor();
  void hide_drawing_cursor();

  void editor_update_statusbar_for_standby();

  void editor_refresh_region();

  // in cursor.c

  static void editor_cursor_exit();

  void editor_draw_cursor(int x, int y);
  void editor_clean_cursor();
  bool editor_cursor_is_subpixel();

  // keys.c

  bool editor_keys_toset_zoom(int scancode);
  bool editor_keys_toset_brushsize(int scancode);

  // click.c

  enum {
    MODE_CLICKANDRELEASE,
    MODE_CLICKANDCLICK,
  };

  void editor_click_start(int mode, int *x, int *y, int *b);
  void editor_click_continue(int mode, int *x, int *y);
  void editor_click_done();
  int editor_click(int *x, int *y, int *update,
		   void (*scroll_callback) (int before_change));
  int editor_click_cancel();

protected:
  virtual bool msg_proc(JMessage msg);

private:
  void drawGrid();
  void editor_request_size(int *w, int *h);
  void editor_setcursor(int x, int y);
  void editor_update_candraw();

  void for_each_pixel_of_brush(int x, int y, int color,
			       void (*pixel)(BITMAP *bmp, int x, int y, int color));
};

JWidget editor_view_new();
int editor_type();

#endif
