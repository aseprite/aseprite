/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include <allegro.h>
/* #include <allegro/internal/aintern.h> */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jinete.h"
#include "jinete/intern.h"

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "core/modules.h"
#include "core/shutdown.h"
#include "dialogs/options.h"
#include "dialogs/tips.h"
#include "file/file.h"
#include "intl/intl.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/recent.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "script/script.h"
#include "util/recscr.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"
#include "widgets/menu.h"
#include "widgets/statebar.h"
#include "widgets/toolbar.h"

#endif

enum {
  OPEN_GFX_FILE,
  DO_SCRIPT_FILE,
  DO_SCRIPT_EXPR,
};

typedef struct Command
{
  int type;
  char *data;
} Command;

static char *exe_name;		      /* name of the program */

static JWidget top_window = NULL;     /* top level window (the desktop) */
static JWidget box_menu_bar = NULL;   /* box where the menu bar is */
static JWidget box_color_bar = NULL;  /* box where the color bar is */
static JWidget box_tool_bar = NULL;   /* box where the tools bar is */
static JWidget box_status_bar = NULL; /* box where the status bar is */
static JWidget menu_bar = NULL;	      /* the menu bar widget */
static JWidget status_bar = NULL;     /* the status bar widget */
static JWidget color_bar = NULL;      /* the color bar widget */
static JWidget tool_bar = NULL;	      /* the tool bar widget */

static JList commands;		/* list of "Command" structures */
static char *palette_filename = NULL;

static int check_args (int argc, char *argv[]);
static void usage (int status);

static Command *command_new (int type, const char *data);
static void command_free (Command *command);

/* install and load all initial stuff */
int app_init(int argc, char *argv[])
{
  exe_name = argv[0];

  /* Initialize language suppport.  */
  intl_init ();

  /* install the `core' of ASE application */
  if (core_init () < 0) {
    user_printf (_("ASE core initialization error.\n"));
    return -1;
  }

  /* init configuration */
  ase_config_init ();

  /* load the language file */
  intl_load_lang ();

  /* search options in the arguments */
  if (check_args (argc, argv) < 0)
    return -1;

  /* GUI is the default mode */
  if (!(ase_mode & MODE_BATCH))
    ase_mode |= MODE_GUI;

  /* install the modules */
  if (modules_init (ase_mode & MODE_GUI ? REQUIRE_INTERFACE:
					  REQUIRE_SCRIPTING) < 0)
    return -1;

  _ji_font_init ();

  /* custom default palette? */
  if (palette_filename) {
    PALETTE pal;
    BITMAP *bmp;

    PRINTF("Loading custom palette file: %s\n", palette_filename);

    bmp = load_bitmap(palette_filename, pal);
    if (!bmp) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      console_printf(_("Error loading default palette from `%s'\n"),
		     palette_filename);
      return -1;
    }

    destroy_bitmap (bmp);

    set_default_palette (pal);
  }

  /* set system palette to the default one */
  set_current_palette (NULL, TRUE);

  /* add the abnormal program exit handler */
  init_shutdown_handler ();

  /* ok */
  return 0;
}

/* runs ASE main dialog */
void app_loop(void)
{
  Command *command;
  JLink link;

  /* initialize GUI interface */
  if (ase_mode & MODE_GUI) {
    JWidget view, editor;

    PRINTF("GUI mode\n");

    /* setup the GUI screen */
    ji_mouse_set_cursor (JI_CURSOR_NORMAL);
    jmanager_refresh_screen ();

    /* load main window */
    top_window = load_widget ("main.jid", "main_window");
    if (!top_window)
      return;

    box_menu_bar = jwidget_find_name (top_window, "menu_bar");
    box_editors = jwidget_find_name (top_window, "editor");
    box_color_bar = jwidget_find_name (top_window, "color_bar");
    box_tool_bar = jwidget_find_name (top_window, "tool_bar");
    box_status_bar = jwidget_find_name (top_window, "status_bar");

    menu_bar = jmenubar_new ();
    status_bar = status_bar_new ();
    color_bar = color_bar_new (box_color_bar->align);
    tool_bar = tool_bar_new (box_tool_bar->align);
    view = editor_view_new ();
    editor = create_new_editor ();

    /* configure all widgets to expansives */
    jwidget_expansive (menu_bar, TRUE);
    jwidget_expansive (status_bar, TRUE);
    jwidget_expansive (color_bar, TRUE);
    jwidget_expansive (tool_bar, TRUE);
    jwidget_expansive (view, TRUE);

    /* prepare the first editor */
    jview_attach (view, editor);

    /* setup the menus */
    jmenubar_set_menu (menu_bar, get_root_menu ());

    /* start text of status bar */
    app_default_status_bar_message ();

    /* add the widgets in the boxes */
    jwidget_add_child (box_menu_bar, menu_bar);
    jwidget_add_child (box_editors, view);
    jwidget_add_child (box_color_bar, color_bar);
    jwidget_add_child (box_tool_bar, tool_bar);
    jwidget_add_child (box_status_bar, status_bar);

    /* layout */
    if (!get_config_bool ("Layout", "MenuBar", TRUE)) jwidget_hide (menu_bar);
    if (!get_config_bool ("Layout", "StatusBar", TRUE)) jwidget_hide (status_bar);
    if (!get_config_bool ("Layout", "ColorBar", TRUE)) jwidget_hide (color_bar);
    if (!get_config_bool ("Layout", "ToolBar", TRUE)) jwidget_hide (tool_bar);

    /* prepare the window */
    jwindow_remap (top_window);

    /* rebuild menus */
    app_realloc_sprite_list ();
    app_realloc_recent_list ();

    /* set current editor */
    set_current_editor (editor);

    /* open the window */
    jwindow_open (top_window);

    /* refresh the screen */
    jmanager_refresh_screen ();
  }

  /* load all startup scripts */
  load_all_scripts ();

  /* set background mode for non-GUI modes */
  if (!(ase_mode & MODE_GUI))
    set_display_switch_mode (SWITCH_BACKAMNESIA);

  /* procress commands */
  PRINTF("Processing commands...\n");

  JI_LIST_FOR_EACH(commands, link) {
    command = link->data;

    switch (command->type) {

      case OPEN_GFX_FILE: {
	Sprite *sprite;

	/* load the sprite */
	sprite = sprite_load (command->data);
	if (!sprite) {
	  /* error */
	  if (ase_mode & MODE_GUI)
	    jalert (_("Error<<Error loading file \"%s\"||&Close"), command->data);
	  else
	    user_printf (_("Error loading file \"%s\"\n"), command->data);
	}
	else {
	  /* mount and select the sprite */
	  sprite_mount (sprite);
	  set_current_sprite (sprite);

	  if (ase_mode & MODE_GUI) {
	    /* show it */
	    set_sprite_in_more_reliable_editor (get_first_sprite ());

	    /* recent file */
	    recent_file (command->data);
	  }
	}
	break;
      }

      case DO_SCRIPT_FILE:
	do_script_file (command->data);
	break;

      case DO_SCRIPT_EXPR:
	do_script_expr (command->data);
	break;
    }
    command_free(command);
  }

  jlist_free(commands);

  /* just batch mode */
  if (ase_mode & MODE_BATCH) {
    PRINTF("Batch mode\n");
  }
  /* run the GUI */
  else if (ase_mode & MODE_GUI) {
    /* select language */
    GUI_SelectLanguage(FALSE);

    /* show tips? */
    if (!current_sprite)
      GUI_Tips(FALSE);

    run_gui();
  }

  /* destroy GUI widgets */
  if (ase_mode & MODE_GUI) {
    /* stop recording */
    if (is_rec_screen())
      rec_screen_off();

    /* save layout */
    set_config_bool("Layout", "MenuBar", !(menu_bar->flags & JI_HIDDEN));
    set_config_bool("Layout", "StatusBar", !(status_bar->flags & JI_HIDDEN));
    set_config_bool("Layout", "ColorBar", !(color_bar->flags & JI_HIDDEN));
    set_config_bool("Layout", "ToolBar", !(tool_bar->flags & JI_HIDDEN));

    /* remove the root-menu from the menu-bar (because the rootmenu
       module should destroy it) */
    jmenubar_set_menu(menu_bar, NULL);

    /* destroy the top-window */
    jwidget_free(top_window);
    top_window = NULL;
  }
}

/* uninstall ASE */
void app_exit(void)
{
  /* remove ase handlers */
  PRINTF("Uninstalling ASE\n");

  /* finalize modules, configuration and core */
  modules_exit();
  ase_config_exit();
  core_exit();
  _ji_font_exit();

  /* remove the shutdown handler */
  remove_shutdown_handler();

  /* final copyright message */
  if (ase_mode & MODE_GUI) {
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    printf(_("ASE is copyright (C) 2001-2005, 2007 by David A. Capello\n"
	     "Report bugs to <%s>.\n"), BUGREPORT);
  }

  intl_exit();
}

/* updates palette and redraw the screen */
void app_refresh_screen(void)
{
  if (ase_mode & MODE_GUI) {
    /* update the color palette */
    set_current_palette(current_sprite ?
			sprite_get_palette(current_sprite,
					   current_sprite->frpos): NULL,
			FALSE);

    /* redraw the screen */
    jmanager_refresh_screen();
  }
}

/* updates the sprites list menu. WARNING!: This routine can't be used
   when a menu callback was called, because, it destroy some menus,
   you should use rebuild_sprite_list () instead (src/gui/gui.c) */
void app_realloc_sprite_list(void)
{
  JWidget list_menuitem = get_sprite_list_menuitem ();
  JWidget menuitem;
  Sprite *sprite;
  JLink link;

  PRINTF("Reallocating sprite list...\n");

  /* update the sprite-list menu */
  if (list_menuitem) {
    Sprite *clipboard = get_clipboard_sprite ();
    JWidget submenu;
    int c, count = 0;

    submenu = jmenuitem_get_submenu (list_menuitem);
    if (submenu) {
      jmenuitem_set_submenu (list_menuitem, NULL);
      jwidget_free (submenu);
    }

    submenu = jmenu_new ();
    jmenuitem_set_submenu (list_menuitem, submenu);

    /* for `null' */
    menuitem = menuitem_new (_("Nothing"));
    menuitem_set_script (menuitem, "ShowSpriteByID (0)");

    if (!current_sprite)
      jwidget_select (menuitem);

    jwidget_add_child (submenu, menuitem);
    count++;

    /* for `clipboard' */
    menuitem = menuitem_new (_("Clipboard"));
    menuitem_set_script (menuitem, "ShowSpriteByID (%d)",
			 clipboard ? clipboard->gfxobj.id: 0);

    if (!clipboard)
      jwidget_disable (menuitem);
    else if (current_sprite == clipboard)
      jwidget_select (menuitem);

    jwidget_add_child (submenu, menuitem);
    count++;

    /* insert a separator */

    c = 0;
    JI_LIST_FOR_EACH(get_sprite_list(), link) {
      if (link->data == clipboard)
	continue;
      c++;
    }

    if (c > 0) {
      jwidget_add_child (submenu, ji_separator_new (NULL, JI_HORIZONTAL));
      count++;

      /* insert all other sprites */
      JI_LIST_FOR_EACH(get_sprite_list(), link) {
	sprite = link->data;

	if (sprite == clipboard)
	  continue;

	/* `count' limit -- XXXX how I know the height of menu-items? */
/* 	if (count >= SCREEN_H/(text_height (font)+4)-2) { */
	if (count >= 14) {
	  menuitem = menuitem_new (_("More"));
	  jwidget_add_child (submenu, menuitem);

	  submenu = jmenu_new ();
	  jmenuitem_set_submenu (menuitem, submenu);
	  count = 0;
	}

	menuitem = menuitem_new (get_filename (sprite->filename));
	menuitem_set_script (menuitem, "ShowSpriteByID (%d)", sprite->gfxobj.id);

	if (current_sprite == sprite)
	  jwidget_select (menuitem);

	jwidget_add_child (submenu, menuitem);
	count++;
      }
    }
  }
}

/* updates the recent list menu. WARNING!: This routine can't be used
   when a menu callback was called, because, it destroy the menus,
   you should use rebuild_recent_list () instead (src/gui/gui.c). */
void app_realloc_recent_list(void)
{
  JWidget list_menuitem = get_recent_list_menuitem();
  JWidget menuitem;

  PRINTF("Reallocating recent list...\n");

  /* update the recent file list menu item */
  if (list_menuitem) {
    JWidget submenu;

    submenu = jmenuitem_get_submenu (list_menuitem);
    if (submenu) {
      jmenuitem_set_submenu (list_menuitem, NULL);
      jwidget_free (submenu);
    }

    submenu = jmenu_new ();
    jmenuitem_set_submenu (list_menuitem, submenu);

    if (jlist_first(get_recent_files_list())) {
      const char *s, *filename;
      char path[1024];
      JLink link;
      int ch;

      JI_LIST_FOR_EACH(get_recent_files_list(), link) {
	filename = link->data;

	ustrcpy(path, empty_string);

	s = filename;
	while ((ch = ugetxc(&s)))
	  if (ch == '\\')
	    ustrcat(path, "\\\\");
	  else
	    uinsert(path, ustrlen(path), ch);

	menuitem = menuitem_new(get_filename(filename));
	menuitem_set_script(menuitem, "sprite_recent_load (\"%s\")", path);

	jwidget_add_child (submenu, menuitem);
      }
    }
    else {
      menuitem = menuitem_new (_("Nothing"));
      jwidget_disable (menuitem);
      jwidget_add_child (submenu, menuitem);
    }
  }
}

int app_get_current_image_type(void)
{
  if (current_sprite)
    return current_sprite->imgtype;
  else
    return IMAGE_INDEXED;
}

JWidget app_get_top_window(void) { return top_window; }
JWidget app_get_menu_bar(void) { return menu_bar; }
JWidget app_get_status_bar(void) { return status_bar; }
JWidget app_get_color_bar(void) { return color_bar; }
JWidget app_get_tool_bar(void) { return tool_bar; }

void app_switch(JWidget widget)
{
  JWidget parent = jwidget_get_parent(widget);

  if (jwidget_is_visible(widget)) {
    jwidget_hide(widget);
    if (parent)
      jwidget_hide(parent);
  }
  else {
    jwidget_show(widget);
    if (parent)
      jwidget_show(parent);
  }

  jwindow_remap(top_window);
  jwidget_dirty(top_window);
}

void app_default_status_bar_message(void)
{
  status_bar_set_text(app_get_status_bar(), 250,
		      "ASE " VERSION ", "
		      "Copyright (C) 2001-2005 David A. Capello");
}

/* looks the inpunt arguments in the command line */
static int check_args(int argc, char *argv[])
{
  int i, n, len;
  char *arg;

  commands = jlist_new();

  for (i=1; i<argc; i++) {
    arg = argv[i];

    for (n=0; arg[n] == '-'; n++);
    len = strlen (arg+n);

    /* option */
    if ((n > 0) && (len > 0)) {
      /* use in batch mode only */
      if (strncmp (arg+n, "batch", len) == 0) {
        ase_mode |= MODE_BATCH;
      }
      /* do script expression */
      else if (strncmp (arg+n, "exp", len) == 0) {
        if (++i < argc)
          jlist_append(commands, command_new(DO_SCRIPT_EXPR, argv[i]));
        else
          usage (1);
      }
      /* open script file */
      else if (strncmp (arg+n, "file", len) == 0) {
        if (++i < argc)
          jlist_append(commands, command_new(DO_SCRIPT_FILE, argv[i]));
        else
          usage (1);
      }
      /* use other palette file */
      else if (strncmp (arg+n, "palette", len) == 0) {
        if (++i < argc)
	  palette_filename = argv[i];
        else
          usage (1);
      }
      /* video resolution */
      else if (strncmp (arg+n, "resolution", len) == 0) {
        if (++i < argc) {
	  int c, num1=0, num2=0, num3=0;
	  char *tok;

	  /* el próximo argumento debe indicar un formato de
	     resolución algo como esto: 320x240[x8] o [8] */
	  c = 0;

	  for (tok=ustrtok (argv[i], "x"); tok;
	       tok=ustrtok (NULL, "x")) {
	    switch (c) {
	      case 0: num1 = ustrtol (tok, NULL, 10); break;
	      case 1: num2 = ustrtol (tok, NULL, 10); break;
	      case 2: num3 = ustrtol (tok, NULL, 10); break;
	    }
	    c++;
	  }

	  switch (c) {
	    case 1:
	      set_config_int ("GfxMode", "Depth", num1);
	      break;
	    case 2:
	    case 3:
	      set_config_int ("GfxMode", "Width", num1);
	      set_config_int ("GfxMode", "Height", num2);
	      if (c == 3)
		set_config_int ("GfxMode", "Depth", num3);
	      break;
	  }
	}
        else {
	  console_printf(_("%s: option \"res\" requires an argument\n"), exe_name);
          usage(1);
	}
      }
      /* verbose mode */
      else if (strncmp(arg+n, "verbose", len) == 0) {
        ase_mode |= MODE_VERBOSE;
      }
      /* show help */
      else if (strncmp(arg+n, "help", len) == 0) {
        usage(0);
      }
      /* show version */
      else if (strncmp(arg+n, "version", len) == 0) {
        ase_mode |= MODE_BATCH;

        console_printf("ase %s\n", VERSION);
      }
      /* invalid argument */
      else {
        usage(1);
      }
    }
    /* graphic file to open */
    else if (n == 0)
      jlist_append(commands, command_new(OPEN_GFX_FILE, argv[i]));
  }

  return 0;
}

/* shows the available options for the program */
static void usage(int status)
{
  /* show options */
  if (!status) {
    /* copyright */
    console_printf
      ("ase %s -- allegro-sprite-editor, %s\n"
       "Copyright (C) 2001-2005, 2007 David A. Capello\n\n",
       VERSION, _("The Ultimate Sprites Factory"));

    /* usage */
    console_printf
      ("%s\n  %s [%s] [%s]...\n\n",
       _("Usage:"), exe_name, _("OPTION"), _("FILE"));

    /* options */
    console_printf
      ("%s:\n"
       "  -batch                   %s\n"
       "  -exp SCRIPT-EXPRESSION   %s\n"
       "  -file SCRIPT-FILE        %s\n"
       "  -palette GFX-FILE        %s\n"
       "  -resolution WxH[xBPP]    %s\n"
       "  -verbose                 %s\n"
       "  -help                    %s\n"
       "  -version                 %s\n"
       "\n",
       _("Options"),
       _("Just process commands in the arguments"),
       _("Process a script expression in the given string"),
       _("Process a script file"),
       _("Use a specify palette by default"),
       _("Change the resolution to use"),
       _("Explain what is being done (in stderr or a log file)"),
       _("Display this help and exits"),
       _("Output version information and exit"));

    /* bugs and web-site */
    console_printf
      ("%s: %s\n%s\n\n  %s\n\n",
       _("Report bugs to"), BUGREPORT,
       _("Find more information in the ASE's official web site at:"), WEBSITE);
  }
  /* how to show options */
  else {
    console_printf (_("Try \"%s --help\" for more information.\n"), exe_name);
  }
  exit (status);
}

static Command *command_new (int type, const char *data)
{
  Command *command;

  command = jnew (Command, 1);
  if (!command)
    return NULL;

  command->type = type;
  command->data = jstrdup (data);

  return command;
}

static void command_free (Command *command)
{
  jfree (command->data);
  jfree (command);
}
