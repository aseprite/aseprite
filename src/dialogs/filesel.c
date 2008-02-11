/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include <assert.h>
#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <errno.h>

#include "jinete/jinete.h"

#include "core/cfg.h"
#include "core/dirs.h"
#include "file/file.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "widgets/fileview.h"

#if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
#  define HAVE_DRIVES
#endif

#ifndef MAX_PATH
#  define MAX_PATH 4096		/* TODO this is needed for Linux, is it correct? */
#endif

/* Variables used only to maintain the history of navigation. */
static JLink navigation_position = NULL; /* current position in the navigation history */
static JList navigation_history = NULL;	/* set of FileItems navigated */
static bool navigation_locked = FALSE;	/* if TRUE the navigation_history isn't
					   modified if the current folder
					   changes (used when the back/forward
					   buttons are pushed) */

static void update_location(JWidget window);
static void update_navigation_buttons(JWidget window);
static void add_in_navigation_history(FileItem *folder);
static void select_filetype_from_filename(JWidget window);

static void goback_command(JWidget widget);
static void goforward_command(JWidget widget);
static void goup_command(JWidget widget);

static bool fileview_msg_proc(JWidget widget, JMessage msg);
static bool location_msg_proc(JWidget widget, JMessage msg);
static bool filetype_msg_proc(JWidget widget, JMessage msg);

/* Shows the dialog to select a file in ASE.

   Mainly it uses:
   * the 'core/file_system' routines.
   * the 'widgets/fileview' widget.
 */
char *ase_file_selector(const char *message,
			const char *init_path,
			const char *exts)
{
  static JWidget window = NULL;
  JWidget fileview, box, ok;
  JWidget goback, goforward, goup;
  JWidget filename_entry;
  JWidget filetype;
  char buf[512];
  char *result = NULL;
  char *tok;

  if (!navigation_history)
    navigation_history = jlist_new();

  /* 'buf' will contain the start folder path */
  ustrcpy(buf, init_path);

  /* use the current path */
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
  else {
    /* remove the filename */
    *get_filename(buf) = 0;
  }
  
  if (!window) {
    JWidget view, location;

    /* load the window widget */
    window = load_widget("filesel.jid", "file_selector");
    if (!window)
      return NULL;

    box = jwidget_find_name(window, "box");
    goback = jwidget_find_name(window, "goback");
    goforward = jwidget_find_name(window, "goforward");
    goup = jwidget_find_name(window, "goup");
    location = jwidget_find_name(window, "location");
    filetype = jwidget_find_name(window, "filetype");

    jwidget_focusrest(goback, FALSE);
    jwidget_focusrest(goforward, FALSE);
    jwidget_focusrest(goup, FALSE);

    add_gfxicon_to_button(goback, GFX_ARROW_LEFT, JI_CENTER | JI_MIDDLE);
    add_gfxicon_to_button(goforward, GFX_ARROW_RIGHT, JI_CENTER | JI_MIDDLE);
    add_gfxicon_to_button(goup, GFX_ARROW_UP, JI_CENTER | JI_MIDDLE);

    jbutton_add_command(goback, goback_command);
    jbutton_add_command(goforward, goforward_command);
    jbutton_add_command(goup, goup_command);

    view = jview_new();
    fileview = fileview_new(get_fileitem_from_path(buf), exts);

    jwidget_add_hook(fileview, -1, fileview_msg_proc, NULL);
    jwidget_add_hook(location, -1, location_msg_proc, NULL);
    jwidget_add_hook(filetype, -1, filetype_msg_proc, NULL);

    jwidget_set_name(fileview, "fileview");
    jwidget_magnetic(fileview, TRUE);

    jview_attach(view, fileview);
    jwidget_expansive(view, TRUE);

    jwidget_add_child(box, view);

    jwidget_set_min_size(window, JI_SCREEN_W*9/10, JI_SCREEN_H*9/10);
    jwindow_remap(window);
    jwindow_center(window);

    jwidget_add_tooltip_text(filetype, "hola\nchau");
  }
  else {
    fileview = jwidget_find_name(window, "fileview");
    filetype = jwidget_find_name(window, "filetype");

    jwidget_signal_off(fileview);
    fileview_set_current_folder(fileview, get_fileitem_from_path(buf));
    jwidget_signal_on(fileview);
  }

  /* current location */
  navigation_position = NULL;
  add_in_navigation_history(fileview_get_current_folder(fileview));
  
  /* fill the location combo-box */
  update_location(window);
  update_navigation_buttons(window);

  /* fill file-type combo-box */
  jcombobox_clear(filetype);
  ustrcpy(buf, exts);
  for (tok = ustrtok(buf, ",");
       tok != NULL;
       tok = ustrtok(NULL, ",")) {
    jcombobox_add_string(filetype, tok, NULL);
  }

  /* file name entry field */
  filename_entry = jwidget_find_name(window, "filename");
  jwidget_set_text(filename_entry, get_filename(init_path));
  select_filetype_from_filename(window);

  /* setup the title of the window */
  jwidget_set_text(window, message);

  /* get the ok-button */
  ok = jwidget_find_name(window, "ok");

  /* update the view */
  jview_update(jwidget_get_view(fileview));

  /* open the window and run... the user press ok? */
  jwindow_open_fg(window);
  if (jwindow_get_killer(window) == ok ||
      jwindow_get_killer(window) == fileview) {
    char *p;

    /* open the selected file */
    FileItem *folder = fileview_get_current_folder(fileview);
    assert(folder != NULL);

    ustrcpy(buf, fileitem_get_filename(folder));
    put_backslash(buf);
    ustrcat(buf, jwidget_get_text(filename_entry));

    /* does it not have extension? ...we should add the extension
       selected in the filetype combo-box */
    p = get_extension(buf);
    if (!p || *p == 0) {
      ustrcat(buf, ".");
      ustrcat(buf, jcombobox_get_selected_string(filetype));
    }

    /* duplicate the buffer to return a new string */
    result = jstrdup(buf);

    /* save the path in the configuration file */
    {
      char *name_dup = jstrdup(result);
      char *s = get_filename(name_dup);
      if (s)
	*s = 0;
      set_config_string("FileSelect", "CurrentDirectory", name_dup);
      jfree(name_dup);
    }
  }

  /* TODO why this doesn't work if I remove this? */
  fileview_stop_threads(fileview);
  jwidget_free(window);
  window = NULL;

  return result;
}

/**
 * Updates the content of the combo-box that shows the current
 * location in the file-system.
 */
static void update_location(JWidget window)
{
  char buf[MAX_PATH*2];
  JWidget fileview = jwidget_find_name(window, "fileview");
  JWidget location = jwidget_find_name(window, "location");
  FileItem *current_folder = fileview_get_current_folder(fileview);
  FileItem *fileitem = current_folder;
  JList locations = jlist_new();
  int c, level = 0;
  JLink link;
  int selected_index = -1;

  while (fileitem != NULL) {
    jlist_prepend(locations, fileitem);
    fileitem = fileitem_get_parent(fileitem);
  }

  /* clear all the items from the combo-box */
  jcombobox_clear(location);

  /* add item by item (from root to the specific current folder) */
  level = 0;
  JI_LIST_FOR_EACH(locations, link) {
    fileitem = link->data;

    /* indentation */
    ustrcpy(buf, empty_string);
    for (c=0; c<level; ++c)
      ustrcat(buf, "  ");

    /* location name */
    ustrcat(buf, fileitem_get_displayname(fileitem));

    /* add the new location to the combo-box */
    jcombobox_add_string(location, buf, fileitem);

    if (fileitem == current_folder)
      selected_index = level;
    
    level++;
  }

  jwidget_signal_off(location);
  jcombobox_select_index(location, selected_index);
  jwidget_set_text(jcombobox_get_entry_widget(location),
		   fileitem_get_displayname(current_folder));
  jentry_deselect_text(jcombobox_get_entry_widget(location));
  jwidget_signal_on(location);

  jlist_free(locations);
}

static void update_navigation_buttons(JWidget window)
{
  JWidget fileview = jwidget_find_name(window, "fileview");
  JWidget goback = jwidget_find_name(window, "goback");
  JWidget goforward = jwidget_find_name(window, "goforward");
  JWidget goup = jwidget_find_name(window, "goup");
  FileItem *current_folder = fileview_get_current_folder(fileview);

  /* update the state of the go back button: if the navigation-history
     has two elements and the navigation-position isn't the first
     one */
  if (jlist_length(navigation_history) > 1 &&
      (!navigation_position ||
       navigation_position != jlist_first(navigation_history))) {
    jwidget_enable(goback);
  }
  else {
    jwidget_disable(goback);
  }

  /* update the state of the go forward button: if the
     navigation-history has two elements and the navigation-position
     isn't the last one */
  if (jlist_length(navigation_history) > 1 &&
      (!navigation_position ||
       navigation_position != jlist_last(navigation_history))) {
    jwidget_enable(goforward);
  }
  else {
    jwidget_disable(goforward);
  }

  /* update the state of the go up button: if the current-folder isn't
     the root-item */
  if (current_folder != get_root_fileitem())
    jwidget_enable(goup);
  else
    jwidget_disable(goup);
}

static void add_in_navigation_history(FileItem *folder)
{
  assert(fileitem_is_folder(folder));

  /* remove the history from the current position */
  if (navigation_position) {
    JLink next;
    for (navigation_position = navigation_position->next;
	 navigation_position != navigation_history->end;
	 navigation_position = next) {
      next = navigation_position->next;
      jlist_delete_link(navigation_history,
			navigation_position);
    }
    navigation_position = NULL;
  }

  /* if the history is empty or if the last item isn't the folder that
     we are visiting... */
  if (jlist_empty(navigation_history) ||
      jlist_last_data(navigation_history) != folder) {
    /* ...we can add the location in the history */
    jlist_append(navigation_history, folder);
    navigation_position = jlist_last(navigation_history);
  }
}

static void select_filetype_from_filename(JWidget window)
{
  JWidget entry = jwidget_find_name(window, "filename");
  JWidget filetype = jwidget_find_name(window, "filetype");
  const char *filename = jwidget_get_text(entry);
  char *p = get_extension(filename);
  char buf[MAX_PATH];

  if (p && *p != 0) {
    ustrcpy(buf, get_extension(filename));
    ustrlwr(buf);
    jcombobox_select_string(filetype, buf);
  }
}

static void goback_command(JWidget widget)
{
  JWidget fileview = jwidget_find_name(jwidget_get_window(widget),
				       "fileview");

  if (jlist_length(navigation_history) > 1) {
    if (!navigation_position)
      navigation_position = jlist_last(navigation_history);

    if (navigation_position->prev != navigation_history->end) {
      navigation_position = navigation_position->prev;

      navigation_locked = TRUE;
      fileview_set_current_folder(fileview,
				  navigation_position->data);
      navigation_locked = FALSE;
    }
  }
}

static void goforward_command(JWidget widget)
{
  JWidget fileview = jwidget_find_name(jwidget_get_window(widget),
				       "fileview");

  if (jlist_length(navigation_history) > 1) {
    if (!navigation_position)
      navigation_position = jlist_first(navigation_history);

    if (navigation_position->next != navigation_history->end) {
      navigation_position = navigation_position->next;

      navigation_locked = TRUE;
      fileview_set_current_folder(fileview,
				  navigation_position->data);
      navigation_locked = FALSE;
    }
  }
}

static void goup_command(JWidget widget)
{
  JWidget fileview = jwidget_find_name(jwidget_get_window(widget),
				       "fileview");
  fileview_goup(fileview);
}

/* hook for the 'fileview' widget in the dialog */
static bool fileview_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    switch (msg->signal.num) {
    
      case SIGNAL_FILEVIEW_FILE_SELECTED: {
	FileItem *fileitem = fileview_get_selected(widget);

	if (!fileitem_is_folder(fileitem)) {
	  JWidget window = jwidget_get_window(widget);
	  JWidget entry = jwidget_find_name(window, "filename");
	  const char *filename = fileitem_get_filename(fileitem);

	  jwidget_set_text(entry, get_filename(filename));
	  select_filetype_from_filename(window);
	}
	break;
      }

	/* when a file is accepted */
      case SIGNAL_FILEVIEW_FILE_ACCEPT:
	jwidget_close_window(widget);
	break;

	/* when the current folder change */
      case SIGNAL_FILEVIEW_CURRENT_FOLDER_CHANGED: {
	JWidget window = jwidget_get_window(widget);

	if (!navigation_locked)
	  add_in_navigation_history(fileview_get_current_folder(widget));

	update_location(window);
	update_navigation_buttons(window);
	break;
      }

    }
  }
  return FALSE;
}

/* hook for the 'location' combo-box */
static bool location_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    switch (msg->signal.num) {

      /* when the user change the location we have to set the
	 current-folder in the 'fileview' widget */
      case JI_SIGNAL_COMBOBOX_SELECT: {
	FileItem *fileitem =
	  jcombobox_get_data(widget,
			     jcombobox_get_selected_index(widget));

	if (fileitem) {
	  JWidget fileview = jwidget_find_name(jwidget_get_window(widget),
					       "fileview");

	  fileview_set_current_folder(fileview, fileitem);

	  /* refocus the 'fileview' (the focus in that widget is more
	     useful for the user) */
	  jmanager_set_focus(fileview);
	}
	break;
      }
    }
  }
  return FALSE;
}

/* hook for the 'filetype' combo-box */
static bool filetype_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    switch (msg->signal.num) {

      /* when the user select a new file-type (extension), we have to
	 change the file-extension in the 'filename' entry widget */
      case JI_SIGNAL_COMBOBOX_SELECT: {
	const char *ext = jcombobox_get_selected_string(widget);
	JWidget window = jwidget_get_window(widget);
	JWidget entry = jwidget_find_name(window, "filename");
	char buf[MAX_PATH];
	char *p;

	ustrcpy(buf, jwidget_get_text(entry));
	p = get_extension(buf);
	if (p && *p != 0) {
	  ustrcpy(p, ext);
	  jwidget_set_text(entry, buf);
	}
	break;
      }

    }
  }
  return FALSE;
}

