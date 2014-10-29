/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_APP_MENUS_H_INCLUDED
#define APP_APP_MENUS_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/unique_ptr.h"
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

    // Updates the menu of recent files.
    bool rebuildRecentList();

    Menu* getRootMenu() { return m_rootMenu; }
    MenuItem* getRecentListMenuitem() { return m_recentListMenuitem; }
    Menu* getDocumentTabPopupMenu() { return m_documentTabPopupMenu; }
    Menu* getLayerPopupMenu() { return m_layerPopupMenu; }
    Menu* getFramePopupMenu() { return m_framePopupMenu; }
    Menu* getCelPopupMenu() { return m_celPopupMenu; }
    Menu* getCelMovementPopupMenu() { return m_celMovementPopupMenu; }

    void applyShortcutToMenuitemsWithCommand(Command* command, Params* params, Key* key);

  private:
    Menu* loadMenuById(TiXmlHandle& handle, const char *id);
    Menu* convertXmlelemToMenu(TiXmlElement* elem);
    Widget* convertXmlelemToMenuitem(TiXmlElement* elem);
    Widget* createInvalidVersionMenuitem();
    void applyShortcutToMenuitemsWithCommand(Menu* menu, Command* command, Params* params, Key* key);

    base::UniquePtr<Menu> m_rootMenu;
    MenuItem* m_recentListMenuitem;
    base::UniquePtr<Menu> m_documentTabPopupMenu;
    base::UniquePtr<Menu> m_layerPopupMenu;
    base::UniquePtr<Menu> m_framePopupMenu;
    base::UniquePtr<Menu> m_celPopupMenu;
    base::UniquePtr<Menu> m_celMovementPopupMenu;
  };

} // namespace app

#endif
