/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_LOW_H
#define JINETE_LOW_H

#include "jinete/base.h"

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
void _jmanager_close_window(JWidget manager, JWidget window,
			    bool sendtokill, bool redraw_background);

/**********************************************************************/
/* jtheme.c */

int _ji_theme_init(void);
void _ji_theme_exit(void);

void _ji_theme_draw_sprite_color(struct BITMAP *bmp, struct BITMAP *sprite,
				 int x, int y, int color);

void _ji_theme_rectedge(struct BITMAP *bmp,
			int x1, int y1, int x2, int y2, int c1, int c2);

void _ji_theme_rectfill_exclude(struct BITMAP *bmp,
				int x1, int y1, int x2, int y2,
				int ex1, int ey1, int ex2, int ey2, int color);

void _ji_theme_textbox_draw(struct BITMAP *bmp, JWidget textbox, int *w, int *h);

/**********************************************************************/
/* jfontbmp.c */

struct FONT *_ji_bitmap2font(struct BITMAP *bmp);

JI_END_DECLS

#endif /* JINETE_LOW_H */
