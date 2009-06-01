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

#include <assert.h>
#include <allegro.h>
/* #include <allegro/internal/aintern.h> */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "ase/ui_context.h"
#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "core/drop_files.h"
#include "core/file_system.h"
#include "core/modules.h"
#include "dialogs/options.h"
#include "dialogs/tips.h"
#include "file/file.h"
#include "intl/intl.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/recent.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "util/boundary.h"
#include "util/recscr.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"
#include "widgets/menuitem.h"
#include "widgets/statebar.h"
#include "widgets/tabs.h"
#include "widgets/toolbar.h"

#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

/* options */
enum {
  OPEN_GFX_FILE,
};

typedef struct Option
{
  int type;
  char *data;
} Option;

typedef struct AppHook
{
  void (*proc)(void *);
  void *data;
} AppHook;

static char *exe_name;		      /* name of the program */

static JList apphooks[APP_EVENTS];

static JWidget top_window = NULL;     /* top level window (the desktop) */
static JWidget box_menubar = NULL;    /* box where the menu bar is */
static JWidget box_colorbar = NULL;   /* box where the color bar is */
static JWidget box_toolbar = NULL;    /* box where the tools bar is */
static JWidget box_statusbar = NULL;  /* box where the status bar is */
static JWidget box_tabsbar = NULL;    /* box where the tabs bar is */
static JWidget menubar = NULL;	      /* the menu bar widget */
static JWidget statusbar = NULL;      /* the status bar widget */
static JWidget colorbar = NULL;	      /* the color bar widget */
static JWidget toolbar = NULL;	      /* the tool bar widget */
static JWidget tabsbar = NULL;	      /* the tabs bar widget */

static JList options; /* list of "Option" structures (options to execute) */
static char *palette_filename = NULL;

static void tabsbar_select_callback(JWidget tabs, void *data, int button);

static int check_args(int argc, char *argv[]);
static void usage(int status);

static Option *option_new(int type, const char *data);
static void option_free(Option *option);

static AppHook *apphook_new(void (*proc)(void *), void *data);
static void apphook_free(AppHook *apphook);

/**
 * Initializes the application loading the modules, setting the
 * graphics mode, loading the configuration and resources, etc.
 */
bool app_init(int argc, char *argv[])
{
  exe_name = argv[0];

  /* initialize application hooks */
  {
    int c;
    for (c=0; c<APP_EVENTS; ++c)
      apphooks[c] = NULL;
  }

  /* initialize language suppport */
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
  if (!gfxobj_init())
    return FALSE;

  /* install the modules */
  if (!modules_init(REQUIRE_INTERFACE))
    return FALSE;

  _ji_font_init();

  /* custom default palette? */
  if (palette_filename) {
    Palette *pal;

    PRINTF("Loading custom palette file: %s\n", palette_filename);

    pal = palette_load(palette_filename);
    if (pal == NULL) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      console_printf(_("Error loading default palette from `%s'\n"),
		     palette_filename);
      return FALSE;
    }

    set_default_palette(pal);
    palette_free(pal);
  }

  /* set system palette to the default one */
  set_current_palette(NULL, TRUE);

  /* ok */
  return TRUE;
}

/**
 * Runs the ASE application. In GUI mode it's the top-level window, in
 * console/scripting it just runs the specified scripts.
 */
void app_loop()
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
    if (!top_window) {
      allegro_message("Error loading data data/jids/main.jid file.\n"
		      "You have to reinstall the program.\n");
      exit(1);
    }

    box_menubar = jwidget_find_name(top_window, "menubar");
    box_editors = jwidget_find_name(top_window, "editor");
    box_colorbar = jwidget_find_name(top_window, "colorbar");
    box_toolbar = jwidget_find_name(top_window, "toolbar");
    box_statusbar = jwidget_find_name(top_window, "statusbar");
    box_tabsbar = jwidget_find_name(top_window, "tabsbar");

    menubar = jmenubar_new();
    statusbar = statusbar_new();
    colorbar = colorbar_new(box_colorbar->align());
    toolbar = toolbar_new();
    tabsbar = tabs_new(tabsbar_select_callback);
    view = editor_view_new();
    editor = create_new_editor();

    /* append the NULL sprite to the tabs */
    tabs_append_tab(tabsbar, "Nothing", NULL);

    /* configure all widgets to expansives */
    jwidget_expansive(menubar, TRUE);
    jwidget_expansive(statusbar, TRUE);
    jwidget_expansive(colorbar, TRUE);
    jwidget_expansive(toolbar, TRUE);
    jwidget_expansive(tabsbar, TRUE);
    jwidget_expansive(view, TRUE);

    /* prepare the first editor */
    jview_attach(view, editor);

    /* setup the menus */
    jmenubar_set_menu(menubar, get_root_menu());

    /* start text of status bar */
    app_default_statusbar_message();

    /* add the widgets in the boxes */
    if (box_menubar) jwidget_add_child(box_menubar, menubar);
    if (box_editors) jwidget_add_child(box_editors, view);
    if (box_colorbar) jwidget_add_child(box_colorbar, colorbar);
    if (box_toolbar) jwidget_add_child(box_toolbar, toolbar);
    if (box_statusbar) jwidget_add_child(box_statusbar, statusbar);
    if (box_tabsbar) jwidget_add_child(box_tabsbar, tabsbar);

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
    option = reinterpret_cast<Option*>(link->data);

    switch (option->type) {

      case OPEN_GFX_FILE: {
	/* load the sprite */
	Sprite *sprite = sprite_load(option->data);
	if (!sprite) {
	  /* error */
	  if (ase_mode & MODE_GUI)
	    jalert(_("Error<<Error loading file \"%s\"||&Close"), option->data);
	  else
	    user_printf(_("Error loading file \"%s\"\n"), option->data);
	}
	else {
	  /* mount and select the sprite */
	  UIContext* context = UIContext::instance();
	  context->add_sprite(sprite);
	  context->set_current_sprite(sprite);

	  if (ase_mode & MODE_GUI) {
	    /* show it */
	    set_sprite_in_more_reliable_editor(context->get_first_sprite());

	    /* recent file */
	    recent_file(option->data);
	  }
	}
	break;
      }
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
    {
      CurrentSprite sprite;
      if (!sprite)
	dialogs_tips(FALSE);
    }

    // support to drop files from Windows explorer
    install_drop_files();

    gui_run();

    uninstall_drop_files();

    /* stop recording */
    if (is_rec_screen())
      rec_screen_off();

    /* remove the root-menu from the menu-bar (because the rootmenu
       module should destroy it) */
    jmenubar_set_menu(menubar, NULL);

    /* destroy the top-window */
    jwidget_free(top_window);
    top_window = NULL;
  }
}

/**
 * Finishes the ASE application.
 */
void app_exit()
{
  JLink link;
  int c;

  /* remove ase handlers */
  PRINTF("Uninstalling ASE\n");

  app_trigger_event(APP_EXIT);

  /* destroy application hooks */
  for (c=0; c<APP_EVENTS; ++c) {
    if (apphooks[c] != NULL) {
      JI_LIST_FOR_EACH(apphooks[c], link) {
	apphook_free(reinterpret_cast<AppHook*>(link->data));
      }
      jlist_free(apphooks[c]);
      apphooks[c] = NULL;
    }
  }

  /* finalize modules, configuration and core */
  modules_exit();
  UIContext::destroy_instance();
  editor_cursor_exit();
  boundary_exit();

  gfxobj_exit();
  ase_config_exit();
  file_system_exit();
  core_exit();
  _ji_font_exit();

  intl_exit();
}

void app_add_hook(int app_event, void (*proc)(void *data), void *data)
{
  assert(app_event >= 0 && app_event < APP_EVENTS);

  if (apphooks[app_event] == NULL)
    apphooks[app_event] = jlist_new();

  jlist_append(apphooks[app_event], apphook_new(proc, data));
}

void app_trigger_event(int app_event)
{
  assert(app_event >= 0 && app_event < APP_EVENTS);

  if (apphooks[app_event] != NULL) {
    JList list = apphooks[app_event];
    JLink link;

    JI_LIST_FOR_EACH(list, link) {
      AppHook *h = (AppHook *)link->data;
      (h->proc)(h->data);
    }
  }
}

/**
 * Updates palette and redraw the screen.
 */
void app_refresh_screen()
{
  if (ase_mode & MODE_GUI) {
    CurrentSprite sprite;

    /* update the color palette */
    set_current_palette(sprite != NULL ?
			sprite_get_palette(sprite, sprite->frame): NULL,
			FALSE);

    /* redraw the screen */
    jmanager_refresh_screen();
  }
}

/**
 * Regenerates the label for each tab in the @em tabsbar.
 */
void app_realloc_sprite_list()
{
  UIContext* context = UIContext::instance();
  const SpriteList& list = context->get_sprite_list();

  /* insert all other sprites */
  for (SpriteList::const_iterator
	 it = list.begin(); it != list.end(); ++it) {
    Sprite* sprite = *it;
    tabs_set_text_for_tab(tabsbar, get_filename(sprite->filename), sprite);
  }
}

/**
 * Updates the recent list menu.
 *
 * @warning This routine can't be used when a menu callback was
 * called, because, it destroy the menus, you should use
 * schedule_rebuild_recent_list() instead (src/modules/gui.c).
 */
bool app_realloc_recent_list()
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
	filename = reinterpret_cast<const char*>(link->data);

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

int app_get_current_image_type()
{
  CurrentSprite sprite;
  if (sprite)
    return sprite->imgtype;
  else if (screen != NULL && bitmap_color_depth(screen) == 8)
    return IMAGE_INDEXED;
  else
    return IMAGE_RGB;
}

JWidget app_get_top_window() { return top_window; }
JWidget app_get_menubar() { return menubar; }
JWidget app_get_statusbar() { return statusbar; }
JWidget app_get_colorbar() { return colorbar; }
JWidget app_get_toolbar() { return toolbar; }
JWidget app_get_tabsbar() { return tabsbar; }

void app_default_statusbar_message()
{
  statusbar_set_text(app_get_statusbar(), 250,
		     "ASE " VERSION ", " COPYRIGHT);
}

int app_get_fg_color(Sprite *sprite)
{
  color_t c = colorbar_get_fg_color(colorbar);
  assert(sprite != NULL);

  if (sprite->layer != NULL)
    return get_color_for_layer(sprite->layer, c);
  else
    return get_color_for_image(sprite->imgtype, c);
}

int app_get_bg_color(Sprite *sprite)
{
  color_t c = colorbar_get_bg_color(colorbar);
  assert(sprite != NULL);

  if (sprite->layer != NULL)
    return get_color_for_layer(sprite->layer, c);
  else
    return get_color_for_image(sprite->imgtype, c);
}

int app_get_color_to_clear_layer(Layer *layer)
{
  /* all transparent layers are cleared with the mask color */
  color_t color = color_mask();
  
  /* the `Background' is erased with the `Background Color' */
  if (layer != NULL && layer_is_background(layer))
    color = colorbar_get_bg_color(colorbar);

  return get_color_for_layer(layer, color);
}

static void tabsbar_select_callback(JWidget tabs, void *data, int button)
{
  // Note: data can be NULL (the "Nothing" tab)
  Sprite* sprite = (Sprite*)data;

  // put as current sprite
  UIContext* context = UIContext::instance();
  context->show_sprite(sprite);

  // middle button: close the sprite
  if (data && (button & 4))
    command_execute(command_get_by_name(CMD_CLOSE_FILE), NULL);
}

/**
 * Looks the inpunt arguments in the command line.
 */
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
      /* use other palette file */
      if (strncmp(arg+n, "palette", len) == 0) {
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

/**
 * Shows the available options for the program
 */
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
       "  -palette GFX-FILE        %s\n"
       "  -resolution WxH[xBPP]    %s\n"
       "  -verbose                 %s\n"
       "  -help                    %s\n"
       "  -version                 %s\n"
       "\n",
       _("Options"),
       _("Use a specific palette by default"),
       _("Change the resolution to use"),
       _("Explain what is being done (in stderr or a log file)"),
       _("Display this help and exits"),
       _("Output version information and exit"));

    /* web-site */
    console_printf
      ("%s: %s\n%s\n\n  %s\n\n",
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

static void option_free(Option* option)
{
  jfree(option->data);
  jfree(option);
}

static AppHook *apphook_new(void (*proc)(void*), void* data)
{
  AppHook* apphook = jnew(AppHook, 1);
  if (!apphook)
    return NULL;

  apphook->proc = proc;
  apphook->data = data;

  return apphook;
}

static void apphook_free(AppHook *apphook)
{
  jfree(apphook);
}
