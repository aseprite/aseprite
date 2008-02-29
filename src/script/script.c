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

#include <assert.h>
#include <allegro.h>

#include "jinete/jlist.h"

#include "console/console.h"
#include "core/core.h"
#include "core/dirs.h"
#include "modules/editors.h"
#include "modules/sprites.h"
#include "modules/tools2.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "script/bindings.h"
#include "script/script.h"
#include "widgets/editor.h"

/* Global Lua state.  */

static lua_State *L;

/* Level of executed scripts (running == 0 means that we aren't
   processing scripts).  */

static int running = 0;

static JList sprites_of_users = NULL;

static void prepare(void);
static void release(void);

/* Installs all the scripting capability.  */

int init_module_script(void)
{
  /* create the main lua state */
  L = lua_open();
  if (!L)
    return -1;

  /* setup the garbage collector */
  lua_setgcthreshold(L, 0);

  /* install the base library */
  lua_baselibopen(L);
/*   lua_tablibopen(L); */
/*   lua_iolibopen(L); */
/*   lua_strlibopen(L); */
/*   lua_mathlibopen(L); */
/*   lua_dblibopen(L); */

  /* export C routines to use in Lua scripts */
  register_bindings(L);

  /* object metatable */
  register_lua_object_metatable();

  return 0;
}

/* Removes the scripting feature.  */

void exit_module_script(void)
{
  /* remove object metatable */
  unregister_lua_object_metatable();

  /* delete Lua state */
  lua_close(L);
  L = NULL;
}

lua_State *get_lua_state(void)
{
  return L;
}

int script_is_running(void)
{
  return (running > 0) ? TRUE: FALSE;
}

void script_show_err(lua_State *L, int err)
{
  switch (err) {
    case 0:
      /* nothing */
      break;
    case LUA_ERRRUN:
      console_printf("** lua error (run): %s\n", lua_tostring(L, -1));
      break;
    case LUA_ERRFILE:
      console_printf("** lua error (file): %s\n", lua_tostring(L, -1));
      break;
    case LUA_ERRSYNTAX:
      console_printf("** lua error (syntax): %s\n", lua_tostring(L, -1));
      break;
    case LUA_ERRMEM:
      console_printf("** lua error (mem): %s\n", lua_tostring(L, -1));
      break;
    case LUA_ERRERR:
      console_printf("** lua error (err): %s\n", lua_tostring(L, -1));
      break;
    default:
      console_printf("** lua error (unknown): %s\n", lua_tostring(L, -1));
      break;
  }
}

int do_script_raw(lua_State *L, int nargs, int nresults)
{
  int err;

  prepare();

  err = lua_pcall(L, nargs, nresults, 0);
  if (err != 0)
    script_show_err(L, err);

  release();

  return err;
}

int do_script_expr(const char *exp)
{
  int err = luaL_loadbuffer(L, exp, ustrlen(exp), exp);

  prepare();

  if (err == 0)
    err = lua_pcall(L, 0, LUA_MULTRET, 0);

  if (err != 0)
    script_show_err(L, err);

  release();

  return err;
}

int do_script_file(const char *filename)
{
  int found = FALSE;
  char buf[512];
  int err;

  PRINTF("Calling script file \"%s\"\n", filename);

  if (exists(filename)) {
    ustrcpy(buf, filename);
    found = TRUE;
  }
  else {
    DIRS *it, *dirs;

    usprintf(buf, "scripts/%s", filename);
    dirs = filename_in_datadir(buf);
    for (it=dirs; it; it=it->next) {
      if (exists(it->path)) {
	ustrcpy(buf, it->path);
	found = TRUE;
	break;
      }
    }

    dirs_free(dirs);
  }

  if (!found) {
    console_printf(_("File not found: \"%s\"\n"), filename);
    return -1;
  }

  prepare();

  err = luaL_loadfile(L, buf);

  if (err == 0)
    err = lua_pcall(L, 0, LUA_MULTRET, 0);

  if (err != 0)
    script_show_err(L, err);

  release();

  return err;
}

/**
 * Prepare the application to run a script.
 */
static void prepare(void)
{
  if (running == 0) {
    JList all_sprites = get_sprite_list();

    /* reset configuration of tools for the script */
    ResetConfig();

    /* open console */
    console_open();

    /* make a copy of the list of sprites to known which sprites where
       created with the script */
    sprites_of_users = jlist_copy(all_sprites);
  }
  running++;
}

/**
 * Restore the configuration of the application after running a
 * script.
 */
static void release(void)
{
  running--;
  if (running == 0) {
    JList all_sprites = get_sprite_list();
    JLink link;

    /* restore tools configuration for the user */
    RestoreConfig();

    /* close the console */
    console_close();

    /* set current sprite */
    if (current_editor)
      set_current_sprite(editor_get_sprite(current_editor));

    /* check what sprites are new (to enable Undo support), to do this
       we must to  */
    JI_LIST_FOR_EACH(all_sprites, link) {
      Sprite *sprite = link->data;

      /* if the sprite isn't in the old list of sprites ... */
      if (!jlist_find(sprites_of_users, sprite)) {
	/* ...the sprite was created by the script, so the undo is
	   disabled */
	assert(undo_is_disabled(sprite->undo));

	/* this is a sprite created in the script... see the functions
	   NewSprite or LoadSprite to known where the undo is
	   disabled */
	undo_enable(sprite->undo);
      }
    }
    jlist_free(sprites_of_users);
  }
}
