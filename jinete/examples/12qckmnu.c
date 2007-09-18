/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>
#include <stdio.h>

#include "jinete.h"

static void handle_menuitem_selected (JWidget widget, int user_data);

static struct jquickmenu quickmenu[] =
{
  {  0, "&File",	NULL,		NULL,			  0 },
  {  1, "&New",		NULL,		NULL,			  0 },
  {  2, "&Sprite",	NULL,		handle_menuitem_selected, 0 },
  {  2, "&Image",	NULL,		handle_menuitem_selected, 0 },
  {  2, "&Font",	NULL,		handle_menuitem_selected, 0 },
  {  2, "&Palette",	NULL,		handle_menuitem_selected, 0 },
  {  1, "&Open",	"<Ctrl+O>",	handle_menuitem_selected, 0 },
  {  1, "&Save",	"<Ctrl+S>",	handle_menuitem_selected, 0 },
  {  1, NULL,		NULL,		NULL,			  0 },
  {  1, "&Quit",	"<Ctrl+Q> <Alt+X> <Esc>", handle_menuitem_selected, 0 },
  {  0, "&Edit",	NULL,		NULL,			  0 },
  {  1, "Cu&t",		"<Shift+Del>",	handle_menuitem_selected, 0 },
  {  1, "&Copy",	"<Ctrl+Ins>",	handle_menuitem_selected, 0 },
  {  1, "&Paste",	"<Shift+Ins>",	handle_menuitem_selected, 0 },
  {  1, "C&lear",	"<Ctrl+Del>",	handle_menuitem_selected, 0 },
  {  0, "&Tools",	NULL,		handle_menuitem_selected, 0 },
  {  0, "&Help",	NULL,		handle_menuitem_selected, 0 },
  { -1, NULL,		NULL,		NULL,			  0 },
};

int main (int argc, char *argv[])
{
  JWidget manager, window, box, button, menubar;

  allegro_init ();
  if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  manager = jmanager_new ();
  ji_set_standard_theme ();

  window = jwindow_new ("Menus");
  box = jbox_new (JI_VERTICAL);
  button = jbutton_new ("&Close");
  menubar = jmenubar_new_quickmenu (quickmenu);

  jwidget_add_child (window, box);
  jwidget_add_child (box, menubar);
  jwidget_add_child (box, button);

  jwindow_open_bg (window);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN ();

static void handle_menuitem_selected (JWidget widget, int user_data)
{
  printf ("Selected item: %s\n", widget->text);
}

