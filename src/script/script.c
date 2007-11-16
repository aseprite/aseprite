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

#include <allegro.h>

#include "console/console.h"
#include "core/core.h"
#include "core/dirs.h"
#include "modules/editors.h"
#include "modules/sprites.h"
#include "script/bindings.h"
#include "script/script.h"
#include "widgets/editor.h"

#endif

/* Global Lua state.  */

static lua_State *L;

/* Level of executed scripts (running == 0 means that we aren't
   processing scripts).  */

static int running = 0;

static void prepare (void);
static void release (void);

/* Installs all the scripting capability.  */

int init_module_script (void)
{
  /* create the main lua state */
  L = lua_open ();
  if (!L)
    return -1;

  /* setup the garbage collector */
  lua_setgcthreshold (L, 0);

  /* install the base library */
  lua_baselibopen (L);
/*   lua_tablibopen (L); */
/*   lua_iolibopen (L); */
/*   lua_strlibopen (L); */
/*   lua_mathlibopen (L); */
/*   lua_dblibopen (L); */

  /* export C routines to use in Lua scripts */
  register_bindings (L);

  /* object metatable */
  register_lua_object_metatable ();

  return 0;
}

/* Removes the scripting feature.  */

void exit_module_script (void)
{
  /* remove object metatable */
  unregister_lua_object_metatable ();

  /* delete Lua state */
  lua_close (L);
  L = NULL;
}

lua_State *get_lua_state (void)
{
  return L;
}

int script_is_running (void)
{
  return (running > 0) ? TRUE: FALSE;
}

void script_show_err (lua_State *L, int err)
{
  switch (err) {
    case 0:
      /* nothing */
      break;
    case LUA_ERRRUN:
      console_printf ("** lua error (run): %s\n", lua_tostring (L, -1));
      break;
    case LUA_ERRFILE:
      console_printf ("** lua error (file): %s\n", lua_tostring (L, -1));
      break;
    case LUA_ERRSYNTAX:
      console_printf ("** lua error (syntax): %s\n", lua_tostring (L, -1));
      break;
    case LUA_ERRMEM:
      console_printf ("** lua error (mem): %s\n", lua_tostring (L, -1));
      break;
    case LUA_ERRERR:
      console_printf ("** lua error (err): %s\n", lua_tostring (L, -1));
      break;
    default:
      console_printf ("** lua error (unknown): %s\n", lua_tostring (L, -1));
      break;
  }
}

int do_script_raw (lua_State *L, int nargs, int nresults)
{
  int err;

  prepare ();

  err = lua_pcall (L, nargs, nresults, 0);
  if (err != 0)
    script_show_err (L, err);

  release ();

  return err;
}

int do_script_expr (const char *exp)
{
  int err = luaL_loadbuffer (L, exp, ustrlen (exp), exp);

  prepare ();

  if (err == 0)
    err = lua_pcall (L, 0, LUA_MULTRET, 0);

  if (err != 0)
    script_show_err (L, err);

  release ();

  return err;
}

int do_script_file (const char *filename)
{
  int found = FALSE;
  char buf[512];
  int err;

  PRINTF ("Calling script file \"%s\"\n", filename);

  if (exists (filename)) {
    ustrcpy (buf, filename);
    found = TRUE;
  }
  else {
    DIRS *it, *dirs;

    usprintf (buf, "scripts/%s", filename);
    dirs = filename_in_datadir (buf);
    for (it=dirs; it; it=it->next) {
      if (exists (it->path)) {
	ustrcpy (buf, it->path);
	found = TRUE;
	break;
      }
    }

    dirs_free (dirs);
  }

  if (!found) {
    console_printf (_("File not found: \"%s\"\n"), filename);
    return -1;
  }

  prepare ();

  err = luaL_loadfile (L, buf);

  if (err == 0)
    err = lua_pcall (L, 0, LUA_MULTRET, 0);

  if (err != 0)
    script_show_err (L, err);

  release ();

  return err;
}

void load_all_scripts (void)
{
  struct al_ffblk fi;
  DIRS *it, *dirs;
  char buf[512];
  int done;

  dirs = filename_in_datadir ("scripts/*.lua");
  for (it=dirs; it; it=it->next) {
    done = al_findfirst (it->path, &fi, FA_RDONLY | FA_SYSTEM | FA_ARCH);
    while (!done) {
      replace_filename (buf, it->path, fi.name, sizeof (buf));
      do_script_file (buf);
      done = al_findnext (&fi);
    }
    al_findclose (&fi);
  }
  dirs_free (dirs);
}

static void prepare (void)
{
  if (running == 0) {
    console_open ();
    update_global_script_variables ();
  }
  running++;
}

static void release (void)
{
  running--;
  if (running == 0) {
    console_close ();
    if (current_editor)
      set_current_sprite (editor_get_sprite (current_editor));
  }
}
