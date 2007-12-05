/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <errno.h>
#if defined ALLEGRO_UNIX || defined ALLEGRO_DJGPP || defined ALLEGRO_MINGW32
#  include <sys/stat.h>
#endif
#if defined ALLEGRO_UNIX || defined ALLEGRO_MINGW32
#  include <sys/unistd.h>
#endif

#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#include <shlobj.h>
#endif

#include "jinete/jinete.h"

#include "core/cfg.h"
#include "core/dirs.h"
#include "modules/gfx.h"
#include "modules/gui.h"

#endif

#if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
#  define HAVE_DRIVES
#endif

#define FA_ALL       FA_RDONLY | FA_DIREC | FA_ARCH | FA_HIDDEN | FA_SYSTEM

static bool combobox_msg_proc(JWidget widget, JMessage message);
static void add_bookmark_command(JWidget widget, void *data);
static void del_bookmark_command(JWidget widget, void *data);
static void fill_bookmarks_combobox(JWidget combobox);
static void home_command(JWidget widget);
static void fonts_command(JWidget widget);
static void palettes_command(JWidget widget);
static void mkdir_command(JWidget widget);

/**
 * The routine to select file in ASE.
 *
 * It add some extra functionalities to the default Jinete
 * file-selection dialog.
 * 
 * @see ji_file_select_ex.
 */
char *GUI_FileSelect(const char *message,
		     const char *init_path,
		     const char *exts)
{
  JWidget box_left, button_home;
  JWidget button_fonts, button_palettes, button_mkdir;
  JWidget box_top, box_top2, combobox, add_bookmark, del_bookmark, entry_path;
  JWidget widget_extension;
  char buf[512], *selected_filename;

  ustrcpy(buf, init_path);

  /* insert the path */
  if (get_filename(buf) == buf) {
    char path[512];

    /* get saved `path' */
    ustrcpy(path, get_config_string("FileSelect", "CurrentDirectory", ""));

    /* if the `path' doesn't exist */
    if ((!ugetat(path, 0)) || (!ji_dir_exists(path))) {
      /* try to get current `path' */
#ifdef HAVE_DRIVES
      int drive = _al_getdrive();
#else
      int drive = 0;
#endif

      _al_getdcwd(drive, path, sizeof(path) - ucwidth(OTHER_PATH_SEPARATOR));
    }

    fix_filename_case(path);
    fix_filename_slashes(path);
    put_backslash(path);

    ustrcat(path, buf);
    ustrcpy(buf, path);
  }

  /**********************************************************************/
  /* prepare left side */

  box_left = jbox_new(JI_VERTICAL);
  button_home = jbutton_new(NULL);
  button_fonts = jbutton_new(NULL);
  button_palettes = jbutton_new(NULL);
  button_mkdir = jbutton_new(NULL);

  add_gfxicon_to_button(button_home, GFX_FILE_HOME, JI_CENTER | JI_MIDDLE);
  add_gfxicon_to_button(button_fonts, GFX_FILE_FONTS, JI_CENTER | JI_MIDDLE);
  add_gfxicon_to_button(button_palettes, GFX_FILE_PALETTES, JI_CENTER | JI_MIDDLE);
  add_gfxicon_to_button(button_mkdir, GFX_FILE_MKDIR, JI_CENTER | JI_MIDDLE);

  /* hook signals */
  jbutton_add_command(button_home, home_command);
  jbutton_add_command(button_fonts, fonts_command);
  jbutton_add_command(button_palettes, palettes_command);
  jbutton_add_command(button_mkdir, mkdir_command);

  jwidget_add_childs(box_left,
		     button_home, button_fonts,
		     button_palettes, button_mkdir, NULL);

  /**********************************************************************/
  /* prepare top side */

  box_top = jbox_new(JI_HORIZONTAL);
  box_top2 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  combobox = jcombobox_new();
  entry_path = jcombobox_get_entry_widget(combobox);
  add_bookmark = jbutton_new("+");
  del_bookmark = jbutton_new("-");

#ifdef HAVE_DRIVES
  jcombobox_casesensitive(combobox, FALSE);
#else
  jcombobox_casesensitive(combobox, TRUE);
#endif

  jwidget_noborders(box_top2);
  jbutton_set_bevel(add_bookmark, 2, 0, 2, 0);
  jbutton_set_bevel(del_bookmark, 0, 2, 0, 2);
  jcombobox_editable(combobox, TRUE);
  jcombobox_clickopen(combobox, FALSE);

  jwidget_add_hook(combobox, JI_WIDGET, combobox_msg_proc, NULL);
  jbutton_add_command_data(add_bookmark, add_bookmark_command, combobox);
  jbutton_add_command_data(del_bookmark, del_bookmark_command, combobox);

  fill_bookmarks_combobox(combobox);

  jwidget_expansive(combobox, TRUE);
  jwidget_add_childs(box_top, combobox, box_top2, NULL);
  jwidget_add_childs(box_top2, add_bookmark, del_bookmark, NULL);

  /**********************************************************************/
  /* prepare widget_extension */

  widget_extension = jwidget_new(JI_WIDGET);
  jwidget_set_name(box_left, "left");
  jwidget_set_name(box_top, "top");
  jwidget_set_name(entry_path, "path");
  jwidget_add_childs(widget_extension, box_left, box_top, NULL);

  /* call the jinete file selector */
  selected_filename = ji_file_select_ex(message, buf, exts, widget_extension);
  if (selected_filename) {
    char *s, *name_dup = jstrdup(selected_filename);

    /* save the path in the configuration file */
    s = get_filename(name_dup);
    if (s)
      *s = 0;

    set_config_string("FileSelect", "CurrentDirectory", name_dup);
    jfree(name_dup);
  }

  jwidget_free(widget_extension);

  return selected_filename;
}

static bool combobox_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_COMBOBOX_SELECT) {
	ji_file_select_enter_to_path(jcombobox_get_selected_string(widget));
	return TRUE;
      }
      break;
  }

  return FALSE;
}

/**
 * Adds a new bookmark.
 */
static void add_bookmark_command(JWidget widget, void *data)
{
  JWidget combobox = data;
  char buf[64], path[1024];
  int count;

  count = get_config_int ("Bookmarks", "Count", 0);
  count = MID (0, count, 256);

  if (count < 256) {
    replace_filename(path, ji_file_select_get_current_path(), "", 1024);

    jcombobox_add_string(combobox, path);

    usprintf(buf, "Mark%02d", count);

    set_config_string("Bookmarks", buf, path);
    set_config_int("Bookmarks", "Count", count+1);
  }
}

/**
 * Deletes a bookmark.
 */
static void del_bookmark_command(JWidget widget, void *data)
{
  JWidget combobox = data;
  char buf[64], path[1024];
  int index, count;

  count = jcombobox_get_count(combobox);
  if (count > 0) {
    replace_filename(path, ji_file_select_get_current_path(), "", 1024);

    index = jcombobox_get_index(combobox, path);
    if (index >= 0 && index < count) {
      jcombobox_del_index(combobox, index);

      for (; index<count; index++) {
	usprintf(buf, "Mark%02d", index);
	set_config_string("Bookmarks", buf,
			  jcombobox_get_string(combobox, index));
      }
      usprintf(buf, "Mark%02d", index);
      set_config_string("Bookmarks", buf, "");

      set_config_int("Bookmarks", "Count", count-1);
    }
  }
}

/**
 * Fills the combo-box with the existent bookmarks.
 */
static void fill_bookmarks_combobox(JWidget combobox)
{
  const char *path;
  char buf[256];
  int c, count;

  count = get_config_int("Bookmarks", "Count", 0);
  count = MID(0, count, 256);

  for (c=0; c<count; c++) {
    usprintf(buf, "Mark%02d", c);
    path = get_config_string("Bookmarks", buf, "");
    if (path && *path)
      jcombobox_add_string(combobox, path);
  }
}

/**
 * Goes to the "Home" folder.
 */
static void home_command(JWidget widget)
{
  char *env;

  /* in Windows we can use the "My Documents" folder */
#ifdef ALLEGRO_WINDOWS
  TCHAR szPath[MAX_PATH];
  if (SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, 
		      NULL, 0, szPath) == S_OK) {
    ji_file_select_enter_to_path(szPath);
    return;
  }
#endif

  /* in Unix we can use the HOME enviroment variable */
  env = getenv("HOME");

  /* home directory? */
  if ((env) && (*env)) {
    ji_file_select_enter_to_path(env);
  }
  /* ok, maybe we are in DOS, so we can use the ASE directory */
  else {
    char path[1024];

    get_executable_name (path, sizeof (path));
    *get_filename (path) = 0;

    ji_file_select_enter_to_path(path);
  }
}

/**
 * Goes to the "Fonts" directory.
 */
static void fonts_command(JWidget widget)
{
  DIRS *dir, *dirs = filename_in_datadir ("fonts/");

  for (dir=dirs; dir; dir=dir->next) {
    if (ji_dir_exists (dir->path)) {
      ji_file_select_enter_to_path (dir->path);
      break;
    }
  }

  dirs_free (dirs);
}

/**
 * Goes to the "Palettes" directory.
 */
static void palettes_command(JWidget widget)
{
  DIRS *dir, *dirs = filename_in_datadir ("palettes/");

  for (dir=dirs; dir; dir=dir->next) {
    if (ji_dir_exists (dir->path)) {
      ji_file_select_enter_to_path (dir->path);
      break;
    }
  }

  dirs_free (dirs);
}

/**
 * Shows the dialog to makes a new folder/directory.
 */
static void mkdir_command(JWidget widget)
{
  JWidget window, box1, box2, label_name, entry_name, button_create, button_cancel;

  window = jwindow_new(_("Make Directory"));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_name = jlabel_new(_("Name:"));
  entry_name = jentry_new(256, _("New Directory"));
  button_create = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  jwidget_set_static_size(entry_name, JI_SCREEN_W*75/100, 0);

  jwidget_add_child(box2, button_create);
  jwidget_add_child(box2, button_cancel);
  jwidget_add_child(box1, label_name);
  jwidget_add_child(box1, entry_name);
  jwidget_add_child(box1, box2);
  jwidget_add_child(window, box1);

  jwidget_magnetic(button_create, TRUE);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_create) {
    char buf[1024];
    int res;

    ustrcpy(buf, ji_file_select_get_current_path());
    put_backslash(buf);
    ustrcat(buf, jwidget_get_text(entry_name));

#if defined ALLEGRO_UNIX || defined ALLEGRO_DJGPP
    res = mkdir(buf, 0777);
#else
    res = mkdir(buf);
#endif

    if (res != 0)
      jalert(_("Error<<Error making the directory||&Close"));
    /* fill again the file-list */
    else
      ji_file_select_refresh_listbox();
  }

  jwidget_free(window);
}
