/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JTHEME_H_INCLUDED
#define JINETE_JTHEME_H_INCLUDED

#include "jinete/jbase.h"

struct FONT;
struct BITMAP;

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
  virtual void draw_button(JWidget widget, JRect clip) = 0;
  virtual void draw_check(JWidget widget, JRect clip) = 0;
  virtual void draw_entry(JWidget widget, JRect clip) = 0;
  virtual void draw_grid(JWidget widget, JRect clip) = 0;
  virtual void draw_label(JWidget widget, JRect clip) = 0;
  virtual void draw_link_label(JWidget widget, JRect clip) = 0;
  virtual void draw_listbox(JWidget widget, JRect clip) = 0;
  virtual void draw_listitem(JWidget widget, JRect clip) = 0;
  virtual void draw_menu(JWidget widget, JRect clip) = 0;
  virtual void draw_menuitem(JWidget widget, JRect clip) = 0;
  virtual void draw_panel(JWidget widget, JRect clip) = 0;
  virtual void draw_radio(JWidget widget, JRect clip) = 0;
  virtual void draw_separator(JWidget widget, JRect clip) = 0;
  virtual void draw_slider(JWidget widget, JRect clip) = 0;
  virtual void draw_combobox_entry(JWidget widget, JRect clip) = 0;
  virtual void draw_combobox_button(JWidget widget, JRect clip) = 0;
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
