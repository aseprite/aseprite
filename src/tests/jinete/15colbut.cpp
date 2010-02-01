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
#include <stdio.h>

#include "jinete/jinete.h"

static JList windows;

static JWidget my_button_new(const char *text, int color);
static int my_button_get_color(JWidget widget);
static bool my_button_msg_proc(JWidget widget, JMessage msg);

static void new_palette_window(JWidget widget);

static bool hooked_window1_msg_proc(JWidget widget, JMessage msg);
static bool hooked_window_bg_msg_proc(JWidget widget, JMessage msg);

int main (int argc, char *argv[])
{
  JWidget manager, window1, box1, button1, button2, button3, button4, button5;

  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  manager = jmanager_new();
  ji_set_standard_theme();

  windows = jlist_new();
  window1 = jwindow_new("Colors");
  box1 = jbox_new(JI_VERTICAL | JI_HOMOGENEOUS);
  button1 = my_button_new("&Red", makecol (255, 0, 0));
  button2 = my_button_new("&Green", makecol (0, 128, 0));
  button3 = my_button_new("&Blue", makecol (0, 0, 255));
  button4 = jbutton_new("&Palette");
  button5 = jbutton_new("&Close");

  jbutton_set_bevel(button4, 4, 4, 0, 0);
  jbutton_set_bevel(button5, 0, 0, 4, 4);

  jwidget_add_hook(window1, JI_WIDGET, hooked_window1_msg_proc, NULL);
  jbutton_add_command(button4, new_palette_window);

  jwidget_add_child (box1, button1);
  jwidget_add_child (box1, button2);
  jwidget_add_child (box1, button3);
  jwidget_add_child (box1, button4);
  jwidget_add_child (box1, button5);
  jwidget_add_child (window1, box1);

  window1->open_window_bg();

  jmanager_run(manager);
  jlist_free(windows);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();

static JWidget my_button_new(const char *text, int color)
{
  JWidget widget = jbutton_new(text);

  widget->user_data[0] = (void *)color;

  jwidget_add_hook(widget, JI_WIDGET, my_button_msg_proc, NULL);
  widget->set_align(JI_LEFT | JI_BOTTOM);

  return widget;
}

static int my_button_get_color(JWidget widget)
{
  return (int)widget->user_data[0];
}

static bool my_button_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_DRAW) {
    JRect rect = jwidget_get_rect (widget);
    int color = my_button_get_color (widget);
    int bg, c1, c2;

    /* with mouse */
    if (jwidget_has_mouse (widget))
      bg = JI_COLOR_SHADE (color, 64, 64, 64);
    /* without mouse */
    else
      bg = color;

    /* selected */
    if (jwidget_is_selected (widget)) {
      c1 = JI_COLOR_SHADE (color, -128, -128, -128);
      c2 = JI_COLOR_SHADE (color, 128, 128, 128);
    }
    /* non-selected */
    else {
      c1 = JI_COLOR_SHADE (color, 128, 128, 128);
      c2 = JI_COLOR_SHADE (color, -128, -128, -128);
    }

    /* 1st border */
    if (jwidget_has_focus (widget))
      jdraw_rect (rect, makecol (0, 0, 0));
    else
      jdraw_rectedge (rect, c1, c2);

    /* 2nd border */
    jrect_shrink (rect, 1);
    if (jwidget_has_focus (widget))
      jdraw_rectedge (rect, c1, c2);
    else
      jdraw_rect (rect, bg);

    /* background */
    jrect_shrink (rect, 1);
    jdraw_rectfill (rect, bg);

    /* text */
    jdraw_widget_text (widget, makecol (255, 255, 255), bg, false);

    jrect_free (rect);
    return true;
  }
  /* button select signal */
  else if (msg->type == JM_SIGNAL &&
	   msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
    int color = my_button_get_color(widget);
    JWidget alert = jalert_new("#%02x%02x%02x"
			       "<<Red:   %d"
			       "<<Green: %d"
			       "<<Blue:  %d"
			       "||&Close",
			       getr(color), getg(color), getb(color),
			       getr(color), getg(color), getb(color));

    jlist_append(windows, alert);
    jwidget_add_hook(alert, JI_WIDGET, hooked_window_bg_msg_proc, NULL);
    alert->open_window_bg();

    /* return true to avoid close the window */
    return true;
  }
  else
    return false;
}

static void new_palette_window(JWidget widget)
{
  JWidget window = jwindow_new ("Palette");
  JWidget box = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  JWidget button;
  int c, color;

  for (c=0; c<8; c++) {
    color = makecol (rand () % 255, rand () % 255, rand () % 255);
    button = my_button_new ("", color);
    jwidget_add_child (box, button);
  }
  jwidget_add_child (window, box);

  jlist_append(windows, window);
  jwidget_add_hook(window, JI_WIDGET, hooked_window_bg_msg_proc, NULL);
  window->open_window_bg();
}

static bool hooked_window1_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_CLOSE) {
    JLink link, next;
    JI_LIST_FOR_EACH_SAFE(windows, link, next) /* close all windows */
      reinterpret_cast<Window*>(link->data)->close_window(NULL);
  }
  return false;
}

static bool hooked_window_bg_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_CLOSE)
    jlist_remove(windows, widget);
  return false;
}
