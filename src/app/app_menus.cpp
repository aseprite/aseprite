// Aseprite
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
#include "base/bind.h"
#include "base/fs.h"
#include "base/string.h"
#include "she/menus.h"
#include "she/system.h"
#include "ui/ui.h"

#include "tinyxml.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace app {

using namespace ui;

namespace {

// TODO Move this to she layer
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

void destroy_instance(AppMenus* instance)
{
  delete instance;
}

bool is_text_entry_shortcut(const she::Shortcut& shortcut)
{
  const she::KeyModifiers mod = shortcut.modifiers();
  const int chr = shortcut.unicode();
  const int lchr = std::tolower(chr);

  bool result =
    ((mod == she::KeyModifiers::kKeyNoneModifier ||
      mod == she::KeyModifiers::kKeyShiftModifier) &&
     chr >= 32 && chr < 0xF000)
  ||
    ((mod == she::KeyModifiers::kKeyCmdModifier ||
      mod == she::KeyModifiers::kKeyCtrlModifier) &&
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
// she::Shortcut instead of ui::Accelerators)
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
  Key* key = KeyboardShortcuts::instance()->command(commandId, params);
  if (key) {
    native.shortcut = get_os_shortcut_from_key(key);
    native.keyContext = key->keycontext();
  }
  return native;
}

} // anonymous namespace

she::Shortcut get_os_shortcut_from_key(Key* key)
{
  if (key && !key->accels().empty()) {
    const ui::Accelerator& accel = key->accels().front();
    return she::Shortcut(
      (accel.unicodeChar() ? accel.unicodeChar():
                             from_scancode_to_unicode(accel.scancode())),
      accel.modifiers());
  }
  else
    return she::Shortcut();
}

// static
AppMenus* AppMenus::instance()
{
  static AppMenus* instance = NULL;
  if (!instance) {
    instance = new AppMenus;
    App::instance()->Exit.connect(base::Bind<void>(&destroy_instance, instance));
  }
  return instance;
}

AppMenus::AppMenus()
  : m_recentListMenuitem(nullptr)
  , m_osMenu(nullptr)
{
  m_recentFilesConn =
    App::instance()->recentFiles()->Changed.connect(
      base::Bind(&AppMenus::rebuildRecentList, this));
}

AppMenus::~AppMenus()
{
  if (m_osMenu)
    m_osMenu->dispose();
}

void AppMenus::reload()
{
  XmlDocumentRef doc(GuiXml::instance()->doc());
  TiXmlHandle handle(doc.get());
  const char* path = GuiXml::instance()->filename();

  ////////////////////////////////////////
  // Load menus

  LOG("MENU: Loading menus from %s\n", path);

  m_rootMenu.reset(loadMenuById(handle, "main_menu"));

#if _DEBUG
  // Add a warning element because the user is not using the last well-known gui.xml file.
  if (GuiXml::instance()->version() != VERSION)
    m_rootMenu->insertChild(0, createInvalidVersionMenuitem());
#endif

  LOG("MENU: Main menu loaded.\n");

  m_tabPopupMenu.reset(loadMenuById(handle, "tab_popup_menu"));
  m_documentTabPopupMenu.reset(loadMenuById(handle, "document_tab_popup_menu"));
  m_layerPopupMenu.reset(loadMenuById(handle, "layer_popup_menu"));
  m_framePopupMenu.reset(loadMenuById(handle, "frame_popup_menu"));
  m_celPopupMenu.reset(loadMenuById(handle, "cel_popup_menu"));
  m_celMovementPopupMenu.reset(loadMenuById(handle, "cel_movement_popup_menu"));
  m_frameTagPopupMenu.reset(loadMenuById(handle, "frame_tag_popup_menu"));
  m_slicePopupMenu.reset(loadMenuById(handle, "slice_popup_menu"));
  m_palettePopupMenu.reset(loadMenuById(handle, "palette_popup_menu"));
  m_inkPopupMenu.reset(loadMenuById(handle, "ink_popup_menu"));

  ////////////////////////////////////////
  // Load keyboard shortcuts for commands

  LOG("MENU: Loading commands keyboard shortcuts from %s\n", path);

  TiXmlElement* xmlKey = handle
    .FirstChild("gui")
    .FirstChild("keyboard").ToElement();

  KeyboardShortcuts::instance()->clear();
  KeyboardShortcuts::instance()->importFile(xmlKey, KeySource::Original);

  // Load user settings
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

void AppMenus::initTheme()
{
  updateMenusList();
  for (Menu* menu : m_menus)
    if (menu)
      menu->initTheme();
}

bool AppMenus::rebuildRecentList()
{
  AppMenuItem* list_menuitem = dynamic_cast<AppMenuItem*>(m_recentListMenuitem);
  MenuItem* menuitem;

  // Update the recent file list menu item
  if (list_menuitem) {
    if (list_menuitem->hasSubmenuOpened())
      return false;

    Command* cmd_open_file =
      Commands::instance()->byId(CommandId::OpenFile());

    Menu* submenu = list_menuitem->getSubmenu();
    if (submenu) {
      list_menuitem->setSubmenu(NULL);
      submenu->deferDelete();
    }

    // Build the menu of recent files
    submenu = new Menu();
    list_menuitem->setSubmenu(submenu);

    auto it = App::instance()->recentFiles()->files_begin();
    auto end = App::instance()->recentFiles()->files_end();
    if (it != end) {
      Params params;

      for (; it != end; ++it) {
        const char* filename = it->c_str();
        params.set("filename", filename);

        menuitem = new AppMenuItem(
          base::get_file_name(filename).c_str(),
          cmd_open_file,
          params);
        submenu->addChild(menuitem);
      }
    }
    else {
      menuitem = new AppMenuItem("Nothing", NULL, Params());
      menuitem->setEnabled(false);
      submenu->addChild(menuitem);
    }

    // Sync native menus
    if (list_menuitem->native() &&
        list_menuitem->native()->menuItem) {
      she::Menus* menus = she::instance()->menus();
      she::Menu* osMenu = (menus ? menus->createMenu(): nullptr);
      if (osMenu) {
        createNativeSubmenus(osMenu, submenu);
        list_menuitem->native()->menuItem->setSubmenu(osMenu);
      }
    }
  }

  return true;
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

  TiXmlElement* child = elem->FirstChildElement();
  while (child) {
    Widget* menuitem = convertXmlelemToMenuitem(child);
    if (menuitem)
      menu->addChild(menuitem);
    else
      throw base::Exception("Error converting the element \"%s\" to a menu-item.\n",
                            static_cast<const char*>(child->Value()));

    child = child->NextSiblingElement();
  }

  return menu;
}

Widget* AppMenus::convertXmlelemToMenuitem(TiXmlElement* elem)
{
  // is it a <separator>?
  if (strcmp(elem->Value(), "separator") == 0)
    return new MenuSeparator;

  const char* command_id = elem->Attribute("command");
  Command* command =
    command_id ? Commands::instance()->byId(command_id):
                 nullptr;

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
                                          command, params);
  if (!menuitem)
    return nullptr;

  menuitem->processMnemonicFromText();

  // Has it a ID?
  const char* id = elem->Attribute("id");
  if (id) {
    // Recent list menu
    if (std::strcmp(id, "recent_list") == 0) {
      m_recentListMenuitem = menuitem;
    }
    else if (std::strcmp(id, "help_menu") == 0) {
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

Widget* AppMenus::createInvalidVersionMenuitem()
{
  AppMenuItem* menuitem = new AppMenuItem("WARNING!");
  Menu* subMenu = new Menu();
  subMenu->addChild(new AppMenuItem(PACKAGE " is using a customized gui.xml (maybe from your HOME directory)."));
  subMenu->addChild(new AppMenuItem("You should update your customized gui.xml file to the new version to get"));
  subMenu->addChild(new AppMenuItem("the latest commands available."));
  subMenu->addChild(new MenuSeparator);
  subMenu->addChild(new AppMenuItem("You can bypass this validation adding the correct version"));
  subMenu->addChild(new AppMenuItem("number in <gui version=\"" VERSION "\"> element."));
  menuitem->setSubmenu(subMenu);
  return menuitem;
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Command* command, const Params& params, Key* key)
{
  updateMenusList();
  for (Menu* menu : m_menus)
    if (menu)
      applyShortcutToMenuitemsWithCommand(menu, command, params, key);
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Menu* menu, Command* command, const Params& params, Key* key)
{
  for (auto child : menu->children()) {
    if (child->type() == kMenuItemWidget) {
      AppMenuItem* menuitem = dynamic_cast<AppMenuItem*>(child);
      if (!menuitem)
        continue;

      Command* mi_command = menuitem->getCommand();
      const Params& mi_params = menuitem->getParams();

      if ((mi_command) &&
          (base::utf8_icmp(mi_command->id(), command->id()) == 0) &&
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
  m_menus.push_back(m_rootMenu);
  m_menus.push_back(m_tabPopupMenu);
  m_menus.push_back(m_documentTabPopupMenu);
  m_menus.push_back(m_layerPopupMenu);
  m_menus.push_back(m_framePopupMenu);
  m_menus.push_back(m_celPopupMenu);
  m_menus.push_back(m_celMovementPopupMenu);
  m_menus.push_back(m_frameTagPopupMenu);
  m_menus.push_back(m_slicePopupMenu);
  m_menus.push_back(m_palettePopupMenu);
  m_menus.push_back(m_inkPopupMenu);
}

void AppMenus::createNativeMenus()
{
  she::Menus* menus = she::instance()->menus();
  if (!menus)       // This platform doesn't support native menu items
    return;

  if (m_osMenu)
    m_osMenu->dispose();
  m_osMenu = menus->createMenu();

#ifdef __APPLE__ // Create default macOS app menus (App ... Window)
  {
    she::MenuItemInfo about("About " PACKAGE);
    auto native = get_native_shortcut_for_command(CommandId::About());
    about.shortcut = native.shortcut;
    about.execute = [native]{
      if (can_call_global_shortcut(&native)) {
        Command* cmd = Commands::instance()->byId(CommandId::About());
        UIContext::instance()->executeCommand(cmd);
      }
    };

    she::MenuItemInfo preferences("Preferences...");
    native = get_native_shortcut_for_command(CommandId::Options());
    preferences.shortcut = native.shortcut;
    preferences.execute = [native]{
      if (can_call_global_shortcut(&native)) {
        Command* cmd = Commands::instance()->byId(CommandId::Options());
        UIContext::instance()->executeCommand(cmd);
      }
    };

    she::MenuItemInfo hide("Hide " PACKAGE, she::MenuItemInfo::Hide);
    hide.shortcut = she::Shortcut('h', she::kKeyCmdModifier);

    she::MenuItemInfo quit("Quit " PACKAGE, she::MenuItemInfo::Quit);
    quit.shortcut = she::Shortcut('q', she::kKeyCmdModifier);

    she::Menu* appMenu = menus->createMenu();
    appMenu->addItem(menus->createMenuItem(about));
    appMenu->addItem(menus->createMenuItem(she::MenuItemInfo(she::MenuItemInfo::Separator)));
    appMenu->addItem(menus->createMenuItem(preferences));
    appMenu->addItem(menus->createMenuItem(she::MenuItemInfo(she::MenuItemInfo::Separator)));
    appMenu->addItem(menus->createMenuItem(hide));
    appMenu->addItem(menus->createMenuItem(she::MenuItemInfo("Hide Others", she::MenuItemInfo::HideOthers)));
    appMenu->addItem(menus->createMenuItem(she::MenuItemInfo("Show All", she::MenuItemInfo::ShowAll)));
    appMenu->addItem(menus->createMenuItem(she::MenuItemInfo(she::MenuItemInfo::Separator)));
    appMenu->addItem(menus->createMenuItem(quit));

    she::MenuItem* appItem = menus->createMenuItem(she::MenuItemInfo("App"));
    appItem->setSubmenu(appMenu);
    m_osMenu->addItem(appItem);
  }
#endif

  createNativeSubmenus(m_osMenu, m_rootMenu);

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

    she::MenuItemInfo minimize("Minimize", she::MenuItemInfo::Minimize);
    minimize.shortcut = she::Shortcut('m', she::kKeyCmdModifier);

    she::Menu* windowMenu = menus->createMenu();
    windowMenu->addItem(menus->createMenuItem(minimize));
    windowMenu->addItem(menus->createMenuItem(she::MenuItemInfo("Zoom", she::MenuItemInfo::Zoom)));

    she::MenuItem* windowItem = menus->createMenuItem(she::MenuItemInfo("Window"));
    windowItem->setSubmenu(windowMenu);

    // We use helpIndex+1 because the first index in m_osMenu is the
    // App menu.
    m_osMenu->insertItem(helpIndex+1, windowItem);
  }
#endif

  menus->setAppMenu(m_osMenu);
}

void AppMenus::createNativeSubmenus(she::Menu* osMenu, const ui::Menu* uiMenu)
{
  she::Menus* menus = she::instance()->menus();

  for (const auto& child : uiMenu->children()) {
    she::MenuItemInfo info;
    AppMenuItem* appMenuItem = dynamic_cast<AppMenuItem*>(child);
    AppMenuItem::Native native;

    if (child->type() == kSeparatorWidget) {
      info.type = she::MenuItemInfo::Separator;
    }
    else if (child->type() == kMenuItemWidget) {
      if (appMenuItem &&
          appMenuItem->getCommand()) {
        native = get_native_shortcut_for_command(
          appMenuItem->getCommand()->id().c_str(),
          appMenuItem->getParams());
      }

      info.type = she::MenuItemInfo::Normal;
      info.text = child->text();
      info.shortcut = native.shortcut;
      info.execute = [appMenuItem]{
        if (can_call_global_shortcut(appMenuItem->native()))
          appMenuItem->executeClick();
      };
      info.validate = [appMenuItem](she::MenuItem* osItem) {
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

    she::MenuItem* osItem = menus->createMenuItem(info);
    if (osItem) {
      osMenu->addItem(osItem);
      if (appMenuItem) {
        native.menuItem = osItem;
        appMenuItem->setNative(native);
      }

      if (child->type() == ui::kMenuItemWidget &&
          ((ui::MenuItem*)child)->hasSubmenu()) {
        she::Menu* osSubmenu = menus->createMenu();
        createNativeSubmenus(osSubmenu, ((ui::MenuItem*)child)->getSubmenu());
        osItem->setSubmenu(osSubmenu);
      }
    }
  }
}

} // namespace app
