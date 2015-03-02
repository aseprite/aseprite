// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/file_selector.h"

#include "app/app.h"
#include "app/console.h"
#include "app/file/file.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/recent_files.h"
#include "app/ui/file_list.h"
#include "app/ui/skin/skin_parts.h"
#include "app/widget_loader.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/split_string.h"
#include "base/unique_ptr.h"
#include "ui/ui.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <iterator>
#include <set>
#include <string>
#include <vector>

#ifndef MAX_PATH
#  define MAX_PATH 4096         // TODO this is needed for Linux, is it correct?
#endif

namespace app {

using namespace app::skin;
using namespace ui;

template<class Container>
class NullableIterator {
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
static FileItemList* navigation_history = NULL; // Set of FileItems navigated
static NullableIterator<FileItemList> navigation_position; // Current position in the navigation history

// Slot for App::Exit signal
static void on_exit_delete_navigation_history()
{
  delete navigation_history;
}

class CustomFileNameEntry : public Entry
{
public:
  CustomFileNameEntry() :
    Entry(256, ""),
    m_fileList(NULL),
    m_timer(250, this) {
    m_timer.Tick.connect(&CustomFileNameEntry::onTick, this);
  }

  void setAssociatedFileList(FileList* fileList) {
    m_fileList = fileList;
  }

protected:
  virtual void onEntryChange() override {
    Entry::onEntryChange();

    // Start timer to autocomplete again.
    m_timer.start();
  }

  void onTick() {
    m_timer.stop();

    // String to be autocompleted
    std::string left_part = getText();
    if (left_part.empty())
      return;

    const FileItemList& children = m_fileList->getFileList();

    for (IFileItem* child : children) {
      std::string child_name = child->getDisplayName();
      std::string::iterator it1, it2;

      for (it1 = child_name.begin(), it2 = left_part.begin();
           it1 != child_name.end() && it2 != left_part.end();
           ++it1, ++it2) {
        if (std::tolower(*it1) != std::tolower(*it2))
          break;
      }

      // Is the pattern (left_part) in the child_name's beginning?
      if (it2 == left_part.end()) {
        setText(left_part + child_name.substr(left_part.size()));
        selectText(child_name.size(), left_part.size());
        return;
      }
    }
  }

private:
  FileList* m_fileList;
  Timer m_timer;
};

// Class to create CustomFileNameEntries.
class CustomFileNameEntryCreator : public app::WidgetLoader::IWidgetTypeCreator {
public:
  ~CustomFileNameEntryCreator() { }
  void dispose() override { delete this; }
  Widget* createWidgetFromXml(const TiXmlElement* xmlElem) override {
    return new CustomFileNameEntry();
  }
};

class CustomFileNameItem : public ListItem
{
public:
  CustomFileNameItem(const char* text, IFileItem* fileItem)
    : ListItem(text)
    , m_fileItem(fileItem)
  {
  }

  IFileItem* getFileItem() { return m_fileItem; }

private:
  IFileItem* m_fileItem;
};

class CustomFolderNameItem : public ListItem
{
public:
  CustomFolderNameItem(const char* text)
    : ListItem(text)
  {
  }
};

FileSelector::FileSelector()
  : Window(WithTitleBar, "")
  , m_navigationLocked(false)
{
  app::WidgetLoader loader;
  loader.addWidgetType("filenameentry", new CustomFileNameEntryCreator);

  // Load the main widget.
  Box* box = loader.loadWidgetT<Box>("file_selector.xml", "main");
  addChild(box);

  View* view;
  app::finder(this)
    >> "fileview_container" >> view
    >> "goback" >> m_goBack
    >> "goforward" >> m_goForward
    >> "goup" >> m_goUp
    >> "newfolder" >> m_newFolder
    >> "location" >> m_location
    >> "filetype" >> m_fileType
    >> "filename" >> m_fileName;

  m_goBack->setFocusStop(false);
  m_goForward->setFocusStop(false);
  m_goUp->setFocusStop(false);
  m_newFolder->setFocusStop(false);

  set_gfxicon_to_button(m_goBack,
                        PART_COMBOBOX_ARROW_LEFT,
                        PART_COMBOBOX_ARROW_LEFT_SELECTED,
                        PART_COMBOBOX_ARROW_LEFT_DISABLED,
                        JI_CENTER | JI_MIDDLE);
  set_gfxicon_to_button(m_goForward,
                        PART_COMBOBOX_ARROW_RIGHT,
                        PART_COMBOBOX_ARROW_RIGHT_SELECTED,
                        PART_COMBOBOX_ARROW_RIGHT_DISABLED,
                        JI_CENTER | JI_MIDDLE);
  set_gfxicon_to_button(m_goUp,
                        PART_COMBOBOX_ARROW_UP,
                        PART_COMBOBOX_ARROW_UP_SELECTED,
                        PART_COMBOBOX_ARROW_UP_DISABLED,
                        JI_CENTER | JI_MIDDLE);
  set_gfxicon_to_button(m_newFolder,
                        PART_NEWFOLDER,
                        PART_NEWFOLDER_SELECTED,
                        PART_NEWFOLDER,
                        JI_CENTER | JI_MIDDLE);

  setup_mini_look(m_goBack);
  setup_mini_look(m_goForward);
  setup_mini_look(m_goUp);
  setup_mini_look(m_newFolder);

  m_fileList = new FileList();
  m_fileList->setId("fileview");
  view->attachToView(m_fileList);
  m_fileName->setAssociatedFileList(m_fileList);

  m_goBack->Click.connect(Bind<void>(&FileSelector::onGoBack, this));
  m_goForward->Click.connect(Bind<void>(&FileSelector::onGoForward, this));
  m_goUp->Click.connect(Bind<void>(&FileSelector::onGoUp, this));
  m_newFolder->Click.connect(Bind<void>(&FileSelector::onNewFolder, this));
  m_location->CloseListBox.connect(Bind<void>(&FileSelector::onLocationCloseListBox, this));
  m_fileType->Change.connect(Bind<void>(&FileSelector::onFileTypeChange, this));
  m_fileList->FileSelected.connect(Bind<void>(&FileSelector::onFileListFileSelected, this));
  m_fileList->FileAccepted.connect(Bind<void>(&FileSelector::onFileListFileAccepted, this));
  m_fileList->CurrentFolderChanged.connect(Bind<void>(&FileSelector::onFileListCurrentFolderChanged, this));

  getManager()->addMessageFilter(kKeyDownMessage, this);
}

FileSelector::~FileSelector()
{
  getManager()->removeMessageFilter(kKeyDownMessage, this);
}

std::string FileSelector::show(const std::string& title,
  const std::string& initialPath,
  const std::string& showExtensions)
{
  std::string result;

  FileSystemModule* fs = FileSystemModule::instance();
  LockFS lock(fs);

  fs->refresh();

  if (!navigation_history) {
    navigation_history = new FileItemList();
    App::instance()->Exit.connect(&on_exit_delete_navigation_history);
  }

  // we have to find where the user should begin to browse files (start_folder)
  std::string start_folder_path;
  IFileItem* start_folder = NULL;

  // If initialPath doesn't contain a path.
  if (base::get_file_path(initialPath).empty()) {
    // Get the saved `path' in the configuration file.
    std::string path = get_config_string("FileSelect", "CurrentDirectory", "<empty>");
    if (path == "<empty>") {
      start_folder_path = base::get_user_docs_folder();
      path = base::join_path(start_folder_path, initialPath);
    }
    start_folder = fs->getFileItemFromPath(path);
  }
  else {
    // Remove the filename.
    start_folder_path = base::join_path(base::get_file_path(initialPath), "");
  }
  start_folder_path = base::fix_path_separators(start_folder_path);

  if (!start_folder)
    start_folder = fs->getFileItemFromPath(start_folder_path);

  PRINTF("start_folder_path = %s (%p)\n", start_folder_path.c_str(), start_folder);

  setMinSize(gfx::Size(ui::display_w()*9/10, ui::display_h()*9/10));
  remapWindow();
  centerWindow();

  m_fileList->setExtensions(showExtensions.c_str());
  if (start_folder)
    m_fileList->setCurrentFolder(start_folder);

  // current location
  navigation_position.reset();
  addInNavigationHistory(m_fileList->getCurrentFolder());

  // fill the location combo-box
  updateLocation();
  updateNavigationButtons();

  // fill file-type combo-box
  m_fileType->removeAllItems();

  std::vector<std::string> tokens;
  base::split_string(showExtensions, tokens, ",");
  for (const auto& tok : tokens)
    m_fileType->addItem(tok.c_str());

  // file name entry field
  m_fileName->setText(base::get_file_name(initialPath).c_str());
  selectFileTypeFromFileName();
  m_fileName->selectText(0, -1);

  // setup the title of the window
  setText(title.c_str());

  // get the ok-button
  Widget* ok = this->findChild("ok");

  // update the view
  View::getView(m_fileList)->updateView();

  // open the window and run... the user press ok?
again:
  openWindowInForeground();
  if (getKiller() == ok ||
      getKiller() == m_fileList) {
    // open the selected file
    IFileItem* folder = m_fileList->getCurrentFolder();
    ASSERT(folder);

    std::string fn = m_fileName->getText();
    std::string buf;
    IFileItem* enter_folder = NULL;

    // up a level?
    if (fn == "..") {
      enter_folder = folder->getParent();
      if (!enter_folder)
        enter_folder = folder;
    }
    else if (fn.empty()) {
      // show the window again
      setVisible(true);
      goto again;
    }
    else {
      // check if the user specified in "fn" a item of "fileview"
      const FileItemList& children = m_fileList->getFileList();

      std::string fn2 = fn;
#ifdef _WIN32
      fn2 = base::string_to_lower(fn2);
#endif

      for (IFileItem* child : children) {
        std::string child_name = child->getDisplayName();

#ifdef _WIN32
        child_name = base::string_to_lower(child_name);
#endif
        if (child_name == fn2) {
          enter_folder = child;
          buf = enter_folder->getFileName();
          break;
        }
      }

      if (!enter_folder) {
        // does the file-name entry have separators?
        if (base::is_path_separator(*fn.begin())) { // absolute path (UNIX style)
#ifdef _WIN32
          // get the drive of the current folder
          std::string drive = folder->getFileName();
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
#ifdef _WIN32
        // does the file-name entry have colon?
        else if (fn.find(':') != std::string::npos) { // absolute path on Windows
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
        enter_folder = fs->getFileItemFromPath(buf);
      }
    }

    // did we find a folder to enter?
    if (enter_folder &&
        enter_folder->isFolder() &&
        enter_folder->isBrowsable()) {
      // enter in the folder that was specified in the 'm_fileName'
      m_fileList->setCurrentFolder(enter_folder);

      // clear the text of the entry widget
      m_fileName->setText("");

      // show the window again
      setVisible(true);
      goto again;
    }
    // else file-name specified in the entry is really a file to open...

    // does it not have extension? ...we should add the extension
    // selected in the filetype combo-box
    if (base::get_file_extension(buf).empty()) {
      buf += '.';
      buf += m_fileType->getItemText(m_fileType->getSelectedItemIndex());
    }

    // duplicate the buffer to return a new string
    result = buf;

    // save the path in the configuration file
    std::string lastpath = folder->getKeyName();
    set_config_string("FileSelect", "CurrentDirectory",
                      lastpath.c_str());
  }

  return result;
}

bool FileSelector::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage:
      if (msg->ctrlPressed() || msg->cmdPressed()) {
        KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keyMsg->scancode();
        switch (scancode) {
          case kKeyUp: onGoUp(); return true;
          case kKeyLeft: onGoBack(); return true;
          case kKeyRight: onGoForward(); return true;
          case kKeyDown:
            if (m_fileList->getSelectedFileItem() &&
                m_fileList->getSelectedFileItem()->isBrowsable()) {
              m_fileList->setCurrentFolder(
                m_fileList->getSelectedFileItem());
            }
            return true;
        }
      }
      break;
  }
  return Window::onProcessMessage(msg);
}

// Updates the content of the combo-box that shows the current
// location in the file-system.
void FileSelector::updateLocation()
{
  IFileItem* currentFolder = m_fileList->getCurrentFolder();
  IFileItem* fileItem = currentFolder;
  std::list<IFileItem*> locations;
  int selected_index = -1;
  int newItem;

  while (fileItem != NULL) {
    locations.push_front(fileItem);
    fileItem = fileItem->getParent();
  }

  // Clear all the items from the combo-box
  m_location->removeAllItems();

  // Add item by item (from root to the specific current folder)
  int level = 0;
  for (std::list<IFileItem*>::iterator it=locations.begin(), end=locations.end();
       it != end; ++it) {
    fileItem = *it;

    // Indentation
    std::string buf;
    for (int c=0; c<level; ++c)
      buf += "  ";

    // Location name
    buf += fileItem->getDisplayName();

    // Add the new location to the combo-box
    m_location->addItem(new CustomFileNameItem(buf.c_str(), fileItem));

    if (fileItem == currentFolder)
      selected_index = level;

    level++;
  }

  // Add paths from recent files list
  {
    newItem = m_location->addItem("");
    newItem = m_location->addItem("-------- Recent Paths --------");

    RecentFiles::const_iterator it = App::instance()->getRecentFiles()->paths_begin();
    RecentFiles::const_iterator end = App::instance()->getRecentFiles()->paths_end();
    for (; it != end; ++it)
      m_location->addItem(new CustomFolderNameItem(it->c_str()));
  }

  // Select the location
  {
    m_location->setSelectedItemIndex(selected_index);
    m_location->getEntryWidget()->setText(currentFolder->getDisplayName().c_str());
    m_location->getEntryWidget()->deselectText();
  }
}

void FileSelector::updateNavigationButtons()
{
  // Update the state of the go back button: if the navigation-history
  // has two elements and the navigation-position isn't the first one.
  m_goBack->setEnabled(navigation_history->size() > 1 &&
                       (navigation_position.isNull() ||
                        navigation_position.getIterator() != navigation_history->begin()));

  // Update the state of the go forward button: if the
  // navigation-history has two elements and the navigation-position
  // isn't the last one.
  m_goForward->setEnabled(navigation_history->size() > 1 &&
                          (navigation_position.isNull() ||
                           navigation_position.getIterator() != navigation_history->end()-1));

  // Update the state of the go up button: if the current-folder isn't
  // the root-item.
  IFileItem* currentFolder = m_fileList->getCurrentFolder();
  m_goUp->setEnabled(currentFolder != FileSystemModule::instance()->getRootFileItem());
}

void FileSelector::addInNavigationHistory(IFileItem* folder)
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

void FileSelector::selectFileTypeFromFileName()
{
  std::string ext = base::get_file_extension(m_fileName->getText());

  if (!ext.empty()) {
    ext = base::string_to_lower(ext);
    m_fileType->setSelectedItemIndex(m_fileType->findItemIndex(ext.c_str()));
  }
}

void FileSelector::onGoBack()
{
  if (navigation_history->size() > 1) {
    if (navigation_position.isNull())
      navigation_position.setIterator(navigation_history->end()-1);

    if (navigation_position.getIterator() != navigation_history->begin()) {
      navigation_position.setIterator(navigation_position.getIterator()-1);

      m_navigationLocked = true;
      m_fileList->setCurrentFolder(*navigation_position.getIterator());
      m_navigationLocked = false;
    }
  }
}

void FileSelector::onGoForward()
{
  if (navigation_history->size() > 1) {
    if (navigation_position.isNull())
      navigation_position.setIterator(navigation_history->begin());

    if (navigation_position.getIterator() != navigation_history->end()-1) {
      navigation_position.setIterator(navigation_position.getIterator()+1);

      m_navigationLocked = true;
      m_fileList->setCurrentFolder(*navigation_position.getIterator());
      m_navigationLocked = false;
    }
  }
}

void FileSelector::onGoUp()
{
  m_fileList->goUp();
}

void FileSelector::onNewFolder()
{
  base::UniquePtr<Window> window(load_widget<Window>("file_selector.xml", "newfolder_dialog"));
  Button* ok;
  Entry* name;

  app::finder(window)
    >> "ok" >> ok
    >> "name" >> name;

  window->openWindowInForeground();
  if (window->getKiller() == ok) {
    IFileItem* currentFolder = m_fileList->getCurrentFolder();
    if (currentFolder) {
      std::string dirname = name->getText();

      // Create the new directory
      try {
        currentFolder->createDirectory(dirname);

        // Enter in the new folder
        for (FileItemList::const_iterator it=currentFolder->getChildren().begin(),
               end=currentFolder->getChildren().end(); it != end; ++it) {
          if ((*it)->getDisplayName() == dirname) {
            m_fileList->setCurrentFolder(*it);
            break;
          }
        }
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }
    }
  }
}

// Hook for the 'location' combo-box
void FileSelector::onLocationCloseListBox()
{
  // When the user change the location we have to set the
  // current-folder in the 'fileview' widget
  CustomFileNameItem* comboFileItem = dynamic_cast<CustomFileNameItem*>(m_location->getSelectedItem());
  IFileItem* fileItem = (comboFileItem != NULL ? comboFileItem->getFileItem(): NULL);

  // Maybe the user selected a recent file path
  if (fileItem == NULL) {
    CustomFolderNameItem* comboFolderItem =
      dynamic_cast<CustomFolderNameItem*>(m_location->getSelectedItem());

    if (comboFolderItem != NULL) {
      std::string path = comboFolderItem->getText();
      fileItem = FileSystemModule::instance()->getFileItemFromPath(path);
    }
  }

  if (fileItem != NULL) {
    m_fileList->setCurrentFolder(fileItem);

    // Refocus the 'fileview' (the focus in that widget is more
    // useful for the user)
    getManager()->setFocus(m_fileList);
  }
}

// When the user selects a new file-type (extension), we have to
// change the file-extension in the 'filename' entry widget
void FileSelector::onFileTypeChange()
{
  std::string newExtension = m_fileType->getItemText(m_fileType->getSelectedItemIndex());
  std::string fileName = m_fileName->getText();
  std::string currentExtension = base::get_file_extension(fileName);

  if (!currentExtension.empty()) {
    m_fileName->setText((fileName.substr(0, fileName.size()-currentExtension.size())+newExtension).c_str());
    m_fileName->selectAllText();
  }
}

void FileSelector::onFileListFileSelected()
{
  IFileItem* fileitem = m_fileList->getSelectedFileItem();

  if (!fileitem->isFolder()) {
    std::string filename = base::get_file_name(fileitem->getFileName());

    m_fileName->setText(filename.c_str());
    m_fileName->selectAllText();
    selectFileTypeFromFileName();
  }
}

void FileSelector::onFileListFileAccepted()
{
  closeWindow(m_fileList);
}

void FileSelector::onFileListCurrentFolderChanged()
{
  if (!m_navigationLocked)
    addInNavigationHistory(m_fileList->getCurrentFolder());

  updateLocation();
  updateNavigationButtons();
}

} // namespace app
