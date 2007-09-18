/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_THEME_H
#define JINETE_THEME_H

#include "jinete/base.h"

JI_BEGIN_DECLS

struct FONT;

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
  void (*destroy)(void);
  void (*regen)(void);
  void (*set_cursor)(int type);
  void (*init_widget)(JWidget widget);
  JRegion (*get_window_mask)(JWidget widget);
  void (*map_decorative_widget)(JWidget widget);
  int nmethods;
  JDrawFunc *methods;
};

JTheme jtheme_new(void);
JTheme jtheme_new_standard(void);
JTheme jtheme_new_simple(void);
void jtheme_free(JTheme theme);

void jtheme_set_method(JTheme theme, int widget_type, JDrawFunc draw_widget);
JDrawFunc jtheme_get_method(JTheme theme, int widget_type);

/* current theme handle */

void ji_set_theme(JTheme theme);
void ji_set_standard_theme(void);
JTheme ji_get_theme(void);
void ji_regen_theme(void);

JI_END_DECLS

#endif /* JINETE_THEME_H */
