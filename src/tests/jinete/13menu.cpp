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

static JWidget new_menuitem(const char *text, const char *accel_string);
static bool hooked_menuitem_msg_proc(JWidget widget, JMessage msg);

int main(int argc, char *argv[])
{
  JWidget manager, window, box1, button;
  JWidget menu;
  JWidget menubar;
  JWidget menuitem_file;
  JWidget menuitem_edit;
  JWidget menuitem_tool;
  JWidget menuitem_help;
  JWidget menu_file;
  JWidget menuitem_file_new;
  JWidget menuitem_file_open;
  JWidget menuitem_file_save;
  JWidget menuitem_file_sep1;
  JWidget menuitem_file_quit;
  JWidget menu_filenew;
  JWidget menuitem_filenew_sprite;
  JWidget menuitem_filenew_image;
  JWidget menuitem_filenew_font;
  JWidget menuitem_filenew_palette;
  JWidget menu_edit;
  JWidget menuitem_edit_cut;
  JWidget menuitem_edit_copy;
  JWidget menuitem_edit_paste;
  JWidget menuitem_edit_clear;

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

  window = jwindow_new("Menus");
  box1 = jbox_new(JI_VERTICAL);
  button = jbutton_new("&Close");
  menu = jmenu_new();
  menubar = jmenubar_new();
  menuitem_file = jmenuitem_new("&File");
  menuitem_edit = jmenuitem_new("&Edit");
  menuitem_tool = new_menuitem("&Tools", NULL);
  menuitem_help = new_menuitem("&Help", NULL);
  menu_file = jmenu_new();
  menuitem_file_new = jmenuitem_new("&New");
  menuitem_file_open = new_menuitem("&Open", "<Ctrl+O>");
  menuitem_file_save = new_menuitem("&Save", "<Ctrl+S>"); jwidget_disable (menuitem_file_save);
  menuitem_file_sep1 = ji_separator_new(NULL, JI_HORIZONTAL);
  menuitem_file_quit = new_menuitem("&Quit", "<Ctrl+Q>");
  menu_filenew = jmenu_new();
  menuitem_filenew_sprite = new_menuitem("&Sprite", NULL);
  menuitem_filenew_image = new_menuitem("&Image", NULL);
  menuitem_filenew_font = new_menuitem("&Font", NULL);
  menuitem_filenew_palette = new_menuitem("&Palette", NULL);
  menu_edit = jmenu_new ();
  menuitem_edit_cut = new_menuitem("Cu&t", "<Shift+Del>");
  menuitem_edit_copy = new_menuitem("&Copy", "<Ctrl+Ins>");
  menuitem_edit_paste = new_menuitem("&Paste", "<Shift+Ins>");
  menuitem_edit_clear = new_menuitem("C&lear", "<Ctrl+Del>");

  jwidget_add_child(menu_file, menuitem_file_new);
  jwidget_add_child(menu_file, menuitem_file_open);
  jwidget_add_child(menu_file, menuitem_file_save);
  jwidget_add_child(menu_file, menuitem_file_sep1);
  jwidget_add_child(menu_file, menuitem_file_quit);
  jmenuitem_set_submenu(menuitem_file, menu_file);

  jwidget_add_child(menu_filenew, menuitem_filenew_sprite);
  jwidget_add_child(menu_filenew, menuitem_filenew_image);
  jwidget_add_child(menu_filenew, menuitem_filenew_font);
  jwidget_add_child(menu_filenew, menuitem_filenew_palette);
  jmenuitem_set_submenu(menuitem_file_new, menu_filenew);

  jwidget_add_child(menu_edit, menuitem_edit_cut);
  jwidget_add_child(menu_edit, menuitem_edit_copy);
  jwidget_add_child(menu_edit, menuitem_edit_paste);
  jwidget_add_child(menu_edit, menuitem_edit_clear);
  jmenuitem_set_submenu(menuitem_edit, menu_edit);

  jwidget_add_child(menu, menuitem_file);
  jwidget_add_child(menu, menuitem_edit);
  jwidget_add_child(menu, menuitem_tool);
  jwidget_add_child(menu, menuitem_help);
  jmenubar_set_menu(menubar, menu);

  jwidget_add_child(box1, menubar);
  jwidget_add_child(box1, button);
  jwidget_add_child(window, box1);

  window->open_window_bg();

  jmanager_run(manager);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();

static JWidget new_menuitem(const char *text, const char *accel_string)
{
  JWidget menuitem = jmenuitem_new(text);
  JAccel accel = jaccel_new();

  jwidget_add_hook(menuitem, JI_WIDGET,
		   hooked_menuitem_msg_proc, NULL);

  if (accel_string)
    jaccel_add_keys_from_string(accel, accel_string);

  jmenuitem_set_accel(menuitem, accel);

  return menuitem;
}

static bool hooked_menuitem_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL &&
      msg->signal.num == JI_SIGNAL_MENUITEM_SELECT)
    printf("Selected item: %s\n", widget->text);
  return false;
}

