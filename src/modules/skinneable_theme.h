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

#ifndef MODULES_SKINNEABLE_THEME_H_INCLUDED
#define MODULES_SKINNEABLE_THEME_H_INCLUDED

#include <map>
#include <string>
#include <allegro/color.h>
#include "jinete/jtheme.h"
#include "jinete/jrect.h"

enum {

  PART_CURSOR_NORMAL,
  PART_CURSOR_NORMAL_ADD,
  PART_CURSOR_FORBIDDEN,
  PART_CURSOR_HAND,
  PART_CURSOR_SCROLL,
  PART_CURSOR_MOVE,
  PART_CURSOR_SIZE_TL,
  PART_CURSOR_SIZE_T,
  PART_CURSOR_SIZE_TR,
  PART_CURSOR_SIZE_L,
  PART_CURSOR_SIZE_R,
  PART_CURSOR_SIZE_BL,
  PART_CURSOR_SIZE_B,
  PART_CURSOR_SIZE_BR,
  PART_CURSOR_EYEDROPPER,

  PART_RADIO_NORMAL,
  PART_RADIO_SELECTED,
  PART_RADIO_DISABLED,

  PART_CHECK_NORMAL,
  PART_CHECK_SELECTED,
  PART_CHECK_DISABLED,

  PART_CHECK_FOCUS_NW,
  PART_CHECK_FOCUS_N,
  PART_CHECK_FOCUS_NE,
  PART_CHECK_FOCUS_E,
  PART_CHECK_FOCUS_SE,
  PART_CHECK_FOCUS_S,
  PART_CHECK_FOCUS_SW,
  PART_CHECK_FOCUS_W,

  PART_RADIO_FOCUS_NW,
  PART_RADIO_FOCUS_N,
  PART_RADIO_FOCUS_NE,
  PART_RADIO_FOCUS_E,
  PART_RADIO_FOCUS_SE,
  PART_RADIO_FOCUS_S,
  PART_RADIO_FOCUS_SW,
  PART_RADIO_FOCUS_W,

  PART_BUTTON_NORMAL_NW,
  PART_BUTTON_NORMAL_N,
  PART_BUTTON_NORMAL_NE,
  PART_BUTTON_NORMAL_E,
  PART_BUTTON_NORMAL_SE,
  PART_BUTTON_NORMAL_S,
  PART_BUTTON_NORMAL_SW,
  PART_BUTTON_NORMAL_W,

  PART_BUTTON_HOT_NW,
  PART_BUTTON_HOT_N,
  PART_BUTTON_HOT_NE,
  PART_BUTTON_HOT_E,
  PART_BUTTON_HOT_SE,
  PART_BUTTON_HOT_S,
  PART_BUTTON_HOT_SW,
  PART_BUTTON_HOT_W,

  PART_BUTTON_FOCUSED_NW,
  PART_BUTTON_FOCUSED_N,
  PART_BUTTON_FOCUSED_NE,
  PART_BUTTON_FOCUSED_E,
  PART_BUTTON_FOCUSED_SE,
  PART_BUTTON_FOCUSED_S,
  PART_BUTTON_FOCUSED_SW,
  PART_BUTTON_FOCUSED_W,

  PART_BUTTON_SELECTED_NW,
  PART_BUTTON_SELECTED_N,
  PART_BUTTON_SELECTED_NE,
  PART_BUTTON_SELECTED_E,
  PART_BUTTON_SELECTED_SE,
  PART_BUTTON_SELECTED_S,
  PART_BUTTON_SELECTED_SW,
  PART_BUTTON_SELECTED_W,

  PART_SUNKEN_NORMAL_NW,
  PART_SUNKEN_NORMAL_N,
  PART_SUNKEN_NORMAL_NE,
  PART_SUNKEN_NORMAL_E,
  PART_SUNKEN_NORMAL_SE,
  PART_SUNKEN_NORMAL_S,
  PART_SUNKEN_NORMAL_SW,
  PART_SUNKEN_NORMAL_W,

  PART_SUNKEN_FOCUSED_NW,
  PART_SUNKEN_FOCUSED_N,
  PART_SUNKEN_FOCUSED_NE,
  PART_SUNKEN_FOCUSED_E,
  PART_SUNKEN_FOCUSED_SE,
  PART_SUNKEN_FOCUSED_S,
  PART_SUNKEN_FOCUSED_SW,
  PART_SUNKEN_FOCUSED_W,

  PART_SUNKEN2_NORMAL_NW,
  PART_SUNKEN2_NORMAL_N,
  PART_SUNKEN2_NORMAL_NE,
  PART_SUNKEN2_NORMAL_E,
  PART_SUNKEN2_NORMAL_SE,
  PART_SUNKEN2_NORMAL_S,
  PART_SUNKEN2_NORMAL_SW,
  PART_SUNKEN2_NORMAL_W,

  PART_SUNKEN2_FOCUSED_NW,
  PART_SUNKEN2_FOCUSED_N,
  PART_SUNKEN2_FOCUSED_NE,
  PART_SUNKEN2_FOCUSED_E,
  PART_SUNKEN2_FOCUSED_SE,
  PART_SUNKEN2_FOCUSED_S,
  PART_SUNKEN2_FOCUSED_SW,
  PART_SUNKEN2_FOCUSED_W,

  PART_WINDOW_NW,
  PART_WINDOW_N,
  PART_WINDOW_NE,
  PART_WINDOW_E,
  PART_WINDOW_SE,
  PART_WINDOW_S,
  PART_WINDOW_SW,
  PART_WINDOW_W,

  PART_MENU_NW,
  PART_MENU_N,
  PART_MENU_NE,
  PART_MENU_E,
  PART_MENU_SE,
  PART_MENU_S,
  PART_MENU_SW,
  PART_MENU_W,

  PART_WINDOW_CLOSE_BUTTON_NORMAL,
  PART_WINDOW_CLOSE_BUTTON_HOT,
  PART_WINDOW_CLOSE_BUTTON_SELECTED,

  PART_SLIDER_FULL_NW,
  PART_SLIDER_FULL_N,
  PART_SLIDER_FULL_NE,
  PART_SLIDER_FULL_E,
  PART_SLIDER_FULL_SE,
  PART_SLIDER_FULL_S,
  PART_SLIDER_FULL_SW,
  PART_SLIDER_FULL_W,

  PART_SLIDER_EMPTY_NW,
  PART_SLIDER_EMPTY_N,
  PART_SLIDER_EMPTY_NE,
  PART_SLIDER_EMPTY_E,
  PART_SLIDER_EMPTY_SE,
  PART_SLIDER_EMPTY_S,
  PART_SLIDER_EMPTY_SW,
  PART_SLIDER_EMPTY_W,

  PART_SLIDER_FULL_FOCUSED_NW,
  PART_SLIDER_FULL_FOCUSED_N,
  PART_SLIDER_FULL_FOCUSED_NE,
  PART_SLIDER_FULL_FOCUSED_E,
  PART_SLIDER_FULL_FOCUSED_SE,
  PART_SLIDER_FULL_FOCUSED_S,
  PART_SLIDER_FULL_FOCUSED_SW,
  PART_SLIDER_FULL_FOCUSED_W,

  PART_SLIDER_EMPTY_FOCUSED_NW,
  PART_SLIDER_EMPTY_FOCUSED_N,
  PART_SLIDER_EMPTY_FOCUSED_NE,
  PART_SLIDER_EMPTY_FOCUSED_E,
  PART_SLIDER_EMPTY_FOCUSED_SE,
  PART_SLIDER_EMPTY_FOCUSED_S,
  PART_SLIDER_EMPTY_FOCUSED_SW,
  PART_SLIDER_EMPTY_FOCUSED_W,

  PART_SEPARATOR,

  PART_COMBOBOX_ARROW,

  PART_TOOLBUTTON_NORMAL_NW,
  PART_TOOLBUTTON_NORMAL_N,
  PART_TOOLBUTTON_NORMAL_NE,
  PART_TOOLBUTTON_NORMAL_E,
  PART_TOOLBUTTON_NORMAL_SE,
  PART_TOOLBUTTON_NORMAL_S,
  PART_TOOLBUTTON_NORMAL_SW,
  PART_TOOLBUTTON_NORMAL_W,

  PART_TOOLBUTTON_HOT_NW,
  PART_TOOLBUTTON_HOT_N,
  PART_TOOLBUTTON_HOT_NE,
  PART_TOOLBUTTON_HOT_E,
  PART_TOOLBUTTON_HOT_SE,
  PART_TOOLBUTTON_HOT_S,
  PART_TOOLBUTTON_HOT_SW,
  PART_TOOLBUTTON_HOT_W,

  PART_TOOLBUTTON_LAST_NW,
  PART_TOOLBUTTON_LAST_N,
  PART_TOOLBUTTON_LAST_NE,
  PART_TOOLBUTTON_LAST_E,
  PART_TOOLBUTTON_LAST_SE,
  PART_TOOLBUTTON_LAST_S,
  PART_TOOLBUTTON_LAST_SW,
  PART_TOOLBUTTON_LAST_W,

  PART_TAB_NORMAL_NW,
  PART_TAB_NORMAL_N,
  PART_TAB_NORMAL_NE,
  PART_TAB_NORMAL_E,
  PART_TAB_NORMAL_SE,
  PART_TAB_NORMAL_S,
  PART_TAB_NORMAL_SW,
  PART_TAB_NORMAL_W,

  PART_TAB_SELECTED_NW,
  PART_TAB_SELECTED_N,
  PART_TAB_SELECTED_NE,
  PART_TAB_SELECTED_E,
  PART_TAB_SELECTED_SE,
  PART_TAB_SELECTED_S,
  PART_TAB_SELECTED_SW,
  PART_TAB_SELECTED_W,

  PART_TAB_BOTTOM_SELECTED_NW,
  PART_TAB_BOTTOM_SELECTED_N,
  PART_TAB_BOTTOM_SELECTED_NE,
  PART_TAB_BOTTOM_SELECTED_E,
  PART_TAB_BOTTOM_SELECTED_SE,
  PART_TAB_BOTTOM_SELECTED_S,
  PART_TAB_BOTTOM_SELECTED_SW,
  PART_TAB_BOTTOM_SELECTED_W,

  PART_TAB_BOTTOM_NORMAL,

  PART_TAB_FILLER,

  PART_EDITOR_NORMAL_NW,
  PART_EDITOR_NORMAL_N,
  PART_EDITOR_NORMAL_NE,
  PART_EDITOR_NORMAL_E,
  PART_EDITOR_NORMAL_SE,
  PART_EDITOR_NORMAL_S,
  PART_EDITOR_NORMAL_SW,
  PART_EDITOR_NORMAL_W,

  PART_EDITOR_SELECTED_NW,
  PART_EDITOR_SELECTED_N,
  PART_EDITOR_SELECTED_NE,
  PART_EDITOR_SELECTED_E,
  PART_EDITOR_SELECTED_SE,
  PART_EDITOR_SELECTED_S,
  PART_EDITOR_SELECTED_SW,
  PART_EDITOR_SELECTED_W,

  PARTS
};

class SkinneableTheme : public jtheme
{
  BITMAP* m_sheet_bmp;
  BITMAP* m_part[PARTS];
  std::map<std::string, BITMAP*> m_toolicon;

public:
  SkinneableTheme();
  ~SkinneableTheme();

  void regen();
  BITMAP* set_cursor(int type, int* focus_x, int* focus_y);
  void init_widget(JWidget widget);
  JRegion get_window_mask(JWidget widget);
  void map_decorative_widget(JWidget widget);

  int color_foreground();
  int color_disabled();
  int color_face();
  int color_hotface();
  int color_selected();
  int color_background();

  void draw_box(JWidget widget, JRect clip);
  void draw_button(JWidget widget, JRect clip);
  void draw_check(JWidget widget, JRect clip);
  void draw_entry(JWidget widget, JRect clip);
  void draw_grid(JWidget widget, JRect clip);
  void draw_label(JWidget widget, JRect clip);
  void draw_listbox(JWidget widget, JRect clip);
  void draw_listitem(JWidget widget, JRect clip);
  void draw_menu(JWidget widget, JRect clip);
  void draw_menuitem(JWidget widget, JRect clip);
  void draw_panel(JWidget widget, JRect clip);
  void draw_radio(JWidget widget, JRect clip);
  void draw_separator(JWidget widget, JRect clip);
  void draw_slider(JWidget widget, JRect clip);
  void draw_combobox_entry(JWidget widget, JRect clip);
  void draw_combobox_button(JWidget widget, JRect clip);
  void draw_textbox(JWidget widget, JRect clip);
  void draw_view(JWidget widget, JRect clip);
  void draw_view_scrollbar(JWidget widget, JRect clip);
  void draw_view_viewport(JWidget widget, JRect clip);
  void draw_frame(Frame* frame, JRect clip);
  void draw_frame_button(JWidget widget, JRect clip);

  int get_button_normal_text_color() const { return makecol(0, 0, 0); }
  int get_button_normal_face_color() const { return makecol(198, 198, 198); }
  int get_button_hot_text_color() const { return makecol(0, 0, 0); }
  int get_button_hot_face_color() const { return makecol(255, 255, 255); }
  int get_button_selected_text_color() const { return makecol(255, 255, 255); }
  int get_button_selected_face_color() const { return makecol(124, 144, 159); }
  int get_button_selected_offset() const { return 0; }

  int get_check_hot_face_color() const { return makecol(255, 235, 182); }
  int get_check_focus_face_color() const { return makecol(198, 198, 198); }

  int get_radio_hot_face_color() const { return makecol(255, 235, 182); }
  int get_radio_focus_face_color() const { return makecol(198, 198, 198); }

  int get_menuitem_normal_text_color() const { return makecol(0, 0, 0); }
  int get_menuitem_normal_face_color() const { return makecol(211, 203, 190); }
  int get_menuitem_hot_text_color() const { return makecol(0, 0, 0); }
  int get_menuitem_hot_face_color() const { return makecol(255, 235, 182); }
  int get_menuitem_highlight_text_color() const { return makecol(255, 255, 255); }
  int get_menuitem_highlight_face_color() const { return makecol(124, 144, 159); }

  int get_window_face_color() const { return makecol(211, 203, 190); }
  int get_window_titlebar_text_color() const { return makecol(255, 255, 255); }
  int get_window_titlebar_face_color() const { return makecol(124, 144, 159); }

  int get_editor_face_color() const { return makecol(101, 85, 97); }
  int get_editor_sprite_border() const { return makecol(0, 0, 0); }
  int get_editor_sprite_bottom_edge() const { return makecol(65, 65, 44); }

  int get_listitem_normal_text_color() const { return makecol(0, 0, 0); }
  int get_listitem_normal_face_color() const { return makecol(255, 255, 255); }
  int get_listitem_selected_text_color() const { return makecol(255, 255, 255); }
  int get_listitem_selected_face_color() const { return makecol(255, 85, 85); }

  int get_slider_empty_text_color() const { return makecol(0, 0, 0); }
  int get_slider_empty_face_color() const { return makecol(174, 203, 223); }
  int get_slider_full_text_color() const { return makecol(255, 255, 255); }
  int get_slider_full_face_color() const { return makecol(125, 146, 158); }

  int get_tab_selected_text_color() const { return makecol(255, 255, 255); }
  int get_tab_selected_face_color() const { return makecol(125, 146, 158); }
  int get_tab_normal_text_color() const { return makecol(0, 0, 0); }
  int get_tab_normal_face_color() const { return makecol(199, 199, 199); }

  int get_panel_face_color() const { return makecol(125, 146, 158); }

  BITMAP* get_part(int part_i) const { return m_part[part_i]; }
  BITMAP* get_toolicon(const char* tool_id) const;

  // helper functions to draw parts
  void draw_bounds(int x1, int y1, int x2, int y2, int nw, int bg);
  void draw_bounds2(int x1, int y1, int x2, int y2, int x_mid, int nw1, int nw2, int bg1, int bg2);
  void draw_hline(int x1, int y1, int x2, int y2, int part);

  // Wrapper to use the new "Rect" class (x, y, w, h)
  void draw_bounds(const Rect& rc, int nw, int bg) {
    draw_bounds(rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, nw, bg);
  }

private:

  int get_bg_color(JWidget widget);
  void draw_textstring(const char *t, int fg_color, int bg_color,
		       bool fill_bg, JWidget widget, const JRect rect,
		       int selected_offset);
  void draw_entry_cursor(JWidget widget, int x, int y);
  void draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel);
  void less_bevel(int *bevel);
  BITMAP* apply_gui_scale(BITMAP* original);

  static bool theme_frame_button_msg_proc(JWidget widget, JMessage msg);
  static bool theme_combobox_button_msg_proc(JWidget widget, JMessage msg);

};

#endif
