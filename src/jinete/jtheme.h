/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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
 *   * Neither the name of the Jinete nor the names of its contributors may
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

#ifndef JINETE_THEME_H
#define JINETE_THEME_H

#include "jinete/jbase.h"

struct FONT;
struct BITMAP;

struct jtheme
{
  const char *name;
  struct FONT *default_font;
  int desktop_color;
  int textbox_fg_color;
  int textbox_bg_color;
  int check_icon_size;
  int radio_icon_size;
  int scrollbar_size;
  void (*destroy)();
  void (*regen)();
  struct BITMAP *(*set_cursor)(int type, int *focus_x, int *focus_y);
  void (*init_widget)(JWidget widget);
  JRegion (*get_window_mask)(JWidget widget);
  void (*map_decorative_widget)(JWidget widget);
  int (*color_foreground)();
  int (*color_disabled)();
  int (*color_face)();
  int (*color_hotface)();
  int (*color_selected)();
  int (*color_background)();
  int nmethods;
  JDrawFunc *methods;
};

JTheme jtheme_new();
JTheme jtheme_new_standard();
JTheme jtheme_new_simple();
void jtheme_free(JTheme theme);

void jtheme_set_method(JTheme theme, int widget_type, JDrawFunc draw_widget);
JDrawFunc jtheme_get_method(JTheme theme, int widget_type);

/* current theme handle */

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

#endif /* JINETE_THEME_H */
