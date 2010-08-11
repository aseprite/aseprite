/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <algorithm>
#include <cctype>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <errno.h>

#include "jinete/jinete.h"

#include "app.h"
#include "core/cfg.h"
#include "file/file.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "recent_files.h"
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
static bool navigation_locked = false;	/* if true the navigation_history isn't
					   modified if the current folder
					   changes (used when the back/forward
					   buttons are pushed) */

static void update_location(JWidget window);
static void update_navigation_buttons(JWidget window);
static void add_in_navigation_history(IFileItem* folder);
static void select_filetype_from_filename(JWidget window);

static void goback_command(JWidget widget);
static void goforward_command(JWidget widget);
static void goup_command(JWidget widget);

static bool fileview_msg_proc(JWidget widget, JMessage msg);
static bool location_msg_proc(JWidget widget, JMessage msg);
static bool filetype_msg_proc(JWidget widget, JMessage msg);
static bool filename_msg_proc(JWidget widget, JMessage msg);

// Slot for App::Exit signal 
static void on_exit_delete_navigation_history()
{
  jlist_free(navigation_history);
}

/**
 * Shows the dialog to select a file in ASE.
 * 
 * Mainly it uses:
 * - the 'core/file_system' routines.
 * - the 'widgets/fileview' widget.
 */
jstring ase_file_selector(const jstring& message,
			  const jstring& init_path,
			  const jstring& exts)
{
  static Frame* window = NULL;
  Widget* fileview;
  Widget* filename_entry;
  ComboBox* filetype;
  jstring result;

  FileSystemModule::instance()->refresh();

  if (!navigation_history) {
    navigation_history = jlist_new();
    App::instance()->Exit.connect(&on_exit_delete_navigation_history);
  }

  // we have to find where the user should begin to browse files (start_folder)
  jstring start_folder_path;
  IFileItem* start_folder = NULL;

  // if init_path doesn't contain a path...
  if (init_path.filepath().empty()) {
    // get the saved `path' in the configuration file
    jstring path = get_config_string("FileSelect", "CurrentDirectory", "");
    start_folder = FileSystemModule::instance()->getFileItemFromPath(path);

    // is the folder find?
    if (!start_folder) {
      // if the `path' doesn't exist...
      if (path.empty() || (!ji_dir_exists(path.c_str()))) {
	// we can get the current `path' from the system
#ifdef HAVE_DRIVES
	int drive = _al_getdrive();
#else
	int drive = 0;
#endif
	char tmp[1024];
	_al_getdcwd(drive, tmp, sizeof(tmp) - ucwidth(OTHER_PATH_SEPARATOR));
	path = tmp;
      }

      start_folder_path = path / init_path;
    }
  }
  else {
    // remove the filename
    start_folder_path = init_path.filepath() / "";
  }
  start_folder_path.fix_separators();

  if (!start_folder)
    start_folder = FileSystemModule::instance()->getFileItemFromPath(start_folder_path);

  PRINTF("start_folder_path = %s (%p)\n", start_folder_path.c_str(), start_folder);

  if (!window) {
    // load the window widget
    window = static_cast<Frame*>(load_widget("file_selector.xml", "file_selector"));

    JWidget box = jwidget_find_name(window, "box");
    JWidget goback = jwidget_find_name(window, "goback");
    JWidget goforward = jwidget_find_name(window, "goforward");
    JWidget goup = jwidget_find_name(window, "goup");
    JWidget location = jwidget_find_name(window, "location");
    filetype = dynamic_cast<ComboBox*>(jwidget_find_name(window, "filetype"));
    ASSERT(filetype != NULL);
    filename_entry = jwidget_find_name(window, "filename");

    jwidget_focusrest(goback, false);
    jwidget_focusrest(goforward, false);
    jwidget_focusrest(goup, false);

    add_gfxicon_to_button(goback, GFX_ARROW_LEFT, JI_CENTER | JI_MIDDLE);
    add_gfxicon_to_button(goforward, GFX_ARROW_RIGHT, JI_CENTER | JI_MIDDLE);
    add_gfxicon_to_button(goup, GFX_ARROW_UP, JI_CENTER | JI_MIDDLE);

    setup_mini_look(goback);
    setup_mini_look(goforward);
    setup_mini_look(goup);

    jbutton_add_command(goback, goback_command);
    jbutton_add_command(goforward, goforward_command);
    jbutton_add_command(goup, goup_command);

    JWidget view = jview_new();
    fileview = fileview_new(start_folder, exts);

    jwidget_add_hook(fileview, -1, fileview_msg_proc, NULL);
    jwidget_add_hook(location, -1, location_msg_proc, NULL);
    jwidget_add_hook(filetype, -1, filetype_msg_proc, NULL);
    jwidget_add_hook(filename_entry, -1, filename_msg_proc, NULL);

    fileview->setName("fileview");

    jview_attach(view, fileview);
    jwidget_expansive(view, true);

    jwidget_add_child(box, view);

    jwidget_set_min_size(window, JI_SCREEN_W*9/10, JI_SCREEN_H*9/10);
    window->remap_window();
    window->center_window();
  }
  else {
    fileview = jwidget_find_name(window, "fileview");
    filetype = dynamic_cast<ComboBox*>(jwidget_find_name(window, "filetype"));
    ASSERT(filetype != NULL);
    filename_entry = jwidget_find_name(window, "filename");

    jwidget_signal_off(fileview);
    fileview_set_current_folder(fileview, start_folder);
    jwidget_signal_on(fileview);
  }

  // current location
  navigation_position = NULL;
  add_in_navigation_history(fileview_get_current_folder(fileview));
  
  // fill the location combo-box
  update_location(window);
  update_navigation_buttons(window);

  // fill file-type combo-box
  filetype->removeAllItems();

  std::vector<jstring> tokens;
  std::vector<jstring>::iterator tok;

  exts.split(',', tokens);
  for (tok=tokens.begin(); tok!=tokens.end(); ++tok)
    filetype->addItem(tok->c_str());

  // file name entry field
  filename_entry->setText(init_path.filename().c_str());
  select_filetype_from_filename(window);
  jentry_select_text(filename_entry, 0, -1);

  // setup the title of the window
  window->setText(message.c_str());

  // get the ok-button
  JWidget ok = jwidget_find_name(window, "ok");

  // update the view
  jview_update(jwidget_get_view(fileview));

  // open the window and run... the user press ok?
again:
  window->open_window_fg();
  if (window->get_killer() == ok ||
      window->get_killer() == fileview) {
    // open the selected file
    IFileItem *folder = fileview_get_current_folder(fileview);
    ASSERT(folder);

    jstring fn = filename_entry->getText();
    jstring buf;
    IFileItem* enter_folder = NULL;

    // up a level?
    if (fn == "..") {
      enter_folder = folder->getParent();
      if (!enter_folder)
	enter_folder = folder;
    }
    else if (!fn.empty()) {
      // check if the user specified in "fn" a item of "fileview"
      const FileItemList& children = fileview_get_filelist(fileview);

      jstring fn2 = fn;
#ifdef ALLEGRO_WINDOWS
      fn2.tolower();
#endif

      for (FileItemList::const_iterator
	     it=children.begin(); it!=children.end(); ++it) {
	IFileItem* child = *it;
	jstring child_name = child->getDisplayName();

#ifdef ALLEGRO_WINDOWS
	child_name.tolower();
#endif
	if (child_name == fn2) {
	  enter_folder = *it;
	  buf = enter_folder->getFileName();
	  break;
	}
      }

      if (!enter_folder) {
	// does the file-name entry have separators?
	if (jstring::is_separator(fn.front())) { // absolute path (UNIX style)
#ifdef ALLEGRO_WINDOWS
	  // get the drive of the current folder
	  jstring drive = folder->getFileName();
	  if (drive.size() >= 2 && drive[1] == ':') {
	    buf += drive[0];
	    buf += ':';
	    buf += fn;
	  }
	  else
	    buf = jstring("C:") / fn;
#else
	  buf = fn;
#endif
	}
#ifdef ALLEGRO_WINDOWS
	// does the file-name entry have colon?
	else if (fn.find(':') != jstring::npos) { // absolute path on Windows
	  if (fn.size() == 2 && fn[1] == ':') {
	    buf = fn / "";
	  }
	  else {
	    buf = fn;
	  }
	}
#endif
	else {
	  buf = folder->getFileName();
	  buf /= fn;
	}
	buf.fix_separators();

	// we can check if 'buf' is a folder, so we have to enter in it
	enter_folder = FileSystemModule::instance()->getFileItemFromPath(buf);
      }
    }
    else {
      // show the window again
      window->setVisible(true);
      goto again;
    }

    // did we find a folder to enter?
    if (enter_folder &&
	enter_folder->isFolder() &&
	enter_folder->isBrowsable()) {
      // enter in the folder that was specified in the 'filename_entry'
      fileview_set_current_folder(fileview, enter_folder);

      // clear the text of the entry widget
      filename_entry->setText("");

      // show the window again
      window->setVisible(true);
      goto again;
    }
    // else file-name specified in the entry is really a file to open...

    // does it not have extension? ...we should add the extension
    // selected in the filetype combo-box
    if (buf.extension().empty()) {
      buf += '.';
      buf += filetype->getItemText(filetype->getSelectedItem());
    }

    // duplicate the buffer to return a new string
    result = buf;

    // save the path in the configuration file
    jstring lastpath = folder->getKeyName();
    set_config_string("FileSelect", "CurrentDirectory",
		      lastpath.c_str());
  }

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
  JWidget fileview = jwidget_find_name(window, "fileview");
  ComboBox* location = dynamic_cast<ComboBox*>(jwidget_find_name(window, "location"));
  ASSERT(location != NULL);

  IFileItem* current_folder = fileview_get_current_folder(fileview);
  IFileItem* fileitem = current_folder;
  JList locations = jlist_new();
  JLink link;
  int selected_index = -1;
  int newItem;

  while (fileitem != NULL) {
    jlist_prepend(locations, fileitem);
    fileitem = fileitem->getParent();
  }

  // Clear all the items from the combo-box
  location->removeAllItems();

  // Add item by item (from root to the specific current folder)
  int level = 0;
  JI_LIST_FOR_EACH(locations, link) {
    fileitem = reinterpret_cast<IFileItem*>(link->data);

    // Indentation
    jstring buf;
    for (int c=0; c<level; ++c)
      buf += "  ";

    // Location name
    buf += fileitem->getDisplayName();

    // Add the new location to the combo-box
    newItem = location->addItem(buf.c_str());
    location->setItemData(newItem, fileitem);

    if (fileitem == current_folder)
      selected_index = level;
    
    level++;
  }

  // Add paths from recent files list
  {
    newItem = location->addItem("");
    newItem = location->addItem("-------- Recent Paths --------");

    std::set<std::string> included;

    // For each recent file...
    RecentFiles::const_iterator it;
    for (it = RecentFiles::begin(); it != RecentFiles::end(); ++it) {
      // Get the path of the recent file
      std::string path = jstring(*it).filepath();

      // Check if the path was not already included in the list
      if (included.find(path) == included.end()) {
	included.insert(path);

	location->addItem(path);
      }
    }
  }

  // Select the location
  {
    jwidget_signal_off(location);
    location->setSelectedItem(selected_index);
    location->getEntryWidget()->setText(current_folder->getDisplayName().c_str());
    jentry_deselect_text(location->getEntryWidget());
    jwidget_signal_on(location);
  }

  jlist_free(locations);
}

static void update_navigation_buttons(JWidget window)
{
  JWidget fileview = jwidget_find_name(window, "fileview");
  JWidget goback = jwidget_find_name(window, "goback");
  JWidget goforward = jwidget_find_name(window, "goforward");
  JWidget goup = jwidget_find_name(window, "goup");
  IFileItem* current_folder = fileview_get_current_folder(fileview);

  /* update the state of the go back button: if the navigation-history
     has two elements and the navigation-position isn't the first
     one */
  goback->setEnabled(jlist_length(navigation_history) > 1 &&
		     (!navigation_position ||
		      navigation_position != jlist_first(navigation_history)));

  /* update the state of the go forward button: if the
     navigation-history has two elements and the navigation-position
     isn't the last one */
  goforward->setEnabled(jlist_length(navigation_history) > 1 &&
			(!navigation_position ||
			 navigation_position != jlist_last(navigation_history)));

  /* update the state of the go up button: if the current-folder isn't
     the root-item */
  goup->setEnabled(current_folder != FileSystemModule::instance()->getRootFileItem());
}

static void add_in_navigation_history(IFileItem* folder)
{
  ASSERT(folder != NULL);
  ASSERT(folder->isFolder());

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
  ComboBox* filetype = dynamic_cast<ComboBox*>(jwidget_find_name(window, "filetype"));
  ASSERT(filetype != NULL);

  const char *filename = entry->getText();
  char *p = get_extension(filename);
  char buf[MAX_PATH];

  if (p && *p != 0) {
    ustrcpy(buf, get_extension(filename));
    ustrlwr(buf);
    filetype->setSelectedItem(filetype->findItemIndex(buf));
  }
}

static void goback_command(JWidget widget)
{
  JWidget fileview = widget->findSibling("fileview");

  if (jlist_length(navigation_history) > 1) {
    if (!navigation_position)
      navigation_position = jlist_last(navigation_history);

    if (navigation_position->prev != navigation_history->end) {
      navigation_position = navigation_position->prev;

      navigation_locked = true;
      fileview_set_current_folder(fileview,
				  reinterpret_cast<IFileItem*>(navigation_position->data));
      navigation_locked = false;
    }
  }
}

static void goforward_command(JWidget widget)
{
  JWidget fileview = widget->findSibling("fileview");

  if (jlist_length(navigation_history) > 1) {
    if (!navigation_position)
      navigation_position = jlist_first(navigation_history);

    if (navigation_position->next != navigation_history->end) {
      navigation_position = navigation_position->next;

      navigation_locked = true;
      fileview_set_current_folder(fileview,
				  reinterpret_cast<IFileItem*>(navigation_position->data));
      navigation_locked = false;
    }
  }
}

static void goup_command(JWidget widget)
{
  JWidget fileview = widget->findSibling("fileview");
  fileview_goup(fileview);
}

/* hook for the 'fileview' widget in the dialog */
static bool fileview_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    switch (msg->signal.num) {
    
      case SIGNAL_FILEVIEW_FILE_SELECTED: {
	IFileItem* fileitem = fileview_get_selected(widget);

	if (!fileitem->isFolder()) {
	  Frame* window = static_cast<Frame*>(widget->getRoot());
	  Widget* entry = window->findChild("filename");
	  jstring filename = fileitem->getFileName().filename();

	  entry->setText(filename.c_str());
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
	Frame* window = static_cast<Frame*>(widget->getRoot());

	if (!navigation_locked)
	  add_in_navigation_history(fileview_get_current_folder(widget));

	update_location(window);
	update_navigation_buttons(window);
	break;
      }

    }
  }
  return false;
}

// Hook for the 'location' combo-box
static bool location_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    ComboBox* combobox = dynamic_cast<ComboBox*>(widget);
    ASSERT(combobox != NULL);

    switch (msg->signal.num) {

      // When the user change the location we have to set the
      // current-folder in the 'fileview' widget
      case JI_SIGNAL_COMBOBOX_SELECT: {
	int itemIndex = combobox->getSelectedItem();
	IFileItem* fileitem = reinterpret_cast<IFileItem*>(combobox->getItemData(itemIndex));

	// Maybe the user selected a recent file path
	if (fileitem == NULL) {
	  jstring path = combobox->getItemText(itemIndex);
	  if (!path.empty())
	    fileitem = FileSystemModule::instance()->getFileItemFromPath(path);
	}

	if (fileitem != NULL) {
	  Widget* fileview = widget->findSibling("fileview");

	  fileview_set_current_folder(fileview, fileitem);

	  // Refocus the 'fileview' (the focus in that widget is more
	  // useful for the user)
	  jmanager_set_focus(fileview);
	}
	break;
      }
    }
  }
  return false;
}

// Hook for the 'filetype' combo-box
static bool filetype_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL) {
    ComboBox* combobox = dynamic_cast<ComboBox*>(widget);
    ASSERT(combobox != NULL);

    switch (msg->signal.num) {

      // When the user select a new file-type (extension), we have to
      // change the file-extension in the 'filename' entry widget
      case JI_SIGNAL_COMBOBOX_SELECT: {
	std::string ext = combobox->getItemText(combobox->getSelectedItem());
	Frame* window = static_cast<Frame*>(combobox->getRoot());
	Widget* entry = window->findChild("filename");
	char buf[MAX_PATH];
	char* p;

	ustrcpy(buf, entry->getText());
	p = get_extension(buf);
	if (p && *p != 0) {
	  ustrcpy(p, ext.c_str());
	  entry->setText(buf);
	  jentry_select_text(entry, 0, -1);
	}
	break;
      }

    }
  }
  return false;
}

static bool filename_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_KEYRELEASED && msg->key.ascii >= 32) {
    // Check if all keys are released
    for (int c=0; c<KEY_MAX; ++c) {
      if (key[c])
	return false;
    }

    // String to be autocompleted
    jstring left_part = widget->getText();
    if (left_part.empty())
      return false;

    // First we'll need the fileview widget
    Widget* fileview = widget->findSibling("fileview");

    const FileItemList& children = fileview_get_filelist(fileview);

    for (FileItemList::const_iterator
	   it=children.begin(); it!=children.end(); ++it) {
      IFileItem* child = *it;
      jstring child_name = child->getDisplayName();

      jstring::iterator it1, it2;

      for (it1 = child_name.begin(), it2 = left_part.begin();
	   it1!=child_name.end() && it2!=left_part.end();
	   ++it1, ++it2) {
	if (std::tolower(*it1) != std::tolower(*it2))
	  break;
      }

      // Is the pattern (left_part) in the child_name's beginning?
      if (it2 == left_part.end()) {
	widget->setText(child_name.c_str());
	jentry_select_text(widget,
			   child_name.size(),
			   left_part.size());
	clear_keybuf();
	return true;
      }
    }
  }
  return false;
}

