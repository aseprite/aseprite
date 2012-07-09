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

#include "config.h"

#include "app_menus.h"

#include "app.h"
#include "base/bind.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "gui_xml.h"
#include "modules/gui.h"
#include "recent_files.h"
#include "tools/tool_box.h"
#include "ui/gui.h"
#include "util/filetoks.h"
#include "widgets/main_window.h"
#include "widgets/menuitem2.h"

#include "tinyxml.h"
#include <allegro/file.h>
#include <allegro/unicode.h>
#include <stdio.h>
#include <string.h>

using namespace ui;

static void destroy_instance(AppMenus* instance)
{
  delete instance;
}

// static
AppMenus* AppMenus::instance()
{
  static AppMenus* instance = NULL;
  if (!instance) {
    instance = new AppMenus;
    App::instance()->Exit.connect(Bind<void>(&destroy_instance, instance));
  }
  return instance;
}

AppMenus::AppMenus()
  : m_recentListMenuitem(NULL)
{
}

void AppMenus::reload()
{
  TiXmlDocument& doc(GuiXml::instance()->doc());
  TiXmlHandle handle(&doc);
  const char* path = GuiXml::instance()->filename();

  /**************************************************/
  /* load menus                                     */
  /**************************************************/

  PRINTF(" - Loading menus from \"%s\"...\n", path);

  m_rootMenu.reset(loadMenuById(handle, "main_menu"));

  // Add a warning element because the user is not using the last well-known gui.xml file.
  if (GuiXml::instance()->version() != VERSION)
    m_rootMenu->insertChild(0, createInvalidVersionMenuitem());

  PRINTF("Main menu loaded.\n");

  m_documentTabPopupMenu.reset(loadMenuById(handle, "document_tab_popup"));
  m_layerPopupMenu.reset(loadMenuById(handle, "layer_popup"));
  m_framePopupMenu.reset(loadMenuById(handle, "frame_popup"));
  m_celPopupMenu.reset(loadMenuById(handle, "cel_popup"));
  m_celMovementPopupMenu.reset(loadMenuById(handle, "cel_movement_popup"));

  /**************************************************/
  /* load keyboard shortcuts for commands           */
  /**************************************************/

  PRINTF(" - Loading commands keyboard shortcuts from \"%s\"...\n", path);

  // <gui><keyboard><commands><key>
  TiXmlElement* xmlKey = handle
    .FirstChild("gui")
    .FirstChild("keyboard")
    .FirstChild("commands")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* command_name = xmlKey->Attribute("command");
    const char* command_key = xmlKey->Attribute("shortcut");

    if (command_name && command_key) {
      Command* command = CommandsModule::instance()->getCommandByName(command_name);
      if (command) {
        // Read params
        Params params;

        TiXmlElement* xmlParam = xmlKey->FirstChildElement("param");
        while (xmlParam) {
          const char* param_name = xmlParam->Attribute("name");
          const char* param_value = xmlParam->Attribute("value");

          if (param_name && param_value)
            params.set(param_name, param_value);

          xmlParam = xmlParam->NextSiblingElement();
        }

        bool first_shortcut =
          (get_accel_to_execute_command(command_name, &params) == NULL);

        PRINTF(" - Shortcut for command `%s' <%s>\n", command_name, command_key);

        // add the keyboard shortcut to the command
        JAccel accel =
          add_keyboard_shortcut_to_execute_command(command_key, command_name, &params);

        // add the shortcut to the menuitems with this
        // command (this is only visual, the "manager_msg_proc"
        // is the only one that process keyboard shortcuts)
        if (first_shortcut)
          applyShortcutToMenuitemsWithCommand(m_rootMenu, command, &params, accel);
      }
    }

    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for tools
  PRINTF(" - Loading tools keyboard shortcuts from \"%s\"...\n", path);
  // <gui><keyboard><tools><key>
  xmlKey = handle
    .FirstChild("gui")
    .FirstChild("keyboard")
    .FirstChild("tools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = xmlKey->Attribute("shortcut");

    if (tool_id && tool_key) {
      tools::Tool* tool = App::instance()->getToolBox()->getToolById(tool_id);
      if (tool) {
        PRINTF(" - Shortcut for tool `%s': <%s>\n", tool_id, tool_key);
        add_keyboard_shortcut_to_change_tool(tool_key, tool);
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load keyboard shortcuts for quicktools
  // <gui><keyboard><quicktools><key>
  xmlKey = handle
    .FirstChild("gui")
    .FirstChild("keyboard")
    .FirstChild("quicktools")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_id = xmlKey->Attribute("tool");
    const char* tool_key = xmlKey->Attribute("shortcut");

    if (tool_id && tool_key) {
      tools::Tool* tool = App::instance()->getToolBox()->getToolById(tool_id);
      if (tool) {
        PRINTF(" - Shortcut for quicktool `%s': <%s>\n", tool_id, tool_key);
        add_keyboard_shortcut_to_quicktool(tool_key, tool);
      }
    }
    xmlKey = xmlKey->NextSiblingElement();
  }

  // Load special keyboard shortcuts for sprite editor customization
  // <gui><keyboard><spriteeditor>
  xmlKey = handle
    .FirstChild("gui")
    .FirstChild("keyboard")
    .FirstChild("spriteeditor")
    .FirstChild("key").ToElement();
  while (xmlKey) {
    const char* tool_action = xmlKey->Attribute("action");
    const char* tool_key = xmlKey->Attribute("shortcut");

    if (tool_action && tool_key) {
      PRINTF(" - Shortcut for sprite editor `%s': <%s>\n", tool_action, tool_key);
      add_keyboard_shortcut_to_spriteeditor(tool_key, tool_action);
    }
    xmlKey = xmlKey->NextSiblingElement();
  }
}

bool AppMenus::rebuildRecentList()
{
  MenuItem* list_menuitem = m_recentListMenuitem;
  MenuItem* menuitem;

  // Update the recent file list menu item
  if (list_menuitem) {
    if (list_menuitem->hasSubmenuOpened())
      return false;

    Command* cmd_open_file = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);

    Menu* submenu = list_menuitem->getSubmenu();
    if (submenu) {
      list_menuitem->setSubmenu(NULL);
      submenu->deferDelete();
    }

    // Build the menu of recent files
    submenu = new Menu();
    list_menuitem->setSubmenu(submenu);

    RecentFiles::const_iterator it = App::instance()->getRecentFiles()->files_begin();
    RecentFiles::const_iterator end = App::instance()->getRecentFiles()->files_end();

    if (it != end) {
      Params params;

      for (; it != end; ++it) {
        const char* filename = it->c_str();

        params.set("filename", filename);

        menuitem = new MenuItem2(get_filename(filename), cmd_open_file, &params);
        submenu->addChild(menuitem);
      }
    }
    else {
      menuitem = new MenuItem2("Nothing", NULL, NULL);
      menuitem->setEnabled(false);
      submenu->addChild(menuitem);
    }
  }

  return true;
}

Menu* AppMenus::loadMenuById(TiXmlHandle& handle, const char* id)
{
  ASSERT(id != NULL);

  //PRINTF("loadMenuById(%s)\n", id);

  // <gui><menus><menu>
  TiXmlElement* xmlMenu = handle
    .FirstChild("gui")
    .FirstChild("menus")
    .FirstChild("menu").ToElement();
  while (xmlMenu) {
    const char* menu_id = xmlMenu->Attribute("id");

    if (menu_id && strcmp(menu_id, id) == 0)
      return convertXmlelemToMenu(xmlMenu);

    xmlMenu = xmlMenu->NextSiblingElement();
  }

  throw base::Exception("Error loading menu '%s'\nReinstall the application.", id);
}

Menu* AppMenus::convertXmlelemToMenu(TiXmlElement* elem)
{
  Menu* menu = new Menu();

  //PRINTF("convertXmlelemToMenu(%s, %s, %s)\n", elem->Value(), elem->Attribute("id"), elem->Attribute("text"));

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
    return new Separator(NULL, JI_HORIZONTAL);

  const char* command_name = elem->Attribute("command");
  Command* command =
    command_name ? CommandsModule::instance()->getCommandByName(command_name):
                   NULL;

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

  /* create the item */
  MenuItem2* menuitem = new MenuItem2(elem->Attribute("text"),
                                      command, command ? &params: NULL);
  if (!menuitem)
    return NULL;

  /* has it a ID? */
  const char* id = elem->Attribute("id");
  if (id) {
    /* recent list menu */
    if (strcmp(id, "recent_list") == 0) {
      m_recentListMenuitem = menuitem;
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
  MenuItem2* menuitem = new MenuItem2("WARNING!", NULL, NULL);
  Menu* subMenu = new Menu();
  subMenu->addChild(new MenuItem2(PACKAGE " is using a customized gui.xml (maybe from your HOME directory).", NULL, NULL));
  subMenu->addChild(new MenuItem2("You should update your customized gui.xml file to the new version to get", NULL, NULL));
  subMenu->addChild(new MenuItem2("the latest commands available.", NULL, NULL));
  subMenu->addChild(new Separator(NULL, JI_HORIZONTAL));
  subMenu->addChild(new MenuItem2("You can bypass this validation adding the correct version", NULL, NULL));
  subMenu->addChild(new MenuItem2("number in <gui version=\"" VERSION "\"> element.", NULL, NULL));
  menuitem->setSubmenu(subMenu);
  return menuitem;
}

void AppMenus::applyShortcutToMenuitemsWithCommand(Menu* menu, Command *command, Params* params, JAccel accel)
{
  JList children = menu->getChildren();
  JLink link;

  JI_LIST_FOR_EACH(children, link) {
    Widget* child = (Widget*)link->data;

    if (child->getType() == JI_MENUITEM) {
      ASSERT(dynamic_cast<MenuItem2*>(child) != NULL);

      MenuItem2* menuitem = static_cast<MenuItem2*>(child);
      Command* mi_command = menuitem->getCommand();
      Params* mi_params = menuitem->getParams();

      if (mi_command &&
          ustricmp(mi_command->short_name(), command->short_name()) == 0 &&
          ((mi_params && *mi_params == *params) ||
           (Params() == *params))) {
        // Set the accelerator to be shown in this menu-item
        menuitem->setAccel(jaccel_new_copy(accel));
      }

      if (Menu* submenu = menuitem->getSubmenu())
        applyShortcutToMenuitemsWithCommand(submenu, command, params, accel);
    }
  }

  jlist_free(children);
}
