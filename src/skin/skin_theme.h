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

#include "gfx/rect.h"
#include "skin/skin_parts.h"
#include "ui/color.h"
#include "ui/rect.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <map>
#include <string>

#include <allegro/color.h>

namespace ui {
  class Graphics;
  class IButtonIcon;
}

namespace ThemeColor {
  enum Type {
    Text,
    Disabled,
    Face,
    HotFace,
    Selected,
    Background,
    Desktop,
    TextBoxText,
    TextBoxFace,
    LinkText,
    ButtonNormalText,
    ButtonNormalFace,
    ButtonHotText,
    ButtonHotFace,
    ButtonSelectedText,
    ButtonSelectedFace,
    CheckHotFace,
    CheckFocusFace,
    RadioHotFace,
    RadioFocusFace,
    MenuItemNormalText,
    MenuItemNormalFace,
    MenuItemHotText,
    MenuItemHotFace,
    MenuItemHighlightText,
    MenuItemHighlightFace,
    WindowFace,
    WindowTitlebarText,
    WindowTitlebarFace,
    EditorFace,
    EditorSpriteBorder,
    EditorSpriteBottomBorder,
    ListItemNormalText,
    ListItemNormalFace,
    ListItemSelectedText,
    ListItemSelectedFace,
    SliderEmptyText,
    SliderEmptyFace,
    SliderFullText,
    SliderFullFace,
    TabNormalText,
    TabNormalFace,
    TabSelectedText,
    TabSelectedFace,
    SplitterNormalFace,
    ScrollBarBgFace,
    ScrollBarThumbFace,
    PopupWindowBorder,
    TooltipText,
    TooltipFace,
    FileListEvenRowText,
    FileListEvenRowFace,
    FileListOddRowText,
    FileListOddRowFace,
    FileListSelectedRowText,
    FileListSelectedRowFace,
    FileListDisabledRowText,
    Workspace,
    MaxColors
  };
} 

// This is the GUI theme used by ASEPRITE (which use images from data/skins
// directory).
class SkinTheme : public ui::Theme
{
public:
  static const char* kThemeCloseButtonId;

  SkinTheme();
  ~SkinTheme();

  ui::Color getColor(ThemeColor::Type k) const {
    return m_colors[k];
  }

  FONT* getMiniFont() const { return m_minifont; }

  void reload_skin();
  void reload_fonts();

  ui::Cursor* getCursor(ui::CursorType type);
  void initWidget(ui::Widget* widget);
  void getWindowMask(ui::Widget* widget, gfx::Region& region);
  void mapDecorativeWidget(ui::Widget* widget);

  void paintDesktop(ui::PaintEvent& ev);
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
  void drawSplitter(ui::PaintEvent& ev);
  void paintRadioButton(ui::PaintEvent& ev);
  void draw_separator(ui::Widget* widget, ui::JRect clip);
  void paintSlider(ui::PaintEvent& ev);
  void draw_combobox_entry(ui::Entry* widget, ui::JRect clip);
  void paintComboBoxButton(ui::PaintEvent& ev);
  void draw_textbox(ui::Widget* widget, ui::JRect clip);
  void paintView(ui::PaintEvent& ev);
  void paintViewScrollbar(ui::PaintEvent& ev);
  void paintViewViewport(ui::PaintEvent& ev);
  void paintWindow(ui::PaintEvent& ev);
  void paintPopupWindow(ui::PaintEvent& ev);
  void drawWindowButton(ui::ButtonBase* widget, ui::JRect clip);
  void paintTooltip(ui::PaintEvent& ev);

  int get_button_selected_offset() const { return 0; } // TODO Configurable in xml

  BITMAP* get_part(int part_i) const { return m_part[part_i]; }
  BITMAP* get_toolicon(const char* tool_id) const;

  // helper functions to draw bounds/hlines with sheet parts
  void draw_bounds_array(BITMAP* bmp, int x1, int y1, int x2, int y2, int parts[8]);
  void draw_bounds_nw(BITMAP* bmp, int x1, int y1, int x2, int y2, int nw, ui::Color bg = ui::ColorNone);
  void draw_bounds_nw(ui::Graphics* g, const gfx::Rect& rc, int nw, ui::Color bg = ui::ColorNone);
  void draw_bounds_nw2(ui::Graphics* g, const gfx::Rect& rc, int x_mid, int nw1, int nw2, ui::Color bg1, ui::Color bg2);
  void draw_part_as_hline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part);
  void draw_part_as_vline(BITMAP* bmp, int x1, int y1, int x2, int y2, int part);

  // Wrapper to use the new "Rect" class (x, y, w, h)
  void draw_bounds_nw(BITMAP* bmp, const gfx::Rect& rc, int nw, ui::Color bg) {
    draw_bounds_nw(bmp, rc.x, rc.y, rc.x+rc.w-1, rc.y+rc.h-1, nw, bg);
  }

  void drawProgressBar(BITMAP* bmp, int x1, int y1, int x2, int y2, float progress);

protected:
  void onRegenerate();

private:
  void draw_bounds_template(BITMAP* bmp, int x1, int y1, int x2, int y2,
                            int nw, int n, int ne, int e, int se, int s, int sw, int w);
  void draw_bounds_template(ui::Graphics* g, const gfx::Rect& rc,
                            int nw, int n, int ne, int e, int se, int s, int sw, int w);

  BITMAP* cropPartFromSheet(BITMAP* bmp, int x, int y, int w, int h);
  ui::Color getWidgetBgColor(ui::Widget* widget);
  void draw_textstring(const char *t, ui::Color fg_color, ui::Color bg_color,
                       bool fill_bg, ui::Widget* widget, const ui::JRect rect,
                       int selected_offset);
  void draw_textstring(ui::Graphics* g, const char *t, ui::Color fg_color, ui::Color bg_color,
                       bool fill_bg, ui::Widget* widget, const gfx::Rect& rc,
                       int selected_offset);
  void draw_entry_caret(ui::Entry* widget, int x, int y);

  void paintIcon(ui::Widget* widget, ui::Graphics* g, ui::IButtonIcon* iconInterface, int x, int y);

  static FONT* loadFont(const char* userFont, const std::string& path);

  std::string m_selected_skin;
  BITMAP* m_sheet_bmp;
  BITMAP* m_part[PARTS];
  std::map<std::string, BITMAP*> m_toolicon;
  std::vector<ui::Cursor*> m_cursors;
  std::vector<ui::Color> m_colors;
  FONT* m_minifont;
};

#endif
