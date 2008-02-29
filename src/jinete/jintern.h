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

#ifndef JINETE_LOW_H
#define JINETE_LOW_H

#include "jinete/jbase.h"

JI_BEGIN_DECLS

struct FONT;
struct BITMAP;

/**********************************************************************/
/* jintern.c */

JWidget _ji_get_widget_by_id(JID widget_id);
JWidget *_ji_get_widget_array(int *nwidgets);
JWidget _ji_get_new_widget(void);

void _ji_free_widget(JWidget widget);
void _ji_free_all_widgets(void);

bool _ji_is_valid_widget(JWidget widget);

void _ji_set_font_of_all_widgets(struct FONT *f);

/**********************************************************************/
/* jsystem.c */

int _ji_system_init(void);
void _ji_system_exit(void);

/**********************************************************************/
/* jfont.c */

int _ji_font_init(void);
void _ji_font_exit(void);

/**********************************************************************/
/* jwidget.c */

void _jwidget_add_hook(JWidget widget, JHook hook);
void _jwidget_remove_hook(JWidget widget, JHook hook);

/**********************************************************************/
/* jwindow.c */

bool _jwindow_is_moving(void);

/**********************************************************************/
/* jmanager.c */

void _jmanager_open_window(JWidget manager, JWidget window);
void _jmanager_close_window(JWidget manager, JWidget window, bool redraw_background);

/**********************************************************************/
/* jtheme.c */

int _ji_theme_init(void);
void _ji_theme_exit(void);

void _ji_theme_draw_sprite_color(struct BITMAP *bmp, struct BITMAP *sprite,
				 int x, int y, int color);

void _ji_theme_textbox_draw(struct BITMAP *bmp, JWidget textbox,
			    int *w, int *h, int bg, int fg);

/**********************************************************************/
/* jfontbmp.c */

struct FONT *_ji_bitmap2font(struct BITMAP *bmp);

JI_END_DECLS

#endif /* JINETE_LOW_H */
