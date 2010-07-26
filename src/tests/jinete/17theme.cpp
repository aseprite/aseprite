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

#include <allegro.h>

#include "jinete/jinete.h"

FONT *my_font = NULL;

void set_my_font();
void set_my_palette();
JTheme my_theme_new();

int main(int argc, char *argv[])
{
  JWidget manager, window, box1, box2, box3, box4, box5, button1, button2;
  JWidget label_username, label_password;
  JWidget entry_username, entry_password;
  JTheme my_theme;

  /* Allegro stuff */
  allegro_init();
  set_color_depth(16);
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      set_color_depth(8);
      if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
	if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
	  allegro_message("%s\n", allegro_error);
	  return 1;
	}
      }
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  /* Jinete initialization */
  manager = jmanager_new();

  set_my_font();
  set_my_palette();

  /* change to custom theme */
  my_theme = my_theme_new();
  ji_set_theme(my_theme);

  window = jwindow_new("Login");
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_VERTICAL | JI_HOMOGENEOUS);
  box4 = jbox_new(JI_VERTICAL | JI_HOMOGENEOUS);
  box5 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_username = jlabel_new("Username");
  label_password = jlabel_new("Password");
  entry_username = jentry_new(32, "");
  entry_password = jentry_new(32, "");
  button1 = jbutton_new("&OK");
  button2 = jbutton_new("&Cancel");

  jentry_password(entry_password, true);
  jwidget_magnetic(entry_username, true);
  jwidget_magnetic(button1, true);

  jwidget_add_child(window, box1);
  jwidget_add_child(box1, box2);
  jwidget_add_child(box1, box5);
  jwidget_add_child(box2, box3);
  jwidget_add_child(box2, box4);
  jwidget_add_child(box3, label_username);
  jwidget_add_child(box3, label_password);
  jwidget_add_child(box4, entry_username);
  jwidget_add_child(box4, entry_password);
  jwidget_add_child(box5, button1);
  jwidget_add_child(box5, button2);

  jwindow_sizeable(window, false);

 again:;
  window->open_window_fg();

  if (window->get_killer() == button1) {
    if (*jwidget_get_text(entry_username) &&
	*jwidget_get_text(entry_password))
      jalert("Login Successful"
	     "==Welcome \"%s\""
	     "||&Done",
	     jwidget_get_text(entry_username));
    else {
      jalert("Login Failed"
	     "==Try to use some username==and password"
	     "||&OK");
      goto again;
    }
  }

  jwidget_free(window);

  jtheme_free(my_theme);
  jmanager_free(manager);

  if (my_font)
    destroy_font(my_font);

  return 0;
}

END_OF_MAIN();

void set_my_font()
{
  char buf[512], argv0[512];
  get_executable_name(argv0, sizeof(argv0));
  replace_filename(buf, argv0, "zerohour.ttf", sizeof(buf));
  my_font = ji_font_load_ttf(buf);
  if (my_font) {
    font = my_font;
    ji_font_set_size(font, 8);
  }
}

void set_my_palette()
{
  char buf[512], argv0[512];
  BITMAP *bmp;
  PALETTE pal;
  get_executable_name(argv0, sizeof(argv0));
  replace_filename(buf, argv0, "pal17.pcx", sizeof(buf));
  bmp = load_bitmap(buf, pal);
  if (bmp) {
    set_palette(pal);
    rgb_map = (RGB_MAP*)malloc(sizeof(RGB_MAP));
    create_rgb_table(rgb_map, pal, NULL);
  }
}

/**********************************************************************/
/* Theme */

static void theme_regen();
static BITMAP *theme_set_cursor(int type, int *focus_x, int *focus_y);
static void theme_init_widget(JWidget widget);
static JRegion theme_get_window_mask(JWidget widget);

static void theme_draw_box(JWidget widget, JRect clip);
static void theme_draw_button(JWidget widget, JRect clip);
static void theme_draw_entry(JWidget widget, JRect clip);
static void theme_draw_label(JWidget widget, JRect clip);
static void theme_draw_window(JWidget widget, JRect clip);

static void draw_rect(JRect rect, int color, bool invert);
static void draw_edge(JRect rect, int color, bool invert);
static void draw_entry_cursor(JWidget widget, int x, int y);

JTheme my_theme_new()
{
  JTheme theme;

  theme = jtheme_new();
  if (!theme)
    return NULL;

  theme->name = "My Theme";
  theme->check_icon_size = 8;
  theme->radio_icon_size = 8;

  theme->regen = theme_regen;
  theme->set_cursor = theme_set_cursor;
  theme->init_widget = theme_init_widget;
  theme->get_window_mask = theme_get_window_mask;

  jtheme_set_method(theme, JI_BOX, theme_draw_box);
  jtheme_set_method(theme, JI_BUTTON, theme_draw_button);
  jtheme_set_method(theme, JI_ENTRY, theme_draw_entry);
  jtheme_set_method(theme, JI_LABEL, theme_draw_label);
  jtheme_set_method(theme, JI_FRAME, theme_draw_frame);

  return theme;
}

static void theme_regen()
{
  ji_get_theme()->desktop_color = makecol(0, 128, 196);
}

static BITMAP *theme_set_cursor(int type, int *focus_x, int *focus_y)
{
  return NULL;
}

static void theme_init_widget(JWidget widget)
{
#define BORDER(n)				\
  widget->border_width.l = n;			\
  widget->border_width.t = n;			\
  widget->border_width.r = n;			\
  widget->border_width.b = n;

#define BORDER4(L,T,R,B)			\
  widget->border_width.l = L;			\
  widget->border_width.t = T;			\
  widget->border_width.r = R;			\
  widget->border_width.b = B;

  switch (widget->draw_type) {

    case JI_BOX:
      BORDER(4);
      widget->child_spacing = 2;
      break;

    case JI_BUTTON:
      BORDER(5);
      widget->child_spacing = 0;
      break;

    case JI_ENTRY:
      BORDER(6);
      widget->child_spacing = 0;
      break;

    case JI_FRAME:
      BORDER4(7, 7+jwidget_get_text_height(widget)+7, 7, 7);
      widget->child_spacing = 1;
      break;
  }
}

static JRegion theme_get_window_mask(JWidget widget)
{
  JRegion reg1 = jregion_new(NULL, 0);
  JRegion reg2 = jregion_new(NULL, 0);
  int table[2] = { 2, 1 };
  JRect rect = jrect_new(0, 0, 0, 0);
  JRect pos = jwidget_get_rect(widget);
  int c;
  bool overlap;

  for (c=0; c<2; c++) {
    jrect_replace(rect,
		  pos->x1+table[c], pos->y1+c,
		  pos->x2-table[c], pos->y1+c+1);
    jregion_reset(reg2, rect);
    jregion_append(reg1, reg2);

    jrect_replace(rect,
		  pos->x1+table[c], pos->y2-c-1,
		  pos->x2-table[c], pos->y2-c);
    jregion_reset(reg2, rect);
    jregion_append(reg1, reg2);
  }

  jrect_replace(rect, pos->x1, pos->y1+c, pos->x2, pos->y2-c);
  jregion_reset(reg2, rect);
  jregion_append(reg1, reg2);

  jrect_free(pos);
  jrect_free(rect);
  jregion_free(reg2);

  jregion_validate(reg1, &overlap);
  return reg1;
}

static void theme_draw_box(JWidget widget, JRect clip)
{
  JWidget window = jwidget_get_window(widget);
  if (window) {
    JRect pos = jwidget_get_child_rect(window);
    draw_edge(pos, makecol(255, 196, 64), false);
    jrect_shrink(pos, 1);
    draw_rect(pos, makecol(255, 196, 64), false);
    jrect_free(pos);
  }
}

static void theme_draw_button(JWidget widget, JRect clip)
{
  JRect pos = jwidget_get_rect(widget);
  int fg, bg;

  if (jwidget_is_disabled(widget)) {
    fg = makecol(196, 196, 196);
    bg = makecol(128, 128, 128);
  }
  else {
    fg = makecol(0, 0, 0);
    bg = jwidget_has_mouse(widget) ? makecol(64, 224, 255):
				     makecol(0, 196, 255);
  }

  draw_edge(pos, makecol(255, 196, 64), true);

  if (jwidget_has_focus (widget)) {
    jrect_shrink(pos, 1);
    draw_edge(pos, makecol(0, 0, 0), false);
  }

  jrect_shrink(pos, 1);
  draw_edge(pos, bg, jwidget_is_selected(widget));

  jrect_shrink(pos, 1);
  draw_edge(pos, makecol(128, 128, 128), !jwidget_is_selected(widget));

  jrect_shrink(pos, 1);
  draw_rect(pos, bg, jwidget_is_selected(widget));

  if (widget->text) {
    struct jrect text;

    jwidget_get_texticon_info(widget, NULL, &text, NULL, 0, 0, 0);

    if (jwidget_is_selected(widget))
      jrect_displace(&text, 1, 1);

    jdraw_text(ji_screen, widget->text_font, widget->text, text.x1+1, text.y1+1,
	       makecol(255, 255, 255), bg, false);
    jdraw_text(ji_screen, widget->text_font, widget->text,
	       text.x1, text.y1, fg, bg, false);
  }

  jrect_free(pos);
}

static void theme_draw_entry(JWidget widget, JRect clip)
{
  int scroll, cursor, state, selbeg, selend;
  bool password = jentry_is_password(widget);
  const char *text = widget->text;
  int c, ch, x, y, w, fg, bg;
  int cursor_x;
  JRect pos = jwidget_get_rect(widget);

  jtheme_entry_info(widget, &scroll, &cursor, &state, &selbeg, &selend);

  bg = makecol(255, 255, 255);

  /* 1st border */
  draw_edge(pos, makecol (255, 196, 64), true);

  /* 2nd border */
  jrect_shrink(pos, 1);
  if (jwidget_has_focus(widget)) {
    jdraw_rect(pos, makecol(0, 0, 0));
    jrect_shrink(pos, 1);
  }
  draw_edge(pos, makecol(192, 192, 192), false);

  /* background */
  jrect_shrink(pos, 1);
  jdraw_rectshade(pos, JI_COLOR_SHADE(bg, -64, -64, -64), bg,
		  JI_VERTICAL);

  /* draw the text */
  x = widget->rc->x1 + widget->border_width.l;
  y = (widget->rc->y1+widget->rc->y2)/2 - jwidget_get_text_height(widget)/2;

  for (c=scroll; ugetat (text, c); c++) {
    ch = password ? '*': ugetat (text, c);

    /* normal text */
    bg = -1;
    fg = makecol(0, 0, 0);

    /* selected */
    if ((c >= selbeg) && (c <= selend)) {
      if (jwidget_has_focus(widget))
	bg = makecol(44, 76, 145);
      else
	bg = makecol(128, 128, 128);
      fg = makecol(255, 255, 255);
    }

    /* disabled */
    if (jwidget_is_disabled(widget)) {
      bg = -1;
      fg = makecol(128, 128, 128);
    }

    w = ji_font_char_len(widget->text_font, ch);
    if (x+w > widget->rc->x2-3)
      return;

    cursor_x = x;
    jdraw_char(widget->text_font, ch, x, y, fg,
	       bg >= 0 ? bg: makecol (192, 192, 192), bg >= 0);
    x += w;

    /* cursor */
    if ((c == cursor) && (state) && (jwidget_has_focus(widget)))
      draw_entry_cursor(widget, cursor_x, y);
  }

  /* draw the cursor if it is next of the last character */
  if ((c == cursor) && (state) &&
      (jwidget_has_focus(widget)) &&
      (jwidget_is_enabled(widget)))
    draw_entry_cursor(widget, x, y);

  jrect_free(pos);
}

static void theme_draw_label(JWidget widget, JRect clip)
{
  /* draw background as Box */
  theme_draw_box(widget, clip);

  if (widget->text) {
    struct jrect text;

    jwidget_get_texticon_info(widget, NULL, &text, NULL, 0, 0, 0);

    jdraw_text(ji_screen, widget->text_font, widget->text, text.x1+1, text.y1+1,
	       makecol(0, 0, 0), makecol(196, 128, 0), false);
    jdraw_text(ji_screen, widget->text_font, widget->text, text.x1, text.y1,
	       makecol(255, 255, 255), makecol(196, 128, 0), false);
  }
}

static void theme_draw_window(JWidget widget, JRect clip)
{
  JRect pos;
  int c;

  pos = jwidget_get_rect(widget);

  /* borders */
  for (c=0; c<3; c++) {
    jdraw_rectedge(pos,
		   JI_COLOR_SHADE(makecol(0, 127, 255), +90, +90, +90),
		   JI_COLOR_SHADE(makecol(0, 127, 255), -90, -90, -90));
    jrect_shrink(pos, 1);
  }

  /* background */
  draw_rect(pos, makecol(0, 127, 255), false);

  /* title bar */
  pos->x1 += 8;
  pos->y1 += 6;
  jdraw_text(ji_screen, widget->text_font, widget->text, pos->x1+1, pos->y1+1,
	     makecol(0, 0, 0), makecol(0, 127, 255), false);
  jdraw_text(ji_screen, widget->text_font, widget->text, pos->x1, pos->y1,
	     makecol(255, 255, 255), makecol(0, 127, 255), false);

  /* border to client area */
  jrect_free(pos);
  pos = jwidget_get_child_rect(widget);
  jrect_stretch(pos, 1);
  jdraw_rectedge(pos,
		 JI_COLOR_SHADE(makecol(0, 127, 255), -32, -32, -32),
		 JI_COLOR_SHADE(makecol(0, 127, 255), +32, +32, +32));
  jrect_free(pos);
}

static void draw_rect(JRect rect, int color, bool invert)
{
  JRect p = jrect_new(0, 0, 0, 0);
  int c1 = JI_COLOR_SHADE(color, +32, +32, +32);
  int c2 = JI_COLOR_SHADE(color, -32, -32, -32);
  int c, h = 3;

  if (invert) {
    c = c1;
    c1 = c2;
    c2 = c;
  }

  jrect_replace(p, rect->x1, rect->y1, rect->x2, rect->y1+h);
  jdraw_rectshade(p, c1, color, JI_VERTICAL);

  jrect_replace(p, rect->x1, rect->y1+h, rect->x2, rect->y2-h);
  jdraw_rectfill(p, color);

  jrect_replace(p, rect->x1, rect->y2-h, rect->x2, rect->y2);
  jdraw_rectshade(p, color, c2, JI_VERTICAL);

  jrect_free(p);
}

static void draw_edge(JRect rect, int color, bool invert)
{
  JRect p = jrect_new(0, 0, 0, 0);
  int c1 = JI_COLOR_SHADE(color, +90, +90, +90);
  int c2 = JI_COLOR_SHADE(color, -90, -90, -90);
  int c;

  if (invert) {
    c = c1;
    c1 = c2;
    c2 = c;
  }

  jrect_replace(p, rect->x1, rect->y1, rect->x2, rect->y1+1);
  jdraw_rectshade(p, c1, color, JI_HORIZONTAL);

  jrect_moveto(p, p->x1, rect->y2-1);
  jdraw_rectshade(p, color, c2, JI_HORIZONTAL);

  jrect_replace(p, rect->x1, rect->y1, rect->x1+1, rect->y2);
  jdraw_rectshade(p, c1, color, JI_VERTICAL);

  jrect_moveto(p, rect->x2-1, p->y1);
  jdraw_rectshade(p, color, c2, JI_VERTICAL);

  jrect_free(p);
}

static void draw_entry_cursor(JWidget widget, int x, int y)
{
  int h = jwidget_get_text_height(widget);

  vline(ji_screen, x,   y-1, y+h, makecol(0, 0, 0));
  vline(ji_screen, x+1, y-1, y+h, makecol(0, 0, 0));
}
