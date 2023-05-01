// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_menus.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/extensions.h"
#include "app/gui_xml.h"
#include "app/i18n/strings.h"
#include "app/recent_files.h"
#include "app/resource_finder.h"
#include "app/tools/tool_box.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui_context.h"
#include "app/util/filetoks.h"
#include "base/fs.h"
#include "base/string.h"
#include "fmt/format.h"
#include "os/menus.h"
#include "os/system.h"
#include "ui/ui.h"
#include "ver/info.h"

#include "tinyxml.h"

#include <cctype>
#include <cstring>
#include <string>
#include <algorithm>
#include <cstdlib>

#define MENUS_TRACE(...) // TRACEARGS

namespace app {

using namespace ui;

namespace {

// TODO Move this to "os" layer
const int kUnicodeEsc      = 27;
const int kUnicodeEnter    = '\r'; // 10
const int kUnicodeInsert   = 0xF727; // NSInsertFunctionKey
const int kUnicodeDel      = 0xF728; // NSDeleteFunctionKey
const int kUnicodeHome     = 0xF729; // NSHomeFunctionKey
const int kUnicodeEnd      = 0xF72B; // NSEndFunctionKey
const int kUnicodePageUp   = 0xF72C; // NSPageUpFunctionKey
const int kUnicodePageDown = 0xF72D; // NSPageDownFunctionKey
const int kUnicodeLeft     = 0xF702; // NSLeftArrowFunctionKey
const int kUnicodeRight    = 0xF703; // NSRightArrowFunctionKey
const int kUnicodeUp       = 0xF700; // NSUpArrowFunctionKey
const int kUnicodeDown     = 0xF701; // NSDownArrowFunctionKey

const char* kFileRecentListGroup = "file_recent_list";

void destroy_instance(AppMenus* instance)
{
  delete instance;
}

bool is_text_entry_shortcut(const os::Shortcut& shortcut)
{
  const os::KeyModifiers mod = shortcut.modifiers();
  const int chr = shortcut.unicode();
  const int lchr = std::tolower(chr);

  bool result =
    ((mod == os::KeyModifiers::kKeyNoneModifier ||
      mod == os::KeyModifiers::kKeyShiftModifier) &&
     chr >= 32 && chr < 0xF000)
  ||
    ((mod == os::KeyModifiers::kKeyCmdModifier ||
      mod == os::KeyModifiers::kKeyCtrlModifier) &&
     (lchr == 'a' || lchr == 'c' || lchr == 'v' || lchr == 'x'))
  ||
    (chr == kUnicodeInsert ||
     chr == kUnicodeDel ||
     chr == kUnicodeHome ||
     chr == kUnicodeEnd ||
     chr == kUnicodeLeft ||
     chr == kUnicodeRight ||
     chr == kUnicodeEsc ||
     chr == kUnicodeEnter);

  return result;
}

bool can_call_global_shortcut(const AppMenuItem::Native* native)
{
  ASSERT(native);

  ui::Manager* manager = ui::Manager::getDefault();
  ASSERT(manager);
  ui::Widget* focus = manager->getFocus();
  return
    // The mouse is not capture
    (manager->getCapture() == nullptr) &&
    // The foreground window must be the main window to avoid calling
    // a global command inside a modal dialog.
    (manager->getForegroundWindow() == App::instance()->mainWindow()) &&
    // If we are in a menubox window (e.g. we've pressed
    // Alt+mnemonic), we should disable the native shortcuts
    // temporarily so we can use mnemonics without modifiers
    // (e.g. Alt+S opens the Sprite menu, then 'S' key should execute
    // "Sprite Size" command in that menu, instead of Stroke command
    // which is in 'Edit > Stroke'). This is necessary in macOS, when
    // the native menu + Aseprite pixel-art menus are enabled.
    (dynamic_cast<MenuBoxWindow*>(manager->getTopWindow()) == nullptr) &&
    // The focused widget cannot be an entry, because entry fields
    // prefer text input, so we cannot call shortcuts without
    // modifiers (e.g. F or T keystrokes) to trigger a global command
    // in a text field.
    (focus == nullptr ||
     focus->type() != ui::kEntryWidget ||
     !is_text_entry_shortcut(native->shortcut)) &&
    (native->keyContext == KeyContext::Any ||
     native->keyContext == KeyboardShortcuts::instance()->getCurrentKeyContext());
}

// TODO this should be on "she" library (or we should use
// os::Shortcut instead of ui::Accelerators)
int from_scancode_to_unicode(KeyScancode scancode)
{
  static int map[] = {
    0, // kKeyNil
    'a', // kKeyA
    'b', // kKeyB
    'c', // kKeyC
    'd', // kKeyD
    'e', // kKeyE
    'f', // kKeyF
    'g', // kKeyG
    'h', // kKeyH
    'i', // kKeyI
    'j', // kKeyJ
    'k', // kKeyK
    'l', // kKeyL
    'm', // kKeyM
    'n', // kKeyN
    'o', // kKeyO
    'p', // kKeyP
    'q', // kKeyQ
    'r', // kKeyR
    's', // kKeyS
    't', // kKeyT
    'u', // kKeyU
    'v', // kKeyV
    'w', // kKeyW
    'x', // kKeyX
    'y', // kKeyY
    'z', // kKeyZ
    '0', // kKey0
    '1', // kKey1
    '2', // kKey2
    '3', // kKey3
    '4', // kKey4
    '5', // kKey5
    '6', // kKey6
    '7', // kKey7
    '8', // kKey8
    '9', // kKey9
    0, // kKey0Pad
    0, // kKey1Pad
    0, // kKey2Pad
    0, // kKey3Pad
    0, // kKey4Pad
    0, // kKey5Pad
    0, // kKey6Pad
    0, // kKey7Pad
    0, // kKey8Pad
    0, // kKey9Pad
    0xF704, // kKeyF1 (NSF1FunctionKey)
    0xF705, // kKeyF2
    0xF706, // kKeyF3
    0xF707, // kKeyF4
    0xF708, // kKeyF5
    0xF709, // kKeyF6
    0xF70A, // kKeyF7
    0xF70B, // kKeyF8
    0xF70C, // kKeyF9
    0xF70D, // kKeyF10
    0xF70E, // kKeyF11
    0xF70F, // kKeyF12
    kUnicodeEsc, // kKeyEsc
    '~', // kKeyTilde
    '-', // kKeyMinus
    '=', // kKeyEquals
    8, // kKeyBackspace
    9, // kKeyTab
    '[', // kKeyOpenbrace
    ']', // kKeyClosebrace
    kUnicodeEnter, // kKeyEnter
    ':', // kKeyColon
    '\'', // kKeyQuote
    '\\', // kKeyBackslash
    0, // kKeyBackslash2
    ',', // kKeyComma
    '.', // kKeyStop
    '/', // kKeySlash
    ' ', // kKeySpace
    kUnicodeInsert, // kKeyInsert (NSInsertFunctionKey)
    kUnicodeDel, // kKeyDel (NSDeleteFunctionKey)
    kUnicodeHome, // kKeyHome (NSHomeFunctionKey)
    kUnicodeEnd, // kKeyEnd (NSEndFunctionKey)
    kUnicodePageUp, // kKeyPageUp (NSPageUpFunctionKey)
    kUnicodePageDown, // kKeyPageDown (NSPageDownFunctionKey)
    kUnicodeLeft, // kKeyLeft (NSLeftArrowFunctionKey)
    kUnicodeRight, // kKeyRight (NSRightArrowFunctionKey)
    kUnicodeUp, // kKeyUp (NSUpArrowFunctionKey)
    kUnicodeDown, // kKeyDown (NSDownArrowFunctionKey)
    '/', // kKeySlashPad
    '*', // kKeyAsterisk
    0, // kKeyMinusPad
    0, // kKeyPlusPad
    0, // kKeyDelPad
    0, // kKeyEnterPad
    0, // kKeyPrtscr
    0, // kKeyPause
    0, // kKeyAbntC1
    0, // kKeyYen
    0, // kKeyKana
    0, // kKeyConvert
    0, // kKeyNoconvert
    0, // kKeyAt
    0, // kKeyCircumflex
    0, // kKeyColon2
    0, // kKeyKanji
    0, // kKeyEqualsPad
    '`', // kKeyBackquote
    0, // kKeySemicolon
    0, // kKeyUnknown1
    0, // kKeyUnknown2
    0, // kKeyUnknown3
    0, // kKeyUnknown4
    0, // kKeyUnknown5
    0, // kKeyUnknown6
    0, // kKeyUnknown7
    0, // kKeyUnknown8
    0, // kKeyLShift
    0, // kKeyRShift
    0, // kKeyLControl
    0, // kKeyRControl
    0, // kKeyAlt
    0, // kKeyAltGr
    0, // kKeyLWin
    0, // kKeyRWin
    0, // kKeyMenu
    0, // kKeyCommand
    0, // kKeyScrLock
    0, // kKeyNumLock
    0, // kKeyCapsLock
  };
  if (scancode >= 0 && scancode < sizeof(map) / sizeof(map[0]))
    return map[scancode];
  else
    return 0;
}

AppMenuItem::Native get_native_shortcut_for_command(
  const char* commandId,
  const Params& params = Params())
{
  AppMenuItem::Native native;
  KeyPtr key = KeyboardShortcuts::instance()->command(commandId, params);
  if (key) {
    native.shortcut = get_os_shortcut_from_key(key.get());
    native.keyContext = key->keycontext();
  }
  return native;
}

void destroy_menu_item(ui::Widget* item)
{
  if (item->parent())
    item->parent()->removeChild(item);

  if (auto appItem = dynamic_cast<AppMenuItem*>(item)) {
    if (appItem)
      appItem->disposeNative();
  }

  item->deferDelete();
}

} // anonymous namespace

os::Shortcut get_os_shortcut_from_key(const Key* key)
{
  if (key && !key->accels().empty()) {
    const ui::Accelerator& accel = key->accels().front();

#ifdef __APPLE__
    // Shortcuts with spacebar as modifier do not work well in macOS
    // (they will be called when the space bar is unpressed too).
    if ((accel.modifiers() & ui::kKeySpaceModifier) == ui::kKeySpaceModifier)
      return os::Shortcut();
#endif

    return os::Shortcut(
      (accel.unicodeChar() ? accel.unicodeChar():
                             from_scancode_to_unicode(accel.scancode())),
      accel.modifiers());
  }
  else
    return os::Shortcut();
}

// static
AppMenus* AppMenus::instance()
{
  static AppMenus* instance = NULL;
  if (!instance) {
    instance = new AppMenus;
    App::instance()->Exit.connect([]{ destroy_instance(instance); });
  }
  return instance;
}

AppMenus::AppMenus()
  : m_recentFilesPlaceholder(nullptr)
  , m_osMenu(nullptr)
{
  m_recentFilesConn =
    App::instance()->recentFiles()->Changed.connect(
      [this]{ rebuildRecentList(); });
}

void AppMenus::reload()
{
  MENUS_TRACE("MENUS: AppMenus::reload()");

  XmlDocumentRef doc(GuiXml::instance()->doc());
  TiXmlHandle handle(doc.get());
  const char* path = GuiXml::instance()->filename();

  ////////////////////////////////////////
  // Remove all menu items added to groups from recent files and
  // scripts so we can re-add them later in the new menus.

  for (auto& it : m_groups) {
    GroupInfo& group = it.second;
    MENUS_TRACE("MENUS: - groups", it.first, "with", group.items.size(), "item(s)");

    for (auto& item : group.items)
      group.menu->removeChild(item);

    // These values will be restored later
    group.end = nullptr;
  }

  ////////////////////////////////////////
  // Load menus

  LOG("MENU: Loading menus from %s\n", path);

  m_rootMenu.reset(loadMenuById(handle, "main_menu"));

  LOG("MENU: Main menu loaded.\n");

  m_tabPopupMenu.reset(loadMenuById(handle, "tab_popup_menu"));
  m_documentTabPopupMenu.reset(loadMenuById(handle, "document_tab_popup_menu"));
  m_layerPopupMenu.reset(loadMenuById(handle, "layer_popup_menu"));
  m_framePopupMenu.reset(loadMenuById(handle, "frame_popup_menu"));
  m_celPopupMenu.reset(loadMenuById(handle, "cel_popup_menu"));
  m_celMovementPopupMenu.reset(loadMenuById(handle, "cel_movement_popup_menu"));
  m_tagPopupMenu.reset(loadMenuById(handle, "tag_popup_menu"));
  m_slicePopupMenu.reset(loadMenuById(handle, "slice_popup_menu"));
  m_palettePopupMenu.reset(loadMenuById(handle, "palette_popup_menu"));
  m_inkPopupMenu.reset(loadMenuById(handle, "ink_popup_menu"));

  // Add one menu item to run each script from the user scripts/ folder
  {
    MenuItem* scriptsMenu = dynamic_cast<MenuItem*>(
      m_rootMenu->findItemById("scripts_menu"));
#ifdef ENABLE_SCRIPTING
    // Load scripts
    ResourceFinder rf;
    rf.includeUserDir("scripts/.");
    std::string scriptsDir = rf.getFirstOrCreateDefault();
    scriptsDir = base::get_file_path(scriptsDir);
    if (base::is_directory(scriptsDir)) {
      loadScriptsSubmenu(scriptsMenu->getSubmenu(), scriptsDir, true);
    }
#else
    // Scripting is not available
    if (scriptsMenu) {
      delete scriptsMenu;
      delete m_rootMenu->findItemById("scripts_menu_separator");

      // Remove scripts group
      auto it = m_groups.find("file_scripts");
      if (it != m_groups.end())
        m_groups.erase(it);
    }
#endif
  }

  // Remove the "Enter license" menu item when DRM is not enabled.
#ifndef ENABLE_DRM
  if (auto helpMenuItem = m_rootMenu->findItemById("help_menu")) {
    if (Menu* helpMenu = dynamic_cast<MenuItem*>(helpMenuItem)->getSubmenu()) {
      delete helpMenu->findChild("enter_license_separator");
      delete helpMenu->findChild("enter_license");
    }

    auto it = m_groups.find("help_enter_license");
    if (it != m_groups.end())
      m_groups.erase(it);
  }
#endif

  ////////////////////////////////////////
  // Re-add menu items in groups (recent files & scripts)

  for (auto& it : m_groups) {
    GroupInfo& group = it.second;
    if (group.menu) {
      MENUS_TRACE("MENUS: - re-adding group ", it.first, "with", group.items.size(), "item(s)");

      auto menu = group.menu;
      int insertIndex = menu->getChildIndex(group.end);
      if (insertIndex < 0)
        insertIndex = -1;
      for (auto& item : group.items) {
        menu->insertChild(++insertIndex, item);
        group.end = item;
      }
    }
    // Delete items that don't have a group now
    else {
      MENUS_TRACE("MENUS: - deleting group ", it.first, "with", group.items.size(), "item(s)");
      for (auto& item : group.items)
        item->deferDelete();
      group.items.clear();
    }
  }

  ////////////////////////////////////////
  // Load keyboard shortcuts for commands

  LOG("MENU: Loading commands keyboard shortcuts from %s\n", path);

  TiXmlElement* xmlKey = handle
    .FirstChild("gui")
    .FirstChild("keyboard").ToElement();

  // From a fresh start, load the default keys
  KeyboardShortcuts::instance()->clear();
  KeyboardShortcuts::instance()->importFile(xmlKey, KeySource::Original);

  // Load extension-defined keys
  for (const Extension* ext : App::instance()->extensions()) {
    if (ext->isEnabled() &&
        ext->hasKeys()) {
      for (const auto& kv : ext->keys()) {
        KeyboardShortcuts::instance()->importFile(
          kv.second, KeySource::ExtensionDefined);
      }
    }
  }

  // Load user-defined keys
  {
    ResourceFinder rf;
    rf.includeUserDir("user.aseprite-keys");
    std::string fn = rf.getFirstOrCreateDefault();
    if (base::is_file(fn))
      KeyboardShortcuts::instance()->importFile(fn, KeySource::UserDefined);
  }

  // Create native menus after the default + user defined keyboard
  // shortcuts are loaded correctly.
  createNativeMenus();
}

#ifdef ENABLE_SCRIPTING
void AppMenus::loadScriptsSubmenu(ui::Menu* menu,
                                  const std::string& dir,
                                  const bool rootLevel)
{
  auto files = base::list_files(dir);
  std::sort(files.begin(), files.end(),
            [](const std::string& a, const std::string& b) {
              return base::compare_filenames(a, b) < 0;
            });
  int insertPos = 0;
  for (auto fn : files) {
    std::string fullFn = base::join_path(dir, fn);
    AppMenuItem* menuitem = nullptr;

    if (fn[0] == '.') // Ignore all files and directories that start with a dot
      continue;

    if (base::is_file(fullFn)) {
      if (base::string_to_lower(base::get_file_extension(fn)) == "lua") {
        Params params;
        params.set("filename", fullFn.c_str());
        menuitem = new AppMenuItem(
          base::get_file_title(fn).c_str(),
          CommandId::RunScript(),
          params);
      }
    }
    else if (base::is_directory(fullFn)) {
      Menu* submenu = new Menu();
      loadScriptsSubmenu(submenu, fullFn, false);

      menuitem = new AppMenuItem(
        base::get_file_title(fn).c_str());
      menuitem->setSubmenu(submenu);
    }
    if (menuitem) {
      menu->insertChild(insertPos++, menuitem);
    }
  }
  if (rootLevel && insertPos > 0)
    menu->insertChild(insertPos, new MenuSeparator());
}
#endif

void AppMenus::initTheme()
{
  updateMenusList();
  for (Menu* menu : m_menus)
    if (menu)
      menu->initTheme();
}

bool AppMenus::rebuildRecentList()
{
  MENUS_TRACE("MENUS: AppMenus::rebuildRecentList m_recentFilesPlaceholder=", m_recentFilesPlaceholder);

  if (!m_recentFilesPlaceholder)
    return true;

  Menu* menu = dynamic_cast<Menu*>(m_recentFilesPlaceholder->parent());
  if (!menu)
    return false;

  AppMenuItem* owner = dynamic_cast<AppMenuItem*>(menu->getOwnerMenuItem());
  if (!owner || owner->hasSubmenuOpened())
    return false;

  // Remove active items
  for (auto item : m_recentMenuItems)
    removeMenuItemFromGroup(item);
  m_recentMenuItems.clear();

  auto recent = App::instance()->recentFiles();
  base::paths files;
  files.insert(files.end(),
               recent->pinnedFiles().begin(),
               recent->pinnedFiles().end());
  files.insert(files.end(),
               recent->recentFiles().begin(),
               recent->recentFiles().end());
  if (!files.empty()) {
    Params params;
    for (const auto& fn : files) {
      params.set("filename", fn.c_str());

      std::unique_ptr<AppMenuItem> menuitem(
        new AppMenuItem(base::get_file_name(fn).c_str(),
                        CommandId::OpenFile(),
                        params));
      menuitem->setIsRecentFileItem(true);

      m_recentMenuItems.push_back(menuitem.get());
      addMenuItemIntoGroup(kFileRecentListGroup, std::move(menuitem));
    }
  }
  else {
      std::unique_ptr<AppMenuItem> menuitem(
        new AppMenuItem(
          Strings::main_menu_file_no_recent_file()));
    menuitem->setIsRecentFileItem(true);
    menuitem->setEnabled(false);

    m_recentMenuItems.push_back(menuitem.get());
    addMenuItemIntoGroup(kFileRecentListGroup, std::move(menuitem));
  }

  // Sync native menus
  if (owner->native() &&
      owner->native()->menuItem) {
    auto menus = os::instance()->menus();
    os::MenuRef osMenu = (menus ? menus->makeMenu(): nullptr);
    if (osMenu) {
      createNativeSubmenus(osMenu.get(), menu);
      owner->native()->menuItem->setSubmenu(osMenu);
    }
  }

  return true;
}

Menu* AppMenus::getAnimationMenu()
{
  auto menuItem =
    dynamic_cast<MenuItem*>(m_rootMenu->findItemById("animation_menu"));
  if (menuItem)
    return menuItem->getSubmenu();
  else
    return nullptr;
}

void AppMenus::addMenuGroup(const std::string& groupId,
                            MenuItem* menuItem)
{
  GroupInfo& group = m_groups[groupId];
  ASSERT(group.menu == nullptr);
  ASSERT(group.end == nullptr);
  group.menu = menuItem->getSubmenu();
  group.end = nullptr;
}

void AppMenus::removeMenuGroup(const std::string& groupId)
{
  auto it = m_groups.find(groupId);
  if (it != m_groups.end()) {
    GroupInfo& group = it->second;

    ASSERT(group.items.empty()); // To remove a group, the group must be empty

    if (group.menu->getOwnerMenuItem()) {
      ui::MenuItem* item = group.menu->getOwnerMenuItem();
      removeMenuItemFromGroup(
        [item](Widget* i){
          return item == i;
        });
    }
    m_groups.erase(it);
  }
}

void AppMenus::addMenuItemIntoGroup(const std::string& groupId,
                                    std::unique_ptr<Widget>&& menuItem)
{
  auto it = m_groups.find(groupId);
  if (it == m_groups.end()) {
    LOG(ERROR, "MENU: An extension tried to add a command (%s) in a non-existent group (%s)\n",
        menuItem->text().c_str(), groupId.c_str());
    menuItem.release();
    return;
  }

  GroupInfo& group = it->second;
  Menu* menu = group.menu;
  ASSERT(menu);

  if (group.end) {
    int insertIndex = menu->getChildIndex(group.end);
    ASSERT(insertIndex >= 0);
    menu->insertChild(insertIndex+1, menuItem.get());
  }
  else {
    menu->addChild(menuItem.get());
  }

  group.end = menuItem.get();
  group.items.push_back(menuItem.get());

  menuItem.release();
}

template<typename Pred>
void AppMenus::removeMenuItemFromGroup(Pred pred)
{
  for (auto& it : m_groups) {
    GroupInfo& group = it.second;
    for (auto it=group.items.begin(); it != group.items.end(); ) {
      auto& item = *it;
      if (pred(item)) {
        if (item == group.end)
          group.end = group.end->previousSibling();

        destroy_menu_item(item);

        it = group.items.erase(it);
      }
      else {
        ++it;
      }
    }
  }
}

void AppMenus::removeMenuItemFromGroup(Command* cmd)
{
  removeMenuItemFromGroup(
    [cmd](Widget* item){
      auto appMenuItem = dynamic_cast<AppMenuItem*>(item);
      return (appMenuItem && appMenuItem->getCommand() == cmd);
    });
}

void AppMenus::removeMenuItemFromGroup(Widget* menuItem)
{
  removeMenuItemFromGroup(
    [menuItem](Widget* item){
      return (item == menuItem);
    });
}

Menu* AppMenus::loadMenuById(TiXmlHandle& handle, const char* id)
{
  ASSERT(id != NULL);

  // <gui><menus><menu>
  TiXmlElement* xmlMenu = handle
    .FirstChild("gui")
    .FirstChild("menus")
    .FirstChild("menu").ToElement();
  while (xmlMenu) {
    const char* menuId = xmlMenu->Attribute("id");

    if (menuId && strcmp(menuId, id) == 0) {
      m_xmlTranslator.setStringIdPrefix(menuId);
      return convertXmlelemToMenu(xmlMenu);
    }

    xmlMenu = xmlMenu->NextSiblingElement();
  }

  throw base::Exception("Error loading menu '%s'\nReinstall the application.", id);
}

Menu* AppMenus::convertXmlelemToMenu(TiXmlElement* elem)
{
  Menu* menu = new Menu();
  menu->setText(m_xmlTranslator(elem, "text"));

  TiXmlElement* child = elem->FirstChildElement();
  while (child) {
    Widget* menuitem = convertXmlelemToMenuitem(child, menu);
    if (menuitem)
      menu->addChild(menuitem);
    else
      throw base::Exception("Error converting the element \"%s\" to a menu-item.\n",
                            static_cast<const char*>(child->Value()));

    child = child->NextSiblingElement();
  }

  return menu;
}

Widget* AppMenus::convertXmlelemToMenuitem(TiXmlElement* elem, Menu* menu)
{
  const char* id = elem->Attribute("id");
  const char* group = elem->Attribute("group");
  const char* standard = elem->Attribute("standard");

  // is it a <separator>?
  if (strcmp(elem->Value(), "separator") == 0) {
    auto item = new MenuSeparator;
    if (id) {
      item->setId(id);

      // Recent list menu
      if (std::strcmp(id, "recent_files_placeholder") == 0) {
        m_recentFilesPlaceholder = item;
      }
    }
    if (group) {
      m_groups[group].menu = menu;
      m_groups[group].end = item;
    }
    return item;
  }

  const char* commandId = elem->Attribute("command");
  Command* command =
    (commandId ? Commands::instance()->byId(commandId):
                 nullptr);

  // load params
  Params params;
  if (command) {
    TiXmlElement* xmlParam = elem->FirstChildElement("param");
    while (xmlParam) {
      const char* param_name = xmlParam->Attribute("name");
      const char* param_value = xmlParam->Attribute("value");

      if (param_name && param_value)
        params.set(param_name, param_value);

      xmlParam = xmlParam->NextSiblingElement();
    }
  }

  // Create the item
  AppMenuItem* menuitem = new AppMenuItem(m_xmlTranslator(elem, "text"),
                                          (command ? command->id(): ""),
                                          params);
  if (!menuitem)
    return nullptr;

  // Get menu item text from command friendly name
  if (command && menuitem->text().empty()) {
    command->loadParams(params);
    menuitem->setText(command->friendlyName());
  }
  // If the menu item has a specific text, process its mnemonic
  else {
    menuitem->processMnemonicFromText();
  }

  if (id) menuitem->setId(id);
  if (group) {
    m_groups[group].menu = menu;
    m_groups[group].end = menuitem;
  }

  if (standard && strcmp(standard, "edit") == 0)
    menuitem->setStandardEditMenu();

  // Has it a ID?
  if (id) {
    if (std::strcmp(id, "help_menu") == 0) {
      m_helpMenuitem = menuitem;
    }
  }

  // Has it a sub-menu (<menu>)?
  if (strcmp(elem->Value(), "menu") == 0) {
    // Create the sub-menu
    Menu* subMenu = convertXmlelemToMenu(elem);
    if (!subMenu)
      throw base::Exception("Error reading the sub-menu\n");

    menuitem->setSubmenu(subMenu);
  }

  return menuitem;
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Command* command,
                                                   const Params& params,
                                                   const KeyPtr& key)
{
  updateMenusList();
  for (Menu* menu : m_menus)
    if (menu)
      applyShortcutToMenuitemsWithCommand(menu, command, params, key);
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Menu* menu,
                                                   Command* command,
                                                   const Params& params,
                                                   const KeyPtr& key)
{
  for (auto child : menu->children()) {
    if (child->type() == kMenuItemWidget) {
      AppMenuItem* menuitem = dynamic_cast<AppMenuItem*>(child);
      if (!menuitem)
        continue;

      const std::string& mi_commandId = menuitem->getCommandId();
      const Params& mi_params = menuitem->getParams();

      if ((base::utf8_icmp(mi_commandId, command->id()) == 0) &&
          (mi_params == params)) {
        // Set the keyboard shortcut to be shown in this menu-item
        menuitem->setKey(key);
      }

      if (Menu* submenu = menuitem->getSubmenu())
        applyShortcutToMenuitemsWithCommand(submenu, command, params, key);
    }
  }
}

void AppMenus::syncNativeMenuItemKeyShortcuts()
{
  syncNativeMenuItemKeyShortcuts(m_rootMenu.get());
}

void AppMenus::syncNativeMenuItemKeyShortcuts(Menu* menu)
{
  for (auto child : menu->children()) {
    if (child->type() == kMenuItemWidget) {
      if (AppMenuItem* menuitem = dynamic_cast<AppMenuItem*>(child))
        menuitem->syncNativeMenuItemKeyShortcut();

      if (Menu* submenu = static_cast<MenuItem*>(child)->getSubmenu())
        syncNativeMenuItemKeyShortcuts(submenu);
    }
  }
}

// TODO redesign the list of popup menus, it might be an
//      autogenerated widget from 'gen'
void AppMenus::updateMenusList()
{
  m_menus.clear();
  m_menus.push_back(m_rootMenu.get());
  m_menus.push_back(m_tabPopupMenu.get());
  m_menus.push_back(m_documentTabPopupMenu.get());
  m_menus.push_back(m_layerPopupMenu.get());
  m_menus.push_back(m_framePopupMenu.get());
  m_menus.push_back(m_celPopupMenu.get());
  m_menus.push_back(m_celMovementPopupMenu.get());
  m_menus.push_back(m_tagPopupMenu.get());
  m_menus.push_back(m_slicePopupMenu.get());
  m_menus.push_back(m_palettePopupMenu.get());
  m_menus.push_back(m_inkPopupMenu.get());
}

void AppMenus::createNativeMenus()
{
  os::Menus* menus = os::instance()->menus();
  if (!menus)       // This platform doesn't support native menu items
    return;

  // Save a reference to the old menu to avoid destroying it.
  os::MenuRef oldOSMenu = m_osMenu;
  m_osMenu = menus->makeMenu();

#ifdef __APPLE__ // Create default macOS app menus (App ... Window)
  {
    os::MenuItemInfo about(fmt::format("About {}", get_app_name()));
    auto native = get_native_shortcut_for_command(CommandId::About());
    about.shortcut = native.shortcut;
    about.execute = [native]{
      if (can_call_global_shortcut(&native)) {
        Command* cmd = Commands::instance()->byId(CommandId::About());
        UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
      }
    };
    about.validate = [native](os::MenuItem* item){
      item->setEnabled(can_call_global_shortcut(&native));
    };

    os::MenuItemInfo preferences("Preferences...");
    native = get_native_shortcut_for_command(CommandId::Options());
    preferences.shortcut = native.shortcut;
    preferences.execute = [native]{
      if (can_call_global_shortcut(&native)) {
        Command* cmd = Commands::instance()->byId(CommandId::Options());
        UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
      }
    };
    preferences.validate = [native](os::MenuItem* item){
      item->setEnabled(can_call_global_shortcut(&native));
    };

    os::MenuItemInfo hide(fmt::format("Hide {}", get_app_name()), os::MenuItemInfo::Hide);
    hide.shortcut = os::Shortcut('h', os::kKeyCmdModifier);

    os::MenuItemInfo quit(fmt::format("Quit {}", get_app_name()), os::MenuItemInfo::Quit);
    quit.shortcut = os::Shortcut('q', os::kKeyCmdModifier);

    os::MenuRef appMenu = menus->makeMenu();
    appMenu->addItem(menus->makeMenuItem(about));
    appMenu->addItem(menus->makeMenuItem(os::MenuItemInfo(os::MenuItemInfo::Separator)));
    appMenu->addItem(menus->makeMenuItem(preferences));
    appMenu->addItem(menus->makeMenuItem(os::MenuItemInfo(os::MenuItemInfo::Separator)));
    appMenu->addItem(menus->makeMenuItem(hide));
    appMenu->addItem(menus->makeMenuItem(os::MenuItemInfo("Hide Others", os::MenuItemInfo::HideOthers)));
    appMenu->addItem(menus->makeMenuItem(os::MenuItemInfo("Show All", os::MenuItemInfo::ShowAll)));
    appMenu->addItem(menus->makeMenuItem(os::MenuItemInfo(os::MenuItemInfo::Separator)));
    appMenu->addItem(menus->makeMenuItem(quit));

    os::MenuItemRef appItem = menus->makeMenuItem(os::MenuItemInfo("App"));
    appItem->setSubmenu(appMenu);
    m_osMenu->addItem(appItem);
  }
#endif

  createNativeSubmenus(m_osMenu.get(), m_rootMenu.get());

#ifdef __APPLE__
  {
    // Search the index where help menu is located (so the Window menu
    // can take its place/index position)
    int i = 0, helpIndex = int(m_rootMenu->children().size());
    for (const auto child : m_rootMenu->children()) {
      if (child == m_helpMenuitem) {
        helpIndex = i;
        break;
      }
      ++i;
    }

    os::MenuItemInfo minimize("Minimize", os::MenuItemInfo::Minimize);
    minimize.shortcut = os::Shortcut('m', os::kKeyCmdModifier);

    os::MenuRef windowMenu = menus->makeMenu();
    windowMenu->addItem(menus->makeMenuItem(minimize));
    windowMenu->addItem(menus->makeMenuItem(os::MenuItemInfo("Zoom", os::MenuItemInfo::Zoom)));

    os::MenuItemRef windowItem = menus->makeMenuItem(os::MenuItemInfo("Window"));
    windowItem->setSubmenu(windowMenu);

    // We use helpIndex+1 because the first index in m_osMenu is the
    // App menu.
    m_osMenu->insertItem(helpIndex+1, windowItem);
  }
#endif

  menus->setAppMenu(m_osMenu);
  if (oldOSMenu)
    oldOSMenu.reset();
}

void AppMenus::createNativeSubmenus(os::Menu* osMenu,
                                    const ui::Menu* uiMenu)
{
  os::Menus* menus = os::instance()->menus();

  for (const auto& child : uiMenu->children()) {
    os::MenuItemInfo info;
    AppMenuItem* appMenuItem = dynamic_cast<AppMenuItem*>(child);
    AppMenuItem::Native native;

    if (child->type() == kSeparatorWidget) {
      info.type = os::MenuItemInfo::Separator;
    }
    else if (child->type() == kMenuItemWidget) {
      if (appMenuItem &&
          appMenuItem->getCommand()) {
        native = get_native_shortcut_for_command(
          appMenuItem->getCommandId().c_str(),
          appMenuItem->getParams());
      }

      info.type = os::MenuItemInfo::Normal;
      info.text = child->text();
      info.shortcut = native.shortcut;
      info.execute = [appMenuItem]{
        if (can_call_global_shortcut(appMenuItem->native()))
          appMenuItem->executeClick();
      };
      info.validate = [appMenuItem](os::MenuItem* osItem) {
        if (can_call_global_shortcut(appMenuItem->native())) {
          appMenuItem->validateItem();
          osItem->setEnabled(appMenuItem->isEnabled());
          osItem->setChecked(appMenuItem->isSelected());
        }
        else {
          // Disable item when there are a modal window
          osItem->setEnabled(false);
        }
      };
    }
    else {
      ASSERT(false);            // Unsupported menu item type
      continue;
    }

    os::MenuItemRef osItem = menus->makeMenuItem(info);
    if (osItem) {
      osMenu->addItem(osItem);
      if (appMenuItem) {
        native.menuItem = osItem;
        appMenuItem->setNative(native);

        // Set this menu item as the standard "Edit" item for macOS
        if (appMenuItem->isStandardEditMenu())
          osItem->setAsStandardEditMenuItem();
      }

      if (child->type() == ui::kMenuItemWidget &&
          ((ui::MenuItem*)child)->hasSubmenu()) {
        os::MenuRef osSubmenu = menus->makeMenu();
        createNativeSubmenus(osSubmenu.get(), ((ui::MenuItem*)child)->getSubmenu());
        osItem->setSubmenu(osSubmenu);
      }
    }
  }
}

} // namespace app
