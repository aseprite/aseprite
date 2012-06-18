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

#ifndef SKIN_THEME_H_INCLUDED
#define SKIN_THEME_H_INCLUDED

#include <allegro/color.h>
#include <map>
#include <string>

#include "gfx/rect.h"
#include "gui/rect.h"
#include "gui/theme.h"
#include "gui/system.h"

#include "skin/skin_parts.h"

namespace ui {
  class Graphics;
  class IButtonIcon;
}

// This is the GUI theme used by ASEPRITE (which use images from data/skins
// directory).
class SkinTheme : public ui::Theme
{
  std::string m_selected_skin;
  BITMAP* m_sheet_bmp;
  BITMAP* m_cursors[ui::JI_CURSORS];
  BITMAP* m_part[PARTS];
  std::map<std::string, BITMAP*> m_toolicon;
  FONT* m_minifont;

public:
  static const char* kThemeCloseButtonId;

  SkinTheme();
  ~SkinTheme();

  FONT* getMiniFont() const { return m_minifont; }

  void reload_skin();
  void reload_fonts();

  BITMAP* set_cursor(int type, int* focus_x, int* focus_y);
  void init_widget(ui::Widget* widget);
  ui::JRegion get_window_mask(ui::Widget* widget);
  void map_decorative_widget(ui::Widget* widget);

  int color_foreground();
  int color_disabled();
  int color_face();
  int color_hotface();
  int color_selected();
  int color_background();

  void paintBox(ui::PaintEvent& ev);
  void paintButton(ui::PaintEvent& ev);
  void paintCheckBox(ui::PaintEvent& ev);
  void paintEntry(ui::PaintEvent& ev);
  void paintGrid(ui::PaintEvent& ev);
  void paintLabel(ui::PaintEvent& ev);
  void paintLinkLabel(ui::PaintEvent& ev);
  void draw_listbox(ui::Widget* widget, ui::JRect clip);
  void draw_listitem(ui::Widget* widget, ui::JRect clip);
  void draw_menu(ui::Menu* menu, ui::JRect clip);
  void draw_menuitem(ui::MenuItem* menuitem, ui::JRect clip);
  void draw_panel(ui::Widget* widget, ui::JRect clip);
  void paintRadioButton(ui::PaintEvent& ev);
  void draw_separator(ui::Widget* widget, ui::JRect clip);
  void paintSlider(ui::PaintEvent& ev);
  void draw_combobox_entry(ui::Entry* widget, ui::JRect clip);
  void paintComboBoxButton(ui::PaintEvent& ev);
  void draw_textbox(ui::Widget* widget, ui::JRect clip);
  void draw_view(ui::Widget* widget, ui::JRect clip);
  void draw_view_scrollbar(ui::Widget* widget, ui::JRect clip);
  void draw_view_viewport(ui::Widget* widget, ui::JRect clip);
  void paintFrame(ui::PaintEvent& ev);
  void draw_frame_button(ui::ButtonBase* widget, ui::JRect clip);
  void paintTooltip(ui::PaintEvent& ev);

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

  int get_scrollbar_bg_face_color() const { return makecol(125, 146, 158); }
  int get_scrollbar_thumb_face_color() const { return makecol(199, 199, 199); }

  BITMAP* get_part(int part_i) const { return m_part[part_i]; }
  BITMAP* get_toolicon(const char* tool_id) const;

  // helper functions to draw bounds/hlines with sheet parts
  void draw_bounds_array(BITMAP* bmp, int x1, int y1, int x2, int y2, int parts[8]);
  void draw_bounds_nw(BITMAP* bmp, int x1, int y1, int x2, int y2, int nw, int bg = -1);
  void draw_bounds_nw(ui::Graphics* g, const gfx::Rect& rc, int nw, int bg = -1);
  void draw_bounds_nw2(ui::Graphics* g, const gfx::Rect& rc, int x_mid, int nw1, int nw2, int bg1, int bg2);
  void draw_part_as_hline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part);
  void draw_part_as_vline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part);

  // Wrapper to use the new "Rect" class (x, y, w, h)
  void draw_bounds_nw(BITMAP* bmp, const gfx::Rect& rc, int nw, int bg) {
    draw_bounds_nw(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, nw, bg);
  }

protected:

  void onRegenerate();

private:
  void draw_bounds_template(BITMAP* bmp, int x1, int y1, int x2, int y2,
                            int nw, int n, int ne, int e, int se, int s, int sw, int w);
  void draw_bounds_template(ui::Graphics* g, const gfx::Rect& rc,
                            int nw, int n, int ne, int e, int se, int s, int sw, int w);

  BITMAP* cropPartFromSheet(BITMAP* bmp, int x, int y, int w, int h, bool cursor = false);
  int get_bg_color(ui::Widget* widget);
  void draw_textstring(const char *t, int fg_color, int bg_color,
                       bool fill_bg, ui::Widget* widget, const ui::JRect rect,
                       int selected_offset);
  void draw_textstring(ui::Graphics* g, const char *t, int fg_color, int bg_color,
                       bool fill_bg, ui::Widget* widget, const gfx::Rect& rc,
                       int selected_offset);
  void draw_entry_caret(ui::Entry* widget, int x, int y);
  void draw_bevel_box(int x1, int y1, int x2, int y2, int c1, int c2, int *bevel);
  void less_bevel(int *bevel);

  void paintIcon(ui::Widget* widget, ui::Graphics* g, ui::IButtonIcon* iconInterface, int x, int y);

  static FONT* loadFont(const char* userFont, const std::string& path);

};

#endif
