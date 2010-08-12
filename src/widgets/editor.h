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
#include "Vaca/Signal.h"

#define MIN_ZOOM 0
#define MAX_ZOOM 5

class Context;
class Sprite;
class IToolLoop;
class ToolLoopManager;
class PixelsMovement;
class Tool;

class Editor : public Widget
{

  // Editor's decorators. These are useful to add some extra-graphics
  // visualizators in the editor to show feedback and get user events (click)
  class Decorator
  {
  public:
    enum Type {
      SELECTION_NW,
      SELECTION_N,
      SELECTION_NE,
      SELECTION_E,
      SELECTION_SE,
      SELECTION_S,
      SELECTION_SW,
      SELECTION_W,
    };
  private:
    Type m_type;
    Rect m_bounds;
  public:
    Decorator(Type type, const Rect& bounds);
    ~Decorator();

    void drawDecorator(Editor*editor, BITMAP* bmp);
    bool isInsideDecorator(int x, int y);

    Vaca::Signal0<void> Click;
  };

  // editor states
  enum State {
    EDITOR_STATE_STANDBY,
    EDITOR_STATE_SCROLLING,
    EDITOR_STATE_DRAWING,
  };

  // Main properties
  State m_state;		// Editor main state
  Sprite* m_sprite;		// Current sprite in the editor
  int m_zoom;			// Current zoom in the editor

  // Drawing cursor
  int m_cursor_thick;
  int m_cursor_screen_x; /* position in the screen (view) */
  int m_cursor_screen_y;
  int m_cursor_editor_x; /* position in the editor (model) */
  int m_cursor_editor_y;
  int m_old_cursor_thick;
  bool m_cursor_candraw : 1;

  // True if the cursor is inside the mask/selection
  bool m_insideSelection : 1;

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

  // Tool-loop manager
  ToolLoopManager* m_toolLoopManager;

  // Decorators
  std::vector<Decorator*> m_decorators;

  // Helper member to move selection. If this member is NULL it means the
  // user is not moving pixels.
  PixelsMovement* m_pixelsMovement;

public:
  // in editor.c

  Editor();
  ~Editor();

  int editor_get_zoom() const { return m_zoom; }
  int editor_get_offset_x() const { return m_offset_x; }
  int editor_get_offset_y() const { return m_offset_y; }
  Sprite* getSprite() { return m_sprite; }
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

  void flashCurrentLayer();
  void setMaskColorForPixelsMovement(color_t color);

  void screen_to_editor(int xin, int yin, int *xout, int *yout);
  void editor_to_screen(int xin, int yin, int *xout, int *yout);

  void show_drawing_cursor();
  void hide_drawing_cursor();

  void editor_update_statusbar_for_standby();

  void editor_refresh_region();

  // in cursor.c

  static int get_raw_cursor_color();
  static bool is_cursor_mask();
  static color_t get_cursor_color();
  static void set_cursor_color(color_t color);

  static void editor_cursor_init();
  static void editor_cursor_exit();

private:

  void editor_update_statusbar_for_pixel_movement();

  void editor_draw_cursor(int x, int y, bool refresh = true);
  void editor_move_cursor(int x, int y, bool refresh = true);
  void editor_clean_cursor(bool refresh = true);
  bool editor_cursor_is_subpixel();

  // keys.c

  bool editor_keys_toset_zoom(int scancode);

public:

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
  bool onProcessMessage(JMessage msg);
  void onCurrentToolChange();

private:
  void drawGrid(const Rect& gridBounds, color_t color);

  void addDecorator(Decorator* decorator);
  void deleteDecorators();
  void turnOnSelectionModifiers();
  void controlInfiniteScroll(JMessage msg);
  void dropPixels();

  void editor_request_size(int *w, int *h);
  void editor_setcursor(int x, int y);
  void editor_update_candraw();
  void editor_set_zoom_and_center_in_mouse(int zoom, int mouse_x, int mouse_y);

  IToolLoop* createToolLoopImpl(Context* context, JMessage msg);

  void for_each_pixel_of_pen(int screen_x, int screen_y,
			     int sprite_x, int sprite_y, int color,
			     void (*pixel)(BITMAP *bmp, int x, int y, int color));
};

JWidget editor_view_new();
int editor_type();

#endif
