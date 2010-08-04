/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "app.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "core/core.h"
#include "intl/intl.h"
#include "modules/gui.h"
#include "modules/rootmenu.h"
#include "resource_finder.h"
#include "tools/toolbox.h"
#include "util/filetoks.h"
#include "widgets/menuitem.h"

#include "tinyxml.h"

static JWidget root_menu;

static JWidget recent_list_menuitem;
static JWidget layer_popup_menu;
static JWidget frame_popup_menu;
static JWidget cel_popup_menu;
static JWidget cel_movement_popup_menu;

static int load_root_menu();
static JWidget load_menu_by_id(TiXmlHandle& handle, const char *id);
static JWidget convert_xmlelem_to_menu(TiXmlElement* elem);
static JWidget convert_xmlelem_to_menuitem(TiXmlElement* elem);
static void apply_shortcut_to_menuitems_with_command(JWidget menu, Command* command, Params* params, JAccel accel);

int init_module_rootmenu()
{
  root_menu = NULL;
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
  delete layer_popup_menu;
  delete frame_popup_menu;
  delete cel_popup_menu;
  delete cel_movement_popup_menu;
}

JWidget get_root_menu() { return root_menu; }

JWidget get_recent_list_menuitem() { return recent_list_menuitem; }
JWidget get_layer_popup_menu() { return layer_popup_menu; }
JWidget get_frame_popup_menu() { return frame_popup_menu; }
JWidget get_cel_popup_menu() { return cel_popup_menu; }
JWidget get_cel_movement_popup_menu() { return cel_movement_popup_menu; }

static int load_root_menu()
{
  if (app_get_menubar())
    jmenubar_set_menu(app_get_menubar(), NULL);

  // destroy `root-menu'
  delete root_menu;		// widget

  // create a new empty-menu
  root_menu = NULL;
  recent_list_menuitem = NULL;
  layer_popup_menu = NULL;
  frame_popup_menu = NULL;
  cel_popup_menu = NULL;
  cel_movement_popup_menu = NULL;

  ResourceFinder rf;
  rf.findInDataDir("gui.xml");

  while (const char* path = rf.next()) {
    PRINTF("Trying to load GUI definition file from \"%s\"...\n", path);

    if (!exists(path))
      continue;

    PRINTF(" - \"%s\" found\n", path);
  
    /* open the XML menu definition file */
    TiXmlDocument doc;
    if (!doc.LoadFile(path))
      throw ase_exception(&doc);

    TiXmlHandle handle(&doc);

    /**************************************************/
    /* load menus                                     */
    /**************************************************/

    PRINTF(" - Loading menus from \"%s\"...\n", path);

    root_menu = load_menu_by_id(handle, "main_menu");
    if (!root_menu)
      throw ase_exception("Error loading main menu from file:\n%s\nReinstall the application.",
			  static_cast<const char*>(path));

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
	Command *command = CommandsModule::instance()->get_command_by_name(command_name);
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

    /**************************************************/
    /* load keyboard shortcuts for tools              */
    /**************************************************/

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
	Tool* tool = App::instance()->get_toolbox()->getToolById(tool_id);
	if (tool) {
	  /* add the keyboard shortcut to the tool */
	  PRINTF(" - Shortcut for tool `%s': <%s>\n", tool_id, tool_key);
	  add_keyboard_shortcut_to_change_tool(tool_key, tool);
	}
      }

      xmlKey = xmlKey->NextSiblingElement();
    }

    if (root_menu) {
      PRINTF("Main menu loaded.\n");
      break;
    }
  }

  // No menus
  if (!root_menu)
    throw ase_exception("Error loading main menu\n");

  // Sets the "menu" of the "menu-bar" to the new "root-menu"
  if (app_get_menubar()) {
    jmenubar_set_menu(app_get_menubar(), root_menu);
    app_get_top_window()->remap_window();
    jwidget_dirty(app_get_top_window());
  }

  return 0;
}

static JWidget load_menu_by_id(TiXmlHandle& handle, const char* id)
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

static JWidget convert_xmlelem_to_menu(TiXmlElement* elem)
{
  JWidget menu = jmenu_new();

  //PRINTF("convert_xmlelem_to_menu(%s, %s, %s)\n", elem->Value(), elem->Attribute("id"), elem->Attribute("text"));

  TiXmlElement* child = elem->FirstChildElement();
  while (child) {
    JWidget menuitem = convert_xmlelem_to_menuitem(child);
    if (menuitem)
      jwidget_add_child(menu, menuitem);
    else
      throw ase_exception("Error converting the element \"%s\" to a menu-item.\n",
			  static_cast<const char*>(child->Value()));

    child = child->NextSiblingElement();
  }

  return menu;
}

static JWidget convert_xmlelem_to_menuitem(TiXmlElement* elem)
{
  // is it a <separator>?
  if (strcmp(elem->Value(), "separator") == 0)
    return ji_separator_new(NULL, JI_HORIZONTAL);

  const char* command_name = elem->Attribute("command");
  Command* command =
    command_name ? CommandsModule::instance()->get_command_by_name(command_name):
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
  JWidget menuitem = menuitem_new(elem->Attribute("text"),
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

  /* has it a sub-menu (<menu>)? */
  if (strcmp(elem->Value(), "menu") == 0) {
    /* create the sub-menu */
    JWidget sub_menu = convert_xmlelem_to_menu(elem);
    if (!sub_menu)
      throw ase_exception("Error reading the sub-menu\n");

    jmenuitem_set_submenu(menuitem, sub_menu);
  }

  return menuitem;
}

static void apply_shortcut_to_menuitems_with_command(JWidget menu, Command *command, Params* params, JAccel accel)
{
  JList children = jwidget_get_children(menu);
  JWidget menuitem, submenu;
  JLink link;

  JI_LIST_FOR_EACH(children, link) {
    menuitem = (JWidget)link->data;

    if (menuitem->getType() == JI_MENUITEM) {
      Command* mi_command = menuitem_get_command(menuitem);
      Params* mi_params = menuitem_get_params(menuitem);

      if (mi_command &&
	  ustricmp(mi_command->short_name(), command->short_name()) == 0 &&
	  ((mi_params && *mi_params == *params) ||
	   (Params() == *params))) {
	// Set the accelerator to be shown in this menu-item
	jmenuitem_set_accel(menuitem, jaccel_new_copy(accel));
      }

      submenu = jmenuitem_get_submenu(menuitem);
      if (submenu)
	apply_shortcut_to_menuitems_with_command(submenu, command, params, accel);
    }
  }

  jlist_free(children);
}
