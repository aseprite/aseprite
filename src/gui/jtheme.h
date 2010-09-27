// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JTHEME_H_INCLUDED
#define GUI_JTHEME_H_INCLUDED

#include "gui/jbase.h"

struct FONT;
struct BITMAP;
class ButtonBase;

class jtheme
{
public:
  const char* name;
  struct FONT* default_font;
  int desktop_color;
  int textbox_fg_color;
  int textbox_bg_color;
  int check_icon_size;
  int radio_icon_size;
  int scrollbar_size;
  int guiscale;

  jtheme();
  virtual ~jtheme();

  virtual void regen() = 0;
  virtual BITMAP* set_cursor(int type, int *focus_x, int *focus_y) = 0;
  virtual void init_widget(JWidget widget) = 0;
  virtual JRegion get_window_mask(JWidget widget) = 0;
  virtual void map_decorative_widget(JWidget widget) = 0;

  virtual int color_foreground() = 0;
  virtual int color_disabled() = 0;
  virtual int color_face() = 0;
  virtual int color_hotface() = 0;
  virtual int color_selected() = 0;
  virtual int color_background() = 0;

  virtual void draw_box(JWidget widget, JRect clip) = 0;
  virtual void draw_button(ButtonBase* widget, JRect clip) = 0;
  virtual void draw_check(ButtonBase* widget, JRect clip) = 0;
  virtual void draw_entry(JWidget widget, JRect clip) = 0;
  virtual void draw_grid(JWidget widget, JRect clip) = 0;
  virtual void draw_label(JWidget widget, JRect clip) = 0;
  virtual void draw_link_label(JWidget widget, JRect clip) = 0;
  virtual void draw_listbox(JWidget widget, JRect clip) = 0;
  virtual void draw_listitem(JWidget widget, JRect clip) = 0;
  virtual void draw_menu(JWidget widget, JRect clip) = 0;
  virtual void draw_menuitem(JWidget widget, JRect clip) = 0;
  virtual void draw_panel(JWidget widget, JRect clip) = 0;
  virtual void draw_radio(ButtonBase* widget, JRect clip) = 0;
  virtual void draw_separator(JWidget widget, JRect clip) = 0;
  virtual void draw_slider(JWidget widget, JRect clip) = 0;
  virtual void draw_combobox_entry(JWidget widget, JRect clip) = 0;
  virtual void draw_combobox_button(ButtonBase* widget, JRect clip) = 0;
  virtual void draw_textbox(JWidget widget, JRect clip) = 0;
  virtual void draw_view(JWidget widget, JRect clip) = 0;
  virtual void draw_view_scrollbar(JWidget widget, JRect clip) = 0;
  virtual void draw_view_viewport(JWidget widget, JRect clip) = 0;
  virtual void draw_frame(Frame* frame, JRect clip) = 0;

};

JTheme jtheme_new_standard();

void ji_set_theme(JTheme theme);
void ji_set_standard_theme();
JTheme ji_get_theme();
void ji_regen_theme();

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
  return ji_get_theme() ? ji_get_theme()->guiscale: 1;
}

#endif
