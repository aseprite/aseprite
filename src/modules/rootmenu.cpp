/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <stdio.h>
#include <string.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "commands/command.h"
#include "commands/params.h"
#include "console.h"
#include "app.h"
#include "core/core.h"
#include "core/dirs.h"
#include "intl/intl.h"
#include "modules/rootmenu.h"
#include "modules/tools.h"
#include "modules/gui.h"
#include "util/filetoks.h"
#include "widgets/menuitem.h"

static JWidget root_menu;

static JWidget recent_list_menuitem;
static JWidget layer_popup_menu;
static JWidget frame_popup_menu;
static JWidget cel_popup_menu;
static JWidget cel_movement_popup_menu;
static JWidget filters_popup_menu;

static int load_root_menu();
static JWidget load_menu_by_id(JXml xml, const char *id, const char *filename);
static JWidget convert_xmlelem_to_menu(JXmlElem elem);
static JWidget convert_xmlelem_to_menuitem(JXmlElem elem);
static void apply_shortcut_to_menuitems_with_command(JWidget menu, Command* command, Params* params, JAccel accel);

int init_module_rootmenu()
{
  root_menu = NULL;
  layer_popup_menu = NULL;
  frame_popup_menu = NULL;
  cel_popup_menu = NULL;
  cel_movement_popup_menu = NULL;
  filters_popup_menu = NULL;
  recent_list_menuitem = NULL;

  return load_root_menu();
}

void exit_module_rootmenu()
{
  jwidget_free(root_menu);

  if (layer_popup_menu) jwidget_free(layer_popup_menu);
  if (frame_popup_menu) jwidget_free(frame_popup_menu);
  if (cel_popup_menu) jwidget_free(cel_popup_menu);
  if (cel_movement_popup_menu) jwidget_free(cel_movement_popup_menu);
  if (filters_popup_menu) jwidget_free(filters_popup_menu);
}

JWidget get_root_menu() { return root_menu; }

JWidget get_recent_list_menuitem() { return recent_list_menuitem; }
JWidget get_layer_popup_menu() { return layer_popup_menu; }
JWidget get_frame_popup_menu() { return frame_popup_menu; }
JWidget get_cel_popup_menu() { return cel_popup_menu; }
JWidget get_cel_movement_popup_menu() { return cel_movement_popup_menu; }

/* void show_fx_popup_menu() */
/* { */
/*   if (is_interactive() && */
/*       filters_popup_menuitem && */
/*       jmenuitem_get_submenu(filters_popup_menuitem)) { */
/*     jmenu_popup(jmenuitem_get_submenu(filters_popup_menuitem), */
/* 		jmouse_x(0), jmouse_y(0)); */
/*   } */
/* } */

static int load_root_menu()
{
  DIRS *dirs, *dir;
  JLink link;
  JXml xml;
  JXmlElem child;

  if (app_get_menubar())
    jmenubar_set_menu(app_get_menubar(), NULL);

  /* destroy `root-menu' if it exists */
  if (root_menu)
    jwidget_free(root_menu);

  /* create a new empty-menu */
  root_menu = NULL;
  recent_list_menuitem = NULL;
  layer_popup_menu = NULL;
  frame_popup_menu = NULL;
  cel_popup_menu = NULL;
  cel_movement_popup_menu = NULL;
  filters_popup_menu = NULL;

  dirs = filename_in_datadir("usergui.xml");
  {
    char buf[256];
    sprintf(buf, "gui-%s.xml", intl_get_lang());
    dirs_cat_dirs(dirs, filename_in_datadir(buf));
    dirs_cat_dirs(dirs, filename_in_datadir("gui-en.xml"));
  }

  for (dir=dirs; dir; dir=dir->next) {
    PRINTF("Trying to load GUI definition file from \"%s\"...\n", dir->path);
    
    /* open the XML menu definition file */
    xml = jxml_new_from_file(dir->path);
    if (xml && jxml_get_root(xml)) {
      /**************************************************/
      /* load menus                                     */
      /**************************************************/

      PRINTF("Trying to menus from \"%s\"...\n", dir->path);

      root_menu = load_menu_by_id(xml, "main_menu", dir->path);
      if (!root_menu) {
	PRINTF("Error loading \"main_menu\" from \"%s\" file.\n", dir->path);
	return -1;
      }

      layer_popup_menu = load_menu_by_id(xml, "layer_popup", dir->path);
      frame_popup_menu = load_menu_by_id(xml, "frame_popup", dir->path);
      cel_popup_menu = load_menu_by_id(xml, "cel_popup", dir->path);
      cel_movement_popup_menu = load_menu_by_id(xml, "cel_movement_popup", dir->path);
      filters_popup_menu = load_menu_by_id(xml, "filters_popup", dir->path);

      /**************************************************/
      /* load keyboard shortcuts for commands           */
      /**************************************************/

      PRINTF("Loading commands shortcuts from \"%s\"...\n", dir->path);

      /* find the <keyboard> element */
      child = jxmlelem_get_elem_by_name(jxml_get_root(xml), "keyboard");
      if (child != NULL) {
	/* find the <menus> element */
	child = jxmlelem_get_elem_by_name(child, "commands");
	if (child != NULL) {
	  /* for each children in <keyboard><commands>...</commands></keyboard> */
	  JI_LIST_FOR_EACH(child->head.children, link) {
	    JXmlNode child2 = (JXmlNode)link->data;

	    /* it is a <key> element? */
	    if (child2->type == JI_XML_ELEM &&
		strcmp(jxmlelem_get_name((JXmlElem)child2), "key") == 0) {
	      /* finally, we can read the <key /> */
	      const char *command_name = jxmlelem_get_attr((JXmlElem)child2, "command");
	      const char *command_key = jxmlelem_get_attr((JXmlElem)child2, "shortcut");

	      if (command_name && command_key) {
		Command *command = CommandsModule::instance()->get_command_by_name(command_name);
		if (command) {
		  // Read params
		  Params params;
		  {
		    JLink link3;
		    JI_LIST_FOR_EACH(((JXmlElem)child2)->head.children, link3) {
		      JXmlNode child3 = (JXmlNode)link3->data;

		      if (child3->type == JI_XML_ELEM &&
			  strcmp(jxmlelem_get_name((JXmlElem)child3), "param") == 0) {
			const char* param_name = jxmlelem_get_attr((JXmlElem)child3, "name");
			const char* param_value = jxmlelem_get_attr((JXmlElem)child3, "value");
			params.set(param_name, param_value);
		      }
		    }
		  }

		  bool first_shortcut =
		    (get_accel_to_execute_command(command_name, &params) == NULL);

		  PRINTF("- Shortcut for command `%s' <%s>\n", command_name, command_key);
		  
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
	    }
	  }
	}
      }

      /**************************************************/
      /* load keyboard shortcuts for tools              */
      /**************************************************/

      PRINTF("Loading tools shortcuts from \"%s\"...\n", dir->path);

      /* find the <keyboard> element */
      child = jxmlelem_get_elem_by_name(jxml_get_root(xml), "keyboard");
      if (child != NULL) {
	/* find the <tools> element */
	child = jxmlelem_get_elem_by_name(child, "tools");
	if (child != NULL) {
	  /* for each children in <keyboard><tools>...</tools></keyboard> */
	  JI_LIST_FOR_EACH(child->head.children, link) {
	    JXmlNode child2 = (JXmlNode)link->data;

	    /* it is a <key> element? */
	    if (child2->type == JI_XML_ELEM &&
		strcmp(jxmlelem_get_name((JXmlElem)child2), "key") == 0) {
	      /* finally, we can read the <key /> */
	      const char *tool_name = jxmlelem_get_attr((JXmlElem)child2, "tool");
	      const char *tool_key = jxmlelem_get_attr((JXmlElem)child2, "shortcut");
	      if (tool_name && tool_key) {
		Tool *tool = get_tool_by_name(tool_name);
		if (tool) {
		  /* add the keyboard shortcut to the tool */
		  PRINTF("- Shortcut for tool `%s': <%s>\n", tool_name, tool_key);
		  add_keyboard_shortcut_to_change_tool(tool_key, tool);
		}
	      }
	    }
	  }
	}
      }
      
      /* free the XML file */
      jxml_free(xml);
    }

    if (root_menu) {
      PRINTF("Main menu loaded.\n");
      break;
    }
  }

  dirs_free(dirs);

  /* no menus? */
  if (!root_menu) {
    user_printf(_("Error loading main menu\n"));
    return -1;
  }

  /* sets the "menu" of the "menu-bar" to the new "root-menu" */
  if (app_get_menubar()) {
    jmenubar_set_menu(app_get_menubar(), root_menu);
    jwindow_remap(app_get_top_window());
    jwidget_dirty(app_get_top_window());
  }

  return 0;
}

static JWidget load_menu_by_id(JXml xml, const char *id, const char *filename)
{
  JWidget menu = NULL;
  JXmlElem elem;

  assert(id != NULL);

  /* get the <menu> element with the specified id */
  elem = jxml_get_elem_by_id(xml, id);
  if (elem) {
    /* is it a <menu> element? */
    if (strcmp(jxmlelem_get_name(elem), "menu") == 0) {
      /* ok, convert it to a menu JWidget */
      menu = convert_xmlelem_to_menu(elem);
    }
    else
      PRINTF("Invalid element with id=\"%s\" in \"%s\"\n", id, filename);
  }
  else
    PRINTF("\"%s\" element couldn't be found...\n", id);

  return menu;
}

static JWidget convert_xmlelem_to_menu(JXmlElem elem)
{
  JWidget menu = jmenu_new();
  JWidget menuitem;
  JLink link;

  JI_LIST_FOR_EACH(((JXmlNode)elem)->children, link) {
    JXmlNode child = (JXmlNode)link->data;

/*     PRINTF("convert_xmlelem_to_menu: child->value = %p (%s)\n", */
/* 	   child->value, child->value ? child->value: ""); */

    if (child->type == JI_XML_ELEM) {
      menuitem = convert_xmlelem_to_menuitem((JXmlElem)child);
      if (menuitem)
	jwidget_add_child(menu, menuitem);
      else {
	PRINTF("Error converting the element \"%s\" to a menu-item.\n",
	       jxmlelem_get_name((JXmlElem)child));
	return NULL;
      }
    }
  }

  return menu;
}

static JWidget convert_xmlelem_to_menuitem(JXmlElem elem)
{
  JWidget menuitem;
  const char *id;

  /* is it a separator? */
  if (strcmp(jxmlelem_get_name(elem), "separator") == 0)
    return ji_separator_new(NULL, JI_HORIZONTAL);

  const char* command_name = jxmlelem_get_attr(elem, "command");
  Command* command = 
    command_name ? CommandsModule::instance()->get_command_by_name(command_name):
		   NULL;

  // load params
  Params params;
  if (command) {
    JLink link3;
    JI_LIST_FOR_EACH(((JXmlElem)elem)->head.children, link3) {
      JXmlNode child3 = (JXmlNode)link3->data;

      if (child3->type == JI_XML_ELEM &&
	  strcmp(jxmlelem_get_name((JXmlElem)child3), "param") == 0) {
	const char* param_name = jxmlelem_get_attr((JXmlElem)child3, "name");
	const char* param_value = jxmlelem_get_attr((JXmlElem)child3, "value");
	params.set(param_name, param_value);
      }
    }
  }

  /* create the item */
  menuitem = menuitem_new(jxmlelem_get_attr(elem, "name"),
			  command, command ? &params: NULL);
  if (!menuitem)
    return NULL;

  /* has it a ID? */
  id = jxmlelem_get_attr(elem, "id");
  if (id != NULL) {
    /* recent list menu */
    if (strcmp(id, "recent_list") == 0) {
      recent_list_menuitem = menuitem;
    }
  }

  /* has it a sub-menu? */
  if (strcmp(jxmlelem_get_name(elem), "menu") == 0) {
    JWidget sub_menu;

    /* create the sub-menu */
    sub_menu = convert_xmlelem_to_menu(elem);
    if (!sub_menu) {
      PRINTF("Error reading the sub-menu\n");
      return menuitem;
    }

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

    if (jwidget_get_type(menuitem) == JI_MENUITEM) {
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
