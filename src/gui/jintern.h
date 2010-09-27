// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JINTERN_H_INCLUDED
#define GUI_JINTERN_H_INCLUDED

#include "gui/jbase.h"

struct FONT;
struct BITMAP;
class Frame;

//////////////////////////////////////////////////////////////////////
// jintern.c

JWidget _ji_get_widget_by_id(JID widget_id);
JWidget *_ji_get_widget_array(int *nwidgets);

void _ji_add_widget(JWidget widget);
void _ji_remove_widget(JWidget widget);
bool _ji_is_valid_widget(JWidget widget);

void _ji_set_font_of_all_widgets(FONT* f);
void _ji_reinit_theme_in_all_widgets();

//////////////////////////////////////////////////////////////////////
// jwindow.c

bool _jwindow_is_moving();

//////////////////////////////////////////////////////////////////////
// jmanager.c

void _jmanager_open_window(JWidget manager, Frame* frame);
void _jmanager_close_window(JWidget manager, Frame* frame, bool redraw_background);

//////////////////////////////////////////////////////////////////////
// jtheme.c

void _ji_theme_draw_sprite_color(struct BITMAP *bmp, struct BITMAP *sprite,
				 int x, int y, int color);

void _ji_theme_textbox_draw(struct BITMAP *bmp, JWidget textbox,
			    int *w, int *h, int bg, int fg);

//////////////////////////////////////////////////////////////////////
// jfontbmp.c

struct FONT *_ji_bitmap2font(struct BITMAP *bmp);

#endif
