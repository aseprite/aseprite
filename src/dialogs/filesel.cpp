/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "app.h"
#include "base/bind.h"
#include "base/path.h"
#include "base/split_string.h"
#include "file/file.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "recent_files.h"
#include "skin/skin_parts.h"
#include "widgets/fileview.h"

#if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')
#  define HAVE_DRIVES
#endif

#ifndef MAX_PATH
#  define MAX_PATH 4096		/* TODO this is needed for Linux, is it correct? */
#endif

template<class Container>
class NullableIterator
{
public:
  typedef typename Container::iterator iterator;

  NullableIterator() : m_isNull(true) { }

  void reset() { m_isNull = true; }

  bool isNull() const { return m_isNull; }
  bool isValid() const { return !m_isNull; }

  iterator getIterator() {
    ASSERT(!m_isNull);
    return m_iterator;
  }

  void setIterator(const iterator& it) {
    m_isNull = false;
    m_iterator = it;
  }

private:
  bool m_isNull;
  typename Container::iterator m_iterator;
};

// Variables used only to maintain the history of navigation.
static FileItemList* navigation_history = NULL;	// Set of FileItems navigated
static NullableIterator<FileItemList> navigation_position; // Current position in the navigation history
static bool navigation_locked = false;	// If true the navigation_history isn't
					// modified if the current folder changes
					//(used when the back/forward buttons
					// are pushed)

static void update_location(Widget* window);
static void update_navigation_buttons(Widget* window);
static void add_in_navigation_history(IFileItem* folder);
static void select_filetype_from_filename(Widget* window);

static void goback_command(Widget* widget);
static void goforward_command(Widget* widget);
static void goup_command(Widget* widget);

static bool fileview_msg_proc(Widget* widget, Message* msg);
static bool location_msg_proc(Widget* widget, Message* msg);
static bool filetype_msg_proc(Widget* widget, Message* msg);
static bool filename_msg_proc(Widget* widget, Message* msg);

// Slot for App::Exit signal 
static void on_exit_delete_navigation_history()
{
  delete navigation_history;
}

/**
 * Shows the dialog to select a file in ASE.
 * 
 * Mainly it uses:
 * - the 'file_system' routines.
 * - the 'widgets/fileview' widget.
 */
base::string ase_file_selector(const base::string& message,
			       const base::string& init_path,
			       const base::string& exts)
{
  static Frame* window = NULL;
  Widget* fileview;
  Entry* filename_entry;
  ComboBox* filetype;
  base::string result;

  FileSystemModule::instance()->refresh();

  if (!navigation_history) {
    navigation_history = new FileItemList();
    App::instance()->Exit.connect(&on_exit_delete_navigation_history);
  }

  // we have to find where the user should begin to browse files (start_folder)
  base::string start_folder_path;
  IFileItem* start_folder = NULL;

  // if init_path doesn't contain a path...
  if (base::get_file_path(init_path).empty()) {
    // get the saved `path' in the configuration file
    base::string path = get_config_string("FileSelect", "CurrentDirectory", "");
    start_folder = FileSystemModule::instance()->getFileItemFromPath(path);

    // is the folder find?
    if (!start_folder) {
      // if the `path' doesn't exist...
      if (path.empty() || (!FileSystemModule::instance()->dirExists(path))) {
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

      start_folder_path = base::join_path(path, init_path);
    }
  }
  else {
    // remove the filename
    start_folder_path = base::join_path(base::get_file_path(init_path), "");
  }
  start_folder_path = base::fix_path_separators(start_folder_path);

  if (!start_folder)
    start_folder = FileSystemModule::instance()->getFileItemFromPath(start_folder_path);

  PRINTF("start_folder_path = %s (%p)\n", start_folder_path.c_str(), start_folder);

  if (!window) {
    // load the window widget
    window = static_cast<Frame*>(load_widget("file_selector.xml", "file_selector"));

    Widget* box = window->findChild("box");
    Button* goback = window->findChildT<Button>("goback");
    Button* goforward = window->findChildT<Button>("goforward");
    Button* goup = window->findChildT<Button>("goup");
    Widget* location = window->findChild("location");
    filetype = window->findChildT<ComboBox>("filetype");
    ASSERT(filetype != NULL);
    filename_entry = window->findChildT<Entry>("filename");

    jwidget_focusrest(goback, false);
    jwidget_focusrest(goforward, false);
    jwidget_focusrest(goup, false);

    set_gfxicon_to_button(goback,
			  PART_COMBOBOX_ARROW_LEFT,
			  PART_COMBOBOX_ARROW_LEFT_SELECTED,
			  PART_COMBOBOX_ARROW_LEFT_DISABLED,
			  JI_CENTER | JI_MIDDLE);
    set_gfxicon_to_button(goforward,
			  PART_COMBOBOX_ARROW_RIGHT,
			  PART_COMBOBOX_ARROW_RIGHT_SELECTED,
			  PART_COMBOBOX_ARROW_RIGHT_DISABLED,
			  JI_CENTER | JI_MIDDLE);
    set_gfxicon_to_button(goup,
			  PART_COMBOBOX_ARROW_UP,
			  PART_COMBOBOX_ARROW_UP_SELECTED,
			  PART_COMBOBOX_ARROW_UP_DISABLED,
			  JI_CENTER | JI_MIDDLE);

    setup_mini_look(goback);
    setup_mini_look(goforward);
    setup_mini_look(goup);

    goback->Click.connect(Bind<void>(&goback_command, goback));
    goforward->Click.connect(Bind<void>(&goforward_command, goforward));
    goup->Click.connect(Bind<void>(&goup_command, goup));

    View* view = new View();
    fileview = fileview_new(start_folder, exts);

    jwidget_add_hook(fileview, -1, fileview_msg_proc, NULL);
    jwidget_add_hook(location, -1, location_msg_proc, NULL);
    jwidget_add_hook(filetype, -1, filetype_msg_proc, NULL);
    jwidget_add_hook(filename_entry, -1, filename_msg_proc, NULL);

    fileview->setName("fileview");

    view->attachToView(fileview);
    jwidget_expansive(view, true);

    box->addChild(view);

    jwidget_set_min_size(window, JI_SCREEN_W*9/10, JI_SCREEN_H*9/10);
    window->remap_window();
    window->center_window();
  }
  else {
    fileview = window->findChild("fileview");
    filetype = window->findChildT<ComboBox>("filetype");
    ASSERT(filetype != NULL);
    filename_entry = window->findChildT<Entry>("filename");

    jwidget_signal_off(fileview);
    fileview_set_current_folder(fileview, start_folder);
    jwidget_signal_on(fileview);
  }

  // current location
  navigation_position.reset();
  add_in_navigation_history(fileview_get_current_folder(fileview));
  
  // fill the location combo-box
  update_location(window);
  update_navigation_buttons(window);

  // fill file-type combo-box
  filetype->removeAllItems();

  std::vector<base::string> tokens;
  std::vector<base::string>::iterator tok;

  base::split_string(exts, tokens, ",");
  for (tok=tokens.begin(); tok!=tokens.end(); ++tok)
    filetype->addItem(tok->c_str());

  // file name entry field
  filename_entry->setText(base::get_file_name(init_path).c_str());
  select_filetype_from_filename(window);
  filename_entry->selectText(0, -1);

  // setup the title of the window
  window->setText(message.c_str());

  // get the ok-button
  Widget* ok = window->findChild("ok");

  // update the view
  View::getView(fileview)->updateView();

  // open the window and run... the user press ok?
again:
  window->open_window_fg();
  if (window->get_killer() == ok ||
      window->get_killer() == fileview) {
    // open the selected file
    IFileItem *folder = fileview_get_current_folder(fileview);
    ASSERT(folder);

    base::string fn = filename_entry->getText();
    base::string buf;
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

      base::string fn2 = fn;
#ifdef WIN32
      fn2 = base::string_to_lower(fn2);
#endif

      for (FileItemList::const_iterator
	     it=children.begin(); it!=children.end(); ++it) {
	IFileItem* child = *it;
	base::string child_name = child->getDisplayName();

#ifdef WIN32
	child_name = base::string_to_lower(child_name);
#endif
	if (child_name == fn2) {
	  enter_folder = *it;
	  buf = enter_folder->getFileName();
	  break;
	}
      }

      if (!enter_folder) {
	// does the file-name entry have separators?
	if (base::is_path_separator(*fn.begin())) { // absolute path (UNIX style)
#ifdef WIN32
	  // get the drive of the current folder
	  base::string drive = folder->getFileName();
	  if (drive.size() >= 2 && drive[1] == ':') {
	    buf += drive[0];
	    buf += ':';
	    buf += fn;
	  }
	  else
	    buf = base::join_path("C:", fn);
#else
	  buf = fn;
#endif
	}
#ifdef WIN32
	// does the file-name entry have colon?
	else if (fn.find(':') != base::string::npos) { // absolute path on Windows
	  if (fn.size() == 2 && fn[1] == ':') {
	    buf = base::join_path(fn, "");
	  }
	  else {
	    buf = fn;
	  }
	}
#endif
	else {
	  buf = folder->getFileName();
	  buf = base::join_path(buf, fn);
	}
	buf = base::fix_path_separators(buf);

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
    if (base::get_file_extension(buf).empty()) {
      buf += '.';
      buf += filetype->getItemText(filetype->getSelectedItem());
    }

    // duplicate the buffer to return a new string
    result = buf;

    // save the path in the configuration file
    base::string lastpath = folder->getKeyName();
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
static void update_location(Widget* window)
{
  Widget* fileview = window->findChild("fileview");
  ComboBox* location = dynamic_cast<ComboBox*>(window->findChild("location"));
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
    base::string buf;
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
    RecentFiles::const_iterator it = App::instance()->getRecentFiles()->begin();
    RecentFiles::const_iterator end = App::instance()->getRecentFiles()->end();
    for (; it != end; ++it) {
      // Get the path of the recent file
      base::string path = base::get_file_path(*it);

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
    location->getEntryWidget()->deselectText();
    jwidget_signal_on(location);
  }

  jlist_free(locations);
}

static void update_navigation_buttons(Widget* window)
{
  Widget* fileview = window->findChild("fileview");
  Widget* goback = window->findChild("goback");
  Widget* goforward = window->findChild("goforward");
  Widget* goup = window->findChild("goup");
  IFileItem* current_folder = fileview_get_current_folder(fileview);

  // Update the state of the go back button: if the navigation-history
  // has two elements and the navigation-position isn't the first one.
  goback->setEnabled(navigation_history->size() > 1 &&
		     (navigation_position.isNull() ||
		      navigation_position.getIterator() != navigation_history->begin()));

  // Update the state of the go forward button: if the
  // navigation-history has two elements and the navigation-position
  // isn't the last one.
  goforward->setEnabled(navigation_history->size() > 1 &&
			(navigation_position.isNull() ||
			 navigation_position.getIterator() != navigation_history->end()-1));

  // Update the state of the go up button: if the current-folder isn't
  // the root-item
  goup->setEnabled(current_folder != FileSystemModule::instance()->getRootFileItem());
}

static void add_in_navigation_history(IFileItem* folder)
{
  ASSERT(folder != NULL);
  ASSERT(folder->isFolder());

  // Remove the history from the current position
  if (navigation_position.isValid()) {
    navigation_history->erase(navigation_position.getIterator()+1, navigation_history->end());
    navigation_position.reset();
  }

  // If the history is empty or if the last item isn't the folder that
  // we are visiting...
  if (navigation_history->empty() ||
      navigation_history->back() != folder) {
    // We can add the location in the history
    navigation_history->push_back(folder);
    navigation_position.setIterator(navigation_history->end()-1);
  }
}

static void select_filetype_from_filename(Widget* window)
{
  Widget* entry = window->findChild("filename");
  ComboBox* filetype = dynamic_cast<ComboBox*>(window->findChild("filetype"));
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

static void goback_command(Widget* widget)
{
  Widget* fileview = widget->findSibling("fileview");

  if (navigation_history->size() > 1) {
    if (navigation_position.isNull())
      navigation_position.setIterator(navigation_history->end()-1);

    if (navigation_position.getIterator() != navigation_history->begin()) {
      navigation_position.setIterator(navigation_position.getIterator()-1);

      navigation_locked = true;
      fileview_set_current_folder(fileview, *navigation_position.getIterator());
      navigation_locked = false;
    }
  }
}

static void goforward_command(Widget* widget)
{
  Widget* fileview = widget->findSibling("fileview");

  if (jlist_length(navigation_history) > 1) {
    if (navigation_position.isNull())
      navigation_position.setIterator(navigation_history->begin());

    if (navigation_position.getIterator() != navigation_history->end()-1) {
      navigation_position.setIterator(navigation_position.getIterator()+1);

      navigation_locked = true;
      fileview_set_current_folder(fileview, *navigation_position.getIterator());
      navigation_locked = false;
    }
  }
}

static void goup_command(Widget* widget)
{
  Widget* fileview = widget->findSibling("fileview");
  fileview_goup(fileview);
}

/* hook for the 'fileview' widget in the dialog */
static bool fileview_msg_proc(Widget* widget, Message* msg)
{
  if (msg->type == JM_SIGNAL) {
    switch (msg->signal.num) {
    
      case SIGNAL_FILEVIEW_FILE_SELECTED: {
	IFileItem* fileitem = fileview_get_selected(widget);

	if (!fileitem->isFolder()) {
	  Frame* window = static_cast<Frame*>(widget->getRoot());
	  Widget* entry = window->findChild("filename");
	  base::string filename = base::get_file_name(fileitem->getFileName());

	  entry->setText(filename.c_str());
	  select_filetype_from_filename(window);
	}
	break;
      }

	/* when a file is accepted */
      case SIGNAL_FILEVIEW_FILE_ACCEPT:
	widget->closeWindow();
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
static bool location_msg_proc(Widget* widget, Message* msg)
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
	  base::string path = combobox->getItemText(itemIndex);
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
static bool filetype_msg_proc(Widget* widget, Message* msg)
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
	Entry* entry = window->findChildT<Entry>("filename");
	char buf[MAX_PATH];
	char* p;

	ustrcpy(buf, entry->getText());
	p = get_extension(buf);
	if (p && *p != 0) {
	  ustrcpy(p, ext.c_str());
	  entry->setText(buf);
	  entry->selectText(0, -1);
	}
	break;
      }

    }
  }
  return false;
}

static bool filename_msg_proc(Widget* widget, Message* msg)
{
  if (msg->type == JM_KEYRELEASED && msg->key.ascii >= 32) {
    // Check if all keys are released
    for (int c=0; c<KEY_MAX; ++c) {
      if (key[c])
	return false;
    }

    // String to be autocompleted
    base::string left_part = widget->getText();
    if (left_part.empty())
      return false;

    // First we'll need the fileview widget
    Widget* fileview = widget->findSibling("fileview");

    const FileItemList& children = fileview_get_filelist(fileview);

    for (FileItemList::const_iterator
	   it=children.begin(); it!=children.end(); ++it) {
      IFileItem* child = *it;
      base::string child_name = child->getDisplayName();

      base::string::iterator it1, it2;

      for (it1 = child_name.begin(), it2 = left_part.begin();
	   it1!=child_name.end() && it2!=left_part.end();
	   ++it1, ++it2) {
	if (std::tolower(*it1) != std::tolower(*it2))
	  break;
      }

      // Is the pattern (left_part) in the child_name's beginning?
      if (it2 == left_part.end()) {
	widget->setText(child_name.c_str());
	((Entry*)widget)->selectText(child_name.size(),
				     left_part.size());
	clear_keybuf();
	return true;
      }
    }
  }
  return false;
}

