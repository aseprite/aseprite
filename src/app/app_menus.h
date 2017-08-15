// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_MENUS_H_INCLUDED
#define APP_APP_MENUS_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "obs/connection.h"
#include "ui/base.h"
#include "ui/menu.h"

class TiXmlElement;
class TiXmlHandle;

namespace app {
  class Key;
  class Command;
  class Params;

  using namespace ui;

  // Class to handle/get/reload available menus in gui.xml file.
  class AppMenus {
    AppMenus();
    DISABLE_COPYING(AppMenus);

  public:
    static AppMenus* instance();

    void reload();
    void initTheme();

    // Updates the menu of recent files.
    bool rebuildRecentList();

    Menu* getRootMenu() { return m_rootMenu; }
    MenuItem* getRecentListMenuitem() { return m_recentListMenuitem; }
    Menu* getTabPopupMenu() { return m_tabPopupMenu; }
    Menu* getDocumentTabPopupMenu() { return m_documentTabPopupMenu; }
    Menu* getLayerPopupMenu() { return m_layerPopupMenu; }
    Menu* getFramePopupMenu() { return m_framePopupMenu; }
    Menu* getCelPopupMenu() { return m_celPopupMenu; }
    Menu* getCelMovementPopupMenu() { return m_celMovementPopupMenu; }
    Menu* getFrameTagPopupMenu() { return m_frameTagPopupMenu; }
    Menu* getSlicePopupMenu() { return m_slicePopupMenu; }
    Menu* getPalettePopupMenu() { return m_palettePopupMenu; }
    Menu* getInkPopupMenu() { return m_inkPopupMenu; }

    void applyShortcutToMenuitemsWithCommand(Command* command, const Params& params, Key* key);

  private:
    Menu* loadMenuById(TiXmlHandle& handle, const char *id);
    Menu* convertXmlelemToMenu(TiXmlElement* elem);
    Widget* convertXmlelemToMenuitem(TiXmlElement* elem);
    Widget* createInvalidVersionMenuitem();
    void applyShortcutToMenuitemsWithCommand(Menu* menu, Command* command, const Params& params, Key* key);
    void updateMenusList();

    base::UniquePtr<Menu> m_rootMenu;
    MenuItem* m_recentListMenuitem;
    base::UniquePtr<Menu> m_tabPopupMenu;
    base::UniquePtr<Menu> m_documentTabPopupMenu;
    base::UniquePtr<Menu> m_layerPopupMenu;
    base::UniquePtr<Menu> m_framePopupMenu;
    base::UniquePtr<Menu> m_celPopupMenu;
    base::UniquePtr<Menu> m_celMovementPopupMenu;
    base::UniquePtr<Menu> m_frameTagPopupMenu;
    base::UniquePtr<Menu> m_slicePopupMenu;
    base::UniquePtr<Menu> m_palettePopupMenu;
    base::UniquePtr<Menu> m_inkPopupMenu;
    obs::scoped_connection m_recentFilesConn;
    std::vector<Menu*> m_menus;
  };

} // namespace app

#endif
