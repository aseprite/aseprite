/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <stdio.h>
#include <string.h>

#include "jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "core/core.h"
#include "core/dirs.h"
#include "intl/intl.h"
#include "modules/rootmenu.h"
#include "util/filetoks.h"
#include "widgets/menuitem.h"

#endif

static JWidget root_menu;

static JWidget sprite_list_menuitem;
static JWidget recent_list_menuitem;
static JWidget layer_popup_menuitem;
static JWidget frame_popup_menuitem;
static JWidget filters_popup_menuitem;

static JWidget convert_xmlelem_to_menu(JXmlElem elem);
static JWidget convert_xmlelem_to_menuitem(JXmlElem elem);
static void apply_shortcut_to_menuitems_with_command(JWidget menu, Command *command);

int init_module_rootmenu(void)
{
  root_menu = 0;

  return load_root_menu();
}

void exit_module_rootmenu(void)
{
  sprite_list_menuitem = 0;
  recent_list_menuitem = 0;
  layer_popup_menuitem = 0;
  frame_popup_menuitem = 0;
  filters_popup_menuitem = 0;

  command_reset_keys();
  jwidget_free(root_menu);
}

int load_root_menu(void)
{
  JLink link, link2;
  DIRS *dirs, *dir;
  JXmlElem elem;
  JXml xml;

  if (app_get_menu_bar())
    jmenubar_set_menu(app_get_menu_bar(), NULL);

  /* destroy `root-menu' if it exists */
  if (root_menu) {
    command_reset_keys();
    jwidget_free(root_menu);
  }

  /* create a new empty-menu */
  root_menu = NULL;
  sprite_list_menuitem = 0;
  recent_list_menuitem = 0;
  layer_popup_menuitem = 0;
  frame_popup_menuitem = 0;
  filters_popup_menuitem = 0;

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
      /* load menus */
      PRINTF("Trying to menus from \"%s\"...\n", dir->path);

      /* get the <menu> element with id="main_menu" */
      elem = jxml_get_elem_by_id(xml, "main_menu");
      if (elem) {
	/* is it a <menu> element? */
	if (strcmp(jxmlelem_get_name(elem), "menu") == 0) {
	  /* ok, convert it to a menu JWidget */
	  root_menu = convert_xmlelem_to_menu(elem);
	  if (!root_menu) {
	    PRINTF("Error loading main menu from \"%s\" file.\n", dir->path);
	    return -1;
	  }
	}
	else {
	  PRINTF("Invalid element with id=\"main_menu\" in \"%s\"\n", dir->path);
	  return -1;
	}
      }
      else {
	PRINTF("main_menu element couldn't be found...\n");
      }

      /**************************************************/
      /* load keyboard shortcuts */
      PRINTF("Trying to load keyboard shortcuts from \"%s\"...\n", dir->path);

      /* find the <keyboard> element */
      JI_LIST_FOR_EACH(((JXmlNode)jxml_get_root(xml))->children, link) {
	JXmlNode child = (JXmlNode)link->data;

	/* is it <keyboard>? */
	if (child->type == JI_XML_ELEM &&
	    strcmp(jxmlelem_get_name((JXmlElem)child), "keyboard") == 0) {

	  /* for each children in <keyboard>...</keyboard> */
	  JI_LIST_FOR_EACH(child->children, link2) {
	    JXmlNode child2 = (JXmlNode)link2->data;

	    /* it is a <key> element? */
	    if (child2->type == JI_XML_ELEM &&
		strcmp(jxmlelem_get_name((JXmlElem)child2), "key") == 0) {
	      /* finally, we can read the <key /> */
	      const char *command_name = jxmlelem_get_attr((JXmlElem)child2, "command");
	      const char *command_key = jxmlelem_get_attr((JXmlElem)child2, "shortcut");
	      if (command_name && command_key) {
		Command *command = command_get_by_name(command_name);
		if (command) {
		  bool first_shortcut = !command->accel;

		  /* add the keyboard shortcut to the command */
		  command_add_key(command, command_key);

		  /* add the shortcut to the menuitems with this
		     command (this is only visual, the
		     "manager_msg_proc" is the only one that process
		     keyboard shortcuts) */
		  if (first_shortcut)
		    apply_shortcut_to_menuitems_with_command(root_menu, command);
		}
	      }
	    }
	  }
	  break;
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
  if (app_get_menu_bar()) {
    jmenubar_set_menu(app_get_menu_bar(), root_menu);
    jwindow_remap(app_get_top_window());
    jwidget_dirty(app_get_top_window());
  }

  return 0;
}

JWidget get_root_menu(void) { return root_menu; }

JWidget get_sprite_list_menuitem(void) { return sprite_list_menuitem; }
JWidget get_recent_list_menuitem(void) { return recent_list_menuitem; }
JWidget get_layer_popup_menuitem(void) { return layer_popup_menuitem; }
JWidget get_frame_popup_menuitem(void) { return frame_popup_menuitem; }

void show_fx_popup_menu(void)
{
  if (is_interactive() &&
      filters_popup_menuitem &&
      jmenuitem_get_submenu(filters_popup_menuitem)) {
    jmenu_popup(jmenuitem_get_submenu(filters_popup_menuitem),
		jmouse_x(0), jmouse_y(0));
  }
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

  /* create the item */
  menuitem = menuitem_new(jxmlelem_get_attr(elem, "name"),
			  command_get_by_name(jxmlelem_get_attr(elem,
								"command")),
			  NULL);
  if (!menuitem)
    return 0;

  /* has it a ID? */
  id = jxmlelem_get_attr(elem, "id");
  if (id) {
    if (strcmp(id, "sprite_list") == 0) {
      sprite_list_menuitem = menuitem;
    }
    /* recent list menu */
    else if (strcmp(id, "recent_list") == 0) {
      recent_list_menuitem = menuitem;
    }
    /* layer popup menu */
    else if (strcmp(id, "layer_popup") == 0) {
      layer_popup_menuitem = menuitem;
    }
    /* frame popup menu */
    else if (strcmp(id, "frame_popup") == 0) {
      frame_popup_menuitem = menuitem;
    }
    /* filters popup menu */
    else if (strcmp(id, "fx_popup") == 0) {
      filters_popup_menuitem = menuitem;
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

static void apply_shortcut_to_menuitems_with_command(JWidget menu, Command *command)
{
  JList children = jwidget_get_children(menu);
  JWidget menuitem, submenu;
  JLink link;

  JI_LIST_FOR_EACH(children, link) {
    menuitem = (JWidget)link->data;

    if (jwidget_get_type(menuitem) == JI_MENUITEM) {
      if (menuitem_get_command(menuitem) == command) {
	jmenuitem_set_accel(menuitem, jaccel_new_copy(command->accel));
      }

      submenu = jmenuitem_get_submenu(menuitem);
      if (submenu) {
	apply_shortcut_to_menuitems_with_command(submenu, command);
      }
    }
  }

  jlist_free(children);
}
