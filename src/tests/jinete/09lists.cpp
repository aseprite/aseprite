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

JRect test1();
void test2(JRect pos);
void test3(JRect pos);
void test4(JRect pos);

int main(int argc, char *argv[])
{
  JWidget manager;
  JRect pos;

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

  pos = test1();
  test2 (pos);
  test3 (pos);
  test4 (pos);

  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();

JRect test1()
{
  JWidget window, box, listbox, button;
  char buf[256];
  JRect pos;
  int c;

  window = jwindow_new("Without Viewport");
  box = jbox_new(JI_VERTICAL);
  listbox = jlistbox_new();
  button = jbutton_new("&Close");

  for (c=0; c<8; c++) {
    usprintf(buf, "Long item (%d)", c);
    jwidget_add_child (listbox, jlistitem_new (buf));
  }

  jlistbox_select_index(listbox, 0);

  jwidget_expansive(listbox, true);

  jwidget_add_child (window, box);
  jwidget_add_child (box, listbox);
  jwidget_add_child (box, button);

  window->center_window();
  pos = jwidget_get_rect(window);

  window->open_window_fg();
  jwidget_free(window);

  return pos;
}

void test2(JRect pos)
{
  JWidget window, box, view, listbox, button;
  char buf[256];
  int c;

  window = jwindow_new("With Viewport");
  box = jbox_new(JI_VERTICAL);
  view = jview_new();
  listbox = jlistbox_new();
  button = jbutton_new("&Close");

  for (c=0; c<8; c++) {
    usprintf(buf, "Long item (%d)", c);
    jwidget_add_child (listbox, jlistitem_new (buf));
  }

  jlistbox_select_index(listbox, 0);

  jview_attach(view, listbox);

  jwidget_expansive(view, true);

  jwidget_add_child (window, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  window->remap_window();
  jwidget_set_rect(window, pos);

  window->open_window_fg();
  jwidget_free(window);
}

void test3(JRect pos)
{
  JWidget window, box, view, listbox, button;
  char buf[256];
  int c;

  window = jwindow_new("Big List");
  box = jbox_new(JI_VERTICAL);
  view = jview_new();
  listbox = jlistbox_new();
  button = jbutton_new("&Close");

  for (c=0; c<512; c++) {
    usprintf(buf, "Long item (%d)", c);
    jwidget_add_child (listbox, jlistitem_new (buf));
  }

  jlistbox_select_index(listbox, 0);

  jview_attach(view, listbox);

  jwidget_expansive(view, true);

  jwidget_add_child (window, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  window->remap_window();
  jwidget_set_rect(window, pos);

  window->open_window_fg();
  jwidget_free(window);
}

void test4(JRect pos)
{
  JWidget window, box, view, widget, listbox, button;
  char buf[256];
  int c;

  window = jwindow_new("Magic List");
  box = jbox_new(JI_VERTICAL);
  view = jview_new();
  listbox = jlistbox_new();
  button = jbutton_new("&Close");

  for (c=0; c<8; c++) {
    usprintf(buf, "Long item (%d)", c);
    widget = jlistitem_new(NULL);
    jwidget_add_child (widget, jcheck_new (buf));
    jwidget_add_child (listbox, widget);
  }

  jlistbox_select_index(listbox, 0);

  jview_attach(view, listbox);

  jwidget_expansive(view, true);

  jwidget_add_child (window, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  window->remap_window();
  jwidget_set_rect(window, pos);

  window->open_window_fg();
  jwidget_free(window);
}
