// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_MENUS_H_INCLUDED
#define APP_APP_MENUS_H_INCLUDED
#pragma once

#include "app/i18n/xml_translator.h"
#include "app/ui/key.h"
#include "app/widget_type_mismatch.h"
#include "base/disable_copying.h"
#include "obs/connection.h"
#include "ui/base.h"
#include "ui/menu.h"

#include <memory>

class TiXmlElement;
class TiXmlHandle;

namespace os {
  class Menu;
  class Shortcut;
}

namespace app {
  class Command;
  class Params;

  using namespace ui;

  // Class to handle/get/reload available menus in gui.xml file.
  class AppMenus {
    AppMenus();
    DISABLE_COPYING(AppMenus);

  public:
    static AppMenus* instance();

    ~AppMenus();

    void reload();
    void initTheme();

    // Updates the menu of recent files.
    bool rebuildRecentList();

    Menu* getRootMenu() { return m_rootMenu.get(); }
    MenuItem* getRecentListMenuitem() { return m_recentListMenuitem; }
    Menu* getTabPopupMenu() { return m_tabPopupMenu.get(); }
    Menu* getDocumentTabPopupMenu() { return m_documentTabPopupMenu.get(); }
    Menu* getLayerPopupMenu() { return m_layerPopupMenu.get(); }
    Menu* getFramePopupMenu() { return m_framePopupMenu.get(); }
    Menu* getCelPopupMenu() { return m_celPopupMenu.get(); }
    Menu* getCelMovementPopupMenu() { return m_celMovementPopupMenu.get(); }
    Menu* getFrameTagPopupMenu() { return m_frameTagPopupMenu.get(); }
    Menu* getSlicePopupMenu() { return m_slicePopupMenu.get(); }
    Menu* getPalettePopupMenu() { return m_palettePopupMenu.get(); }
    Menu* getInkPopupMenu() { return m_inkPopupMenu.get(); }

    void applyShortcutToMenuitemsWithCommand(Command* command, const Params& params,
                                             const KeyPtr& key);
    void syncNativeMenuItemKeyShortcuts();

  private:
    Menu* loadMenuById(TiXmlHandle& handle, const char *id);
    Menu* convertXmlelemToMenu(TiXmlElement* elem);
    Widget* convertXmlelemToMenuitem(TiXmlElement* elem);
    Widget* createInvalidVersionMenuitem();
    void applyShortcutToMenuitemsWithCommand(Menu* menu, Command* command, const Params& params,
                                             const KeyPtr& key);
    void syncNativeMenuItemKeyShortcuts(Menu* menu);
    void updateMenusList();
    void createNativeMenus();
    void createNativeSubmenus(os::Menu* osMenu, const ui::Menu* uiMenu);

#ifdef ENABLE_SCRIPTING
    void loadScriptsSubmenu(ui::Menu* menu,
                            const std::string& dir,
                            const bool rootLevel);
#endif

    std::unique_ptr<Menu> m_rootMenu;
    MenuItem* m_recentListMenuitem;
    MenuItem* m_helpMenuitem;
    std::unique_ptr<Menu> m_tabPopupMenu;
    std::unique_ptr<Menu> m_documentTabPopupMenu;
    std::unique_ptr<Menu> m_layerPopupMenu;
    std::unique_ptr<Menu> m_framePopupMenu;
    std::unique_ptr<Menu> m_celPopupMenu;
    std::unique_ptr<Menu> m_celMovementPopupMenu;
    std::unique_ptr<Menu> m_frameTagPopupMenu;
    std::unique_ptr<Menu> m_slicePopupMenu;
    std::unique_ptr<Menu> m_palettePopupMenu;
    std::unique_ptr<Menu> m_inkPopupMenu;
    obs::scoped_connection m_recentFilesConn;
    std::vector<Menu*> m_menus;
    os::Menu* m_osMenu;
    XmlTranslator m_xmlTranslator;
  };

  os::Shortcut get_os_shortcut_from_key(const Key* key);

} // namespace app

#endif
