// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_THEME_H_INCLUDED
#define GUI_THEME_H_INCLUDED

#include "gui/base.h"

struct BITMAP;
struct FONT;

namespace ui {

  class ButtonBase;
  class Entry;
  class Menu;
  class MenuItem;
  class PaintEvent;

  class Theme
  {
  public:
    const char* name;
    struct FONT* default_font;
    int desktop_color;
    int textbox_fg_color;
    int textbox_bg_color;
    int scrollbar_size;
    int guiscale;

    Theme();
    virtual ~Theme();

    void regenerate();

    virtual BITMAP* set_cursor(int type, int *focus_x, int *focus_y) = 0;
    virtual void init_widget(Widget* widget) = 0;
    virtual ui::JRegion get_window_mask(ui::Widget* widget) = 0;
    virtual void map_decorative_widget(ui::Widget* widget) = 0;

    virtual int color_foreground() = 0;
    virtual int color_disabled() = 0;
    virtual int color_face() = 0;
    virtual int color_hotface() = 0;
    virtual int color_selected() = 0;
    virtual int color_background() = 0;

    virtual void paintBox(PaintEvent& ev) = 0;
    virtual void paintButton(PaintEvent& ev) = 0;
    virtual void paintCheckBox(PaintEvent& ev) = 0;
    virtual void paintEntry(PaintEvent& ev) = 0;
    virtual void paintGrid(PaintEvent& ev) = 0;
    virtual void paintLabel(PaintEvent& ev) = 0;
    virtual void paintLinkLabel(PaintEvent& ev) = 0;
    virtual void draw_listbox(Widget* widget, JRect clip) = 0;
    virtual void draw_listitem(Widget* widget, JRect clip) = 0;
    virtual void draw_menu(Menu* menu, JRect clip) = 0;
    virtual void draw_menuitem(MenuItem* menuitem, JRect clip) = 0;
    virtual void draw_panel(Widget* widget, JRect clip) = 0;
    virtual void paintRadioButton(PaintEvent& ev) = 0;
    virtual void draw_separator(Widget* widget, JRect clip) = 0;
    virtual void paintSlider(PaintEvent& ev) = 0;
    virtual void draw_combobox_entry(Entry* widget, JRect clip) = 0;
    virtual void paintComboBoxButton(PaintEvent& ev) = 0;
    virtual void draw_textbox(Widget* widget, JRect clip) = 0;
    virtual void draw_view(Widget* widget, JRect clip) = 0;
    virtual void draw_view_scrollbar(Widget* widget, JRect clip) = 0;
    virtual void draw_view_viewport(Widget* widget, JRect clip) = 0;
    virtual void paintFrame(PaintEvent& ev) = 0;
    virtual void paintTooltip(PaintEvent& ev) = 0;

  protected:
    virtual void onRegenerate() = 0;

  };

  namespace CurrentTheme
  {
    void set(Theme* theme);
    Theme* get();
  }

  int ji_color_foreground();
  int ji_color_disabled();
  int ji_color_face();
  int ji_color_facelight();
  int ji_color_faceshadow();
  int ji_color_hotface();
  int ji_color_selected();
  int ji_color_background();

  BITMAP* ji_apply_guiscale(BITMAP* original);

  // This value is a factor to multiply every screen size/coordinate.
  // Every icon/graphics/font should be scaled to this factor.
  inline int jguiscale()
  {
    return CurrentTheme::get() ? CurrentTheme::get()->guiscale: 1;
  }

} // namespace ui

#endif
