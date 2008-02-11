/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include <allegro.h>
/* #include <allegro/internal/aintern.h> */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "core/file_system.h"
#include "core/modules.h"
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
#include "widgets/menuitem.h"
#include "widgets/statebar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"

/* options */
enum {
  OPEN_GFX_FILE,
  DO_SCRIPT_FILE,
  DO_SCRIPT_EXPR,
};

typedef struct Option
{
  int type;
  char *data;
} Option;

static char *exe_name;		      /* name of the program */

static JWidget top_window = NULL;     /* top level window (the desktop) */
static JWidget box_menu_bar = NULL;   /* box where the menu bar is */
static JWidget box_color_bar = NULL;  /* box where the color bar is */
static JWidget box_tool_bar = NULL;   /* box where the tools bar is */
static JWidget box_status_bar = NULL; /* box where the status bar is */
static JWidget box_tabs_bar = NULL;   /* box where the tabs bar is */
static JWidget menu_bar = NULL;	      /* the menu bar widget */
static JWidget status_bar = NULL;     /* the status bar widget */
static JWidget color_bar = NULL;      /* the color bar widget */
static JWidget tool_bar = NULL;	      /* the tool bar widget */
static JWidget tabs_bar = NULL;	      /* the tabs bar widget */

static JList options; /* list of "Option" structures (options to execute) */
static char *palette_filename = NULL;

static int check_args(int argc, char *argv[]);
static void usage(int status);

static Option *option_new(int type, const char *data);
static void option_free(Option *option);

/* install and load all initial stuff */
bool app_init(int argc, char *argv[])
{
  exe_name = argv[0];

  /* Initialize language suppport.  */
  intl_init();

  /* install the `core' of ASE application */
  if (!core_init()) {
    user_printf(_("ASE core initialization error.\n"));
    return FALSE;
  }

  /* install the file-system access module */
  if (!file_system_init())
    return FALSE;

  /* init configuration */
  ase_config_init();

  /* load the language file */
  intl_load_lang();

  /* search options in the arguments */
  if (check_args(argc, argv) < 0)
    return FALSE;

  /* GUI is the default mode */
  if (!(ase_mode & MODE_BATCH))
    ase_mode |= MODE_GUI;

  /* install 'raster' stuff */
  if (!gfxobj_init()) {
    return FALSE;
  }

  /* install the modules */
  if (!modules_init(ase_mode & MODE_GUI ? REQUIRE_INTERFACE:
					  REQUIRE_SCRIPTING))
    return FALSE;

  _ji_font_init();

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
      return FALSE;
    }

    destroy_bitmap(bmp);

    set_default_palette(pal);
  }

  /* set system palette to the default one */
  set_current_palette(NULL, TRUE);

  /* ok */
  return TRUE;
}

/* runs ASE main dialog */
void app_loop(void)
{
  Option *option;
  JLink link;

  /* initialize GUI interface */
  if (ase_mode & MODE_GUI) {
    JWidget view, editor;

    PRINTF("GUI mode\n");

    /* setup the GUI screen */
    jmouse_set_cursor(JI_CURSOR_NORMAL);
    jmanager_refresh_screen();

    /* load main window */
    top_window = load_widget("main.jid", "main_window");
    if (!top_window)
      return;

    box_menu_bar = jwidget_find_name(top_window, "menu_bar");
    box_editors = jwidget_find_name(top_window, "editor");
    box_color_bar = jwidget_find_name(top_window, "color_bar");
    box_tool_bar = jwidget_find_name(top_window, "tool_bar");
    box_status_bar = jwidget_find_name(top_window, "status_bar");
    box_tabs_bar = jwidget_find_name(top_window, "tabs_bar");

    menu_bar = jmenubar_new();
    status_bar = status_bar_new();
    color_bar = color_bar_new(box_color_bar->align);
    tool_bar = tool_bar_new(box_tool_bar->align);
    tabs_bar = tabs_new(sprite_show);
    view = editor_view_new();
    editor = create_new_editor();

    /* append the NULL sprite to the tabs */
    tabs_append_tab(tabs_bar, "Nothing", NULL);

    /* configure all widgets to expansives */
    jwidget_expansive(menu_bar, TRUE);
    jwidget_expansive(status_bar, TRUE);
    jwidget_expansive(color_bar, TRUE);
    jwidget_expansive(tool_bar, TRUE);
    jwidget_expansive(tabs_bar, TRUE);
    jwidget_expansive(view, TRUE);

    /* prepare the first editor */
    jview_attach(view, editor);

    /* setup the menus */
    jmenubar_set_menu(menu_bar, get_root_menu());

    /* start text of status bar */
    app_default_status_bar_message();

    /* add the widgets in the boxes */
    if (box_menu_bar) jwidget_add_child(box_menu_bar, menu_bar);
    if (box_editors) jwidget_add_child(box_editors, view);
    if (box_color_bar) jwidget_add_child(box_color_bar, color_bar);
    if (box_tool_bar) jwidget_add_child(box_tool_bar, tool_bar);
    if (box_status_bar) jwidget_add_child(box_status_bar, status_bar);
    if (box_tabs_bar) jwidget_add_child(box_tabs_bar, tabs_bar);

    /* prepare the window */
    jwindow_remap(top_window);

    /* rebuild menus */
    app_realloc_sprite_list();
    app_realloc_recent_list();

    /* set current editor */
    set_current_editor(editor);

    /* open the window */
    jwindow_open(top_window);

    /* refresh the screen */
    jmanager_refresh_screen();
  }

  /* set background mode for non-GUI modes */
/*   if (!(ase_mode & MODE_GUI)) */
/*     set_display_switch_mode(SWITCH_BACKAMNESIA); */
    set_display_switch_mode(SWITCH_BACKGROUND);

  /* procress options */
  PRINTF("Processing options...\n");

  JI_LIST_FOR_EACH(options, link) {
    option = link->data;

    switch (option->type) {

      case OPEN_GFX_FILE: {
	Sprite *sprite;

	/* load the sprite */
	sprite = sprite_load(option->data);
	if (!sprite) {
	  /* error */
	  if (ase_mode & MODE_GUI)
	    jalert(_("Error<<Error loading file \"%s\"||&Close"), option->data);
	  else
	    user_printf(_("Error loading file \"%s\"\n"), option->data);
	}
	else {
	  /* mount and select the sprite */
	  sprite_mount(sprite);
	  set_current_sprite(sprite);

	  if (ase_mode & MODE_GUI) {
	    /* show it */
	    set_sprite_in_more_reliable_editor(get_first_sprite());

	    /* recent file */
	    recent_file(option->data);
	  }
	}
	break;
      }

      case DO_SCRIPT_FILE:
	do_script_file(option->data);
	break;

      case DO_SCRIPT_EXPR:
	do_script_expr(option->data);
	break;
    }
    option_free(option);
  }

  jlist_free(options);

  /* just batch mode */
  if (ase_mode & MODE_BATCH) {
    PRINTF("Batch mode\n");
  }
  /* run the GUI */
  else if (ase_mode & MODE_GUI) {
    /* select language */
    dialogs_select_language(FALSE);

    /* show tips? */
    if (!current_sprite)
      dialogs_tips(FALSE);

    gui_run();
  }

  /* destroy GUI widgets */
  if (ase_mode & MODE_GUI) {
    /* stop recording */
    if (is_rec_screen())
      rec_screen_off();

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
  gfxobj_exit();
  ase_config_exit();
  file_system_exit();
  core_exit();
  _ji_font_exit();

  /* final copyright message */
  if (ase_mode & MODE_GUI) {
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    printf("ASE - " COPYRIGHT "\n%s <%s>.\n",
	   _("Report bugs to"), BUGREPORT);
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
					   current_sprite->frame): NULL,
			FALSE);

    /* redraw the screen */
    jmanager_refresh_screen();
  }
}

void app_realloc_sprite_list(void)
{
  Sprite *sprite;
  JLink link;

  /* insert all other sprites */
  JI_LIST_FOR_EACH(get_sprite_list(), link) {
    sprite = link->data;
    tabs_set_text_for_tab(tabs_bar,
			  get_filename(sprite->filename),
			  sprite);
  }
}

/* updates the recent list menu. WARNING!: This routine can't be used
   when a menu callback was called, because, it destroy the menus,
   you should use rebuild_recent_list() instead (src/gui/gui.c). */
bool app_realloc_recent_list(void)
{
  JWidget list_menuitem = get_recent_list_menuitem();
  JWidget menuitem;

  /* update the recent file list menu item */
  if (list_menuitem) {
    Command *cmd_open_file;
    JWidget submenu;

    if (jmenuitem_has_submenu_opened(list_menuitem))
      return FALSE;

    cmd_open_file = command_get_by_name(CMD_OPEN_FILE);

    submenu = jmenuitem_get_submenu(list_menuitem);
    if (submenu) {
      jmenuitem_set_submenu(list_menuitem, NULL);
      jwidget_free(submenu);
    }

    submenu = jmenu_new();
    jmenuitem_set_submenu(list_menuitem, submenu);

    if (jlist_first(get_recent_files_list())) {
      const char *filename;
      JLink link;

      JI_LIST_FOR_EACH(get_recent_files_list(), link) {
	filename = link->data;

	menuitem = menuitem_new(get_filename(filename),
				cmd_open_file,
				filename);
	jwidget_add_child(submenu, menuitem);
      }
    }
    else {
      menuitem = menuitem_new(_("Nothing"), NULL, NULL);
      jwidget_disable(menuitem);
      jwidget_add_child(submenu, menuitem);
    }
  }

  return TRUE;
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
JWidget app_get_tabs_bar(void) { return tabs_bar; }

void app_default_status_bar_message(void)
{
  status_bar_set_text(app_get_status_bar(), 250,
		      "ASE " VERSION ", " COPYRIGHT);
}

/* looks the inpunt arguments in the command line */
static int check_args(int argc, char *argv[])
{
  int i, n, len;
  char *arg;

  options = jlist_new();

  for (i=1; i<argc; i++) {
    arg = argv[i];

    for (n=0; arg[n] == '-'; n++);
    len = strlen(arg+n);

    /* option */
    if ((n > 0) && (len > 0)) {
      /* use in batch mode only */
      if (strncmp(arg+n, "batch", len) == 0) {
        ase_mode |= MODE_BATCH;
      }
      /* do script expression */
      else if (strncmp(arg+n, "exp", len) == 0) {
        if (++i < argc)
          jlist_append(options, option_new(DO_SCRIPT_EXPR, argv[i]));
        else
          usage(1);
      }
      /* open script file */
      else if (strncmp(arg+n, "file", len) == 0) {
        if (++i < argc)
          jlist_append(options, option_new(DO_SCRIPT_FILE, argv[i]));
        else
          usage(1);
      }
      /* use other palette file */
      else if (strncmp(arg+n, "palette", len) == 0) {
        if (++i < argc)
	  palette_filename = argv[i];
        else
          usage(1);
      }
      /* video resolution */
      else if (strncmp(arg+n, "resolution", len) == 0) {
        if (++i < argc) {
	  int c, num1=0, num2=0, num3=0;
	  char *tok;

	  /* el próximo argumento debe indicar un formato de
	     resolución algo como esto: 320x240[x8] o [8] */
	  c = 0;

	  for (tok=ustrtok(argv[i], "x"); tok;
	       tok=ustrtok(NULL, "x")) {
	    switch (c) {
	      case 0: num1 = ustrtol(tok, NULL, 10); break;
	      case 1: num2 = ustrtol(tok, NULL, 10); break;
	      case 2: num3 = ustrtol(tok, NULL, 10); break;
	    }
	    c++;
	  }

	  switch (c) {
	    case 1:
	      set_config_int("GfxMode", "Depth", num1);
	      break;
	    case 2:
	    case 3:
	      set_config_int("GfxMode", "Width", num1);
	      set_config_int("GfxMode", "Height", num2);
	      if (c == 3)
		set_config_int("GfxMode", "Depth", num3);
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
      jlist_append(options, option_new(OPEN_GFX_FILE, argv[i]));
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
       COPYRIGHT "\n\n",
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
    console_printf(_("Try \"%s --help\" for more information.\n"), exe_name);
  }
  exit(status);
}

static Option *option_new(int type, const char *data)
{
  Option *option;

  option = jnew(Option, 1);
  if (!option)
    return NULL;

  option->type = type;
  option->data = jstrdup(data);

  return option;
}

static void option_free(Option *option)
{
  jfree(option->data);
  jfree(option);
}
