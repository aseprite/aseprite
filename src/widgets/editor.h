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

#ifndef WIDGETS_EDITOR_H
#define WIDGETS_EDITOR_H

#include "jinete/jbase.h"

#define MIN_ZOOM 0
#define MAX_ZOOM 5

struct Sprite;

typedef struct Editor
{
  JWidget widget;

  /* main stuff */
  int state;
  struct Sprite *sprite;
  int zoom;

  /* drawing cursor */
  int cursor_thick;
  int cursor_screen_x; /* position in the screen (view) */
  int cursor_screen_y;
  int cursor_editor_x; /* position in the editor (model) */
  int cursor_editor_y;
  int old_cursor_thick;
  bool cursor_candraw : 1;

  bool alt_pressed : 1;
  bool space_pressed : 1;

  /* offset for the sprite */
  int offset_x;
  int offset_y;

  /* marching ants stuff */
  int mask_timer_id;
  int offset_count;

  /* to save the area that overlap the layer-bound */
  void *rect_data;

  /* region that must be updated */
  JRegion refresh_region;
} Editor;

/**********************************************************************/
/* src/gui/editor/editor.c */

JWidget editor_view_new(void);

JWidget editor_new(void);
int editor_type(void);

Editor *editor_data(JWidget editor);
struct Sprite *editor_get_sprite(JWidget editor);

void editor_set_sprite(JWidget editor, struct Sprite *sprite);
void editor_set_scroll(JWidget editor, int x, int y, int use_refresh_region);
void editor_update(JWidget editor);

void editor_draw_sprite(JWidget editor, int x1, int y1, int x2, int y2);
void editor_draw_sprite_safe(JWidget editor, int x1, int y1, int x2, int y2);

void editor_draw_mask(JWidget editor);
void editor_draw_mask_safe(JWidget editor);

void editor_draw_grid(JWidget editor);
void editor_draw_grid_safe(JWidget editor);

void editor_draw_layer_boundary(JWidget editor);
void editor_draw_layer_boundary_safe(JWidget editor);
void editor_update_layer_boundary(JWidget editor);

void editor_draw_path(JWidget editor, int draw_extras);
void editor_draw_path_safe(JWidget editor, int draw_extras);

void screen_to_editor(JWidget editor, int xin, int yin, int *xout, int *yout);
void editor_to_screen(JWidget editor, int xin, int yin, int *xout, int *yout);

void show_drawing_cursor(JWidget editor);
void hide_drawing_cursor(JWidget editor);

void editor_update_statusbar_for_standby(JWidget editor);

void editor_refresh_region(JWidget editor);

/**********************************************************************/
/* src/gui/editor/cursor.c */

void editor_cursor_exit(void);

void editor_draw_cursor(JWidget editor, int x, int y);
void editor_clean_cursor(JWidget editor);
bool editor_cursor_is_subpixel(JWidget editor);

/**********************************************************************/
/* src/gui/editor/keys.c */

int editor_keys_toset_zoom(JWidget editor, int scancode);
int editor_keys_toset_brushsize(JWidget editor, int scancode);

/**********************************************************************/
/* src/gui/editor/click.c */

enum {
  MODE_CLICKANDRELEASE,
  MODE_CLICKANDCLICK,
};

void editor_click_start(JWidget editor, int mode, int *x, int *y, int *b);
void editor_click_done(JWidget editor);
int editor_click(JWidget editor, int *x, int *y, int *update,
		 void (*scroll_callback) (int before_change));
int editor_click_cancel(JWidget editor);

#endif /* WIDGETS_EDITOR_H */
