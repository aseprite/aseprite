/* ASE - Allegro Sprite Editor
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

#include <allegro/file.h>
#include <allegro/unicode.h>
#include <stdio.h>
#include <string.h>

#include "gui/gui.h"

#include "app.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "gui_xml.h"
#include "modules/gui.h"
#include "modules/rootmenu.h"
#include "tools/tool_box.h"
#include "util/filetoks.h"
#include "widgets/menuitem2.h"

#include "tinyxml.h"

static Menu* root_menu;

static MenuItem* recent_list_menuitem;
static Menu* document_tab_popup_menu;
static Menu* layer_popup_menu;
static Menu* frame_popup_menu;
static Menu* cel_popup_menu;
static Menu* cel_movement_popup_menu;

static int load_root_menu();
static Menu* load_menu_by_id(TiXmlHandle& handle, const char *id);
static Menu* convert_xmlelem_to_menu(TiXmlElement* elem);
static Widget* convert_xmlelem_to_menuitem(TiXmlElement* elem);
static Widget* create_invalid_version_menuitem();
static void apply_shortcut_to_menuitems_with_command(Menu* menu, Command* command, Params* params, JAccel accel);

int init_module_rootmenu()
{
  root_menu = NULL;
  document_tab_popup_menu = NULL;
  layer_popup_menu = NULL;
  frame_popup_menu = NULL;
  cel_popup_menu = NULL;
  cel_movement_popup_menu = NULL;
  recent_list_menuitem = NULL;

  return load_root_menu();
}

void exit_module_rootmenu()
{
  delete root_menu;
  delete document_tab_popup_menu;
  delete layer_popup_menu;
  delete frame_popup_menu;
  delete cel_popup_menu;
  delete cel_movement_popup_menu;
}

Menu* get_root_menu() { return root_menu; }

MenuItem* get_recent_list_menuitem() { return recent_list_menuitem; }
Menu* get_document_tab_popup_menu() { return document_tab_popup_menu; }
Menu* get_layer_popup_menu() { return layer_popup_menu; }
Menu* get_frame_popup_menu() { return frame_popup_menu; }
Menu* get_cel_popup_menu() { return cel_popup_menu; }
Menu* get_cel_movement_popup_menu() { return cel_movement_popup_menu; }

static int load_root_menu()
{
  if (app_get_menubar())
    app_get_menubar()->setMenu(NULL);

  // destroy `root-menu'
  delete root_menu;             // widget

  // create a new empty-menu
  root_menu = NULL;
  recent_list_menuitem = NULL;
  document_tab_popup_menu = NULL;
  layer_popup_menu = NULL;
  frame_popup_menu = NULL;
  cel_popup_menu = NULL;
  cel_movement_popup_menu = NULL;

  TiXmlDocument& doc(GuiXml::instance()->doc());
  TiXmlHandle handle(&doc);
  const char* path = GuiXml::instance()->filename();

  /**************************************************/
  /* load menus                                     */
  /**************************************************/

  PRINTF(" - Loading menus from \"%s\"...\n", path);

  root_menu = load_menu_by_id(handle, "main_menu");
  if (!root_menu)
    throw base::Exception("Error loading main menu from file:\n%s\nReinstall the application.",
                          static_cast<const char*>(path));

  // Add a warning element because the user is not using the last well-known gui.xml file.
  if (GuiXml::instance()->version() != VERSION)
    root_menu->insertChild(0, create_invalid_version_menuitem());

  PRINTF("Main menu loaded.\n");

  document_tab_popup_menu = load_menu_by_id(handle, "document_tab_popup");
  layer_popup_menu = load_menu_by_id(handle, "layer_popup");
  frame_popup_menu = load_menu_by_id(handle, "frame_popup");
  cel_popup_menu = load_menu_by_id(handle, "cel_popup");
  cel_movement_popup_menu = load_menu_by_id(handle, "cel_movement_popup");

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
          apply_shortcut_to_menuitems_with_command(root_menu, command, &params, accel);
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

  // Sets the "menu" of the "menu-bar" to the new "root-menu"
  if (app_get_menubar()) {
    app_get_menubar()->setMenu(root_menu);
    app_get_top_window()->remap_window();
    app_get_top_window()->invalidate();
  }

  return 0;
}

static Menu* load_menu_by_id(TiXmlHandle& handle, const char* id)
{
  ASSERT(id != NULL);

  //PRINTF("load_menu_by_id(%s)\n", id);

  // <gui><menus><menu>
  TiXmlElement* xmlMenu = handle
    .FirstChild("gui")
    .FirstChild("menus")
    .FirstChild("menu").ToElement();
  while (xmlMenu) {
    const char* menu_id = xmlMenu->Attribute("id");

    if (menu_id && strcmp(menu_id, id) == 0)
      return convert_xmlelem_to_menu(xmlMenu);

    xmlMenu = xmlMenu->NextSiblingElement();
  }

  PRINTF(" - \"%s\" element was not found\n", id);
  return NULL;
}

static Menu* convert_xmlelem_to_menu(TiXmlElement* elem)
{
  Menu* menu = new Menu();

  //PRINTF("convert_xmlelem_to_menu(%s, %s, %s)\n", elem->Value(), elem->Attribute("id"), elem->Attribute("text"));

  TiXmlElement* child = elem->FirstChildElement();
  while (child) {
    Widget* menuitem = convert_xmlelem_to_menuitem(child);
    if (menuitem)
      menu->addChild(menuitem);
    else
      throw base::Exception("Error converting the element \"%s\" to a menu-item.\n",
                            static_cast<const char*>(child->Value()));

    child = child->NextSiblingElement();
  }

  return menu;
}

static Widget* convert_xmlelem_to_menuitem(TiXmlElement* elem)
{
  // is it a <separator>?
  if (strcmp(elem->Value(), "separator") == 0)
    return ji_separator_new(NULL, JI_HORIZONTAL);

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
      recent_list_menuitem = menuitem;
    }
  }

  // Has it a sub-menu (<menu>)?
  if (strcmp(elem->Value(), "menu") == 0) {
    // Create the sub-menu
    Menu* subMenu = convert_xmlelem_to_menu(elem);
    if (!subMenu)
      throw base::Exception("Error reading the sub-menu\n");

    menuitem->setSubmenu(subMenu);
  }

  return menuitem;
}

static Widget* create_invalid_version_menuitem()
{
  MenuItem2* menuitem = new MenuItem2("WARNING!", NULL, NULL);
  Menu* subMenu = new Menu();
  subMenu->addChild(new MenuItem2(PACKAGE " is using a customized gui.xml (maybe from your HOME directory).", NULL, NULL));
  subMenu->addChild(new MenuItem2("You should update your customized gui.xml file to the new version to get", NULL, NULL));
  subMenu->addChild(new MenuItem2("the latest commands available.", NULL, NULL));
  subMenu->addChild(ji_separator_new(NULL, JI_HORIZONTAL));
  subMenu->addChild(new MenuItem2("You can bypass this validation adding the correct version", NULL, NULL));
  subMenu->addChild(new MenuItem2("number in <gui version=\"" VERSION "\"> element.", NULL, NULL));
  menuitem->setSubmenu(subMenu);
  return menuitem;
}

static void apply_shortcut_to_menuitems_with_command(Menu* menu, Command *command, Params* params, JAccel accel)
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
        apply_shortcut_to_menuitems_with_command(submenu, command, params, accel);
    }
  }

  jlist_free(children);
}
