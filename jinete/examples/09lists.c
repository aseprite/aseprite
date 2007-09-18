/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

JRect test1(void);
void test2(JRect pos);
void test3(JRect pos);
void test4(JRect pos);

int main(int argc, char *argv[])
{
  JWidget manager;
  JRect pos;

  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message("%s\n", allegro_error);
    return 1;
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

JRect test1(void)
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

  jwidget_expansive(listbox, TRUE);

  jwidget_add_child (window, box);
  jwidget_add_child (box, listbox);
  jwidget_add_child (box, button);

  jwindow_center(window);
  pos = jwidget_get_rect(window);

  jwindow_open_fg(window);
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

  jwidget_expansive(view, TRUE);

  jwidget_add_child (window, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  jwindow_remap(window);
  jwidget_set_rect(window, pos);

  jwindow_open_fg(window);
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

  jwidget_expansive(view, TRUE);

  jwidget_add_child (window, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  jwindow_remap(window);
  jwidget_set_rect(window, pos);

  jwindow_open_fg(window);
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

  jwidget_expansive(view, TRUE);

  jwidget_add_child (window, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  jwindow_remap(window);
  jwidget_set_rect(window, pos);

  jwindow_open_fg(window);
  jwidget_free(window);
}
