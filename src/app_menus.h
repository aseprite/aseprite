/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#ifndef APP_MENUS_H_INCLUDED
#define APP_MENUS_H_INCLUDED

#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "ui/base.h"
#include "ui/menu.h"

class Command;
class Params;
class TiXmlElement;
class TiXmlHandle;

// Class to handle/get/reload available menus in gui.xml file.
class AppMenus
{
  AppMenus();
  DISABLE_COPYING(AppMenus);

public:
  static AppMenus* instance();

  void reload();

  // Updates the menu of recent files.
  bool rebuildRecentList();

  ui::Menu* getRootMenu() { return m_rootMenu; }
  ui::MenuItem* getRecentListMenuitem() { return m_recentListMenuitem; }
  ui::Menu* getDocumentTabPopupMenu() { return m_documentTabPopupMenu; }
  ui::Menu* getLayerPopupMenu() { return m_layerPopupMenu; }
  ui::Menu* getFramePopupMenu() { return m_framePopupMenu; }
  ui::Menu* getCelPopupMenu() { return m_celPopupMenu; }
  ui::Menu* getCelMovementPopupMenu() { return m_celMovementPopupMenu; }

private:
  ui::Menu* loadMenuById(TiXmlHandle& handle, const char *id);
  ui::Menu* convertXmlelemToMenu(TiXmlElement* elem);
  ui::Widget* convertXmlelemToMenuitem(TiXmlElement* elem);
  ui::Widget* createInvalidVersionMenuitem();
  void applyShortcutToMenuitemsWithCommand(ui::Menu* menu, Command* command, Params* params, ui::JAccel accel);

  UniquePtr<ui::Menu> m_rootMenu;
  ui::MenuItem* m_recentListMenuitem;
  UniquePtr<ui::Menu> m_documentTabPopupMenu;
  UniquePtr<ui::Menu> m_layerPopupMenu;
  UniquePtr<ui::Menu> m_framePopupMenu;
  UniquePtr<ui::Menu> m_celPopupMenu;
  UniquePtr<ui::Menu> m_celMovementPopupMenu;
};

#endif
