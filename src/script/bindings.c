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

#include <allegro/gfx.h>

#include "jinete.h"

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "dialogs/colsel.h"
#include "dialogs/dpaledit.h"
#include "dialogs/filesel.h"
#include "effect/colcurve.h"
#include "effect/convmatr.h"
#include "effect/effect.h"
#include "effect/median.h"
#include "effect/replcol.h"
#include "modules/color.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/recent.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "modules/tools2.h"
#include "raster/raster.h"
#include "script/bindings.h"

#endif

/* script objects types */
enum {

  /* graphic objects */
  Type_Image,
  Type_Frame,
  Type_Layer,
  Type_Mask,
  Type_Path,
  Type_Sprite,
  Type_Stock,
  Type_Undo,
  Type_Gfxs = Type_Undo,

  /* effect objects */
  Type_Effect,
  Type_ConvMatr,
  Type_Curve,
  Type_CurvePoint,

  /* gui objects */
/*   Type_JList, */
  Type_JMessage,
  Type_JRect,
  Type_JRegion,
  Type_JWidget,

  /* total of objects */
  Types,
};

/* static RHash *hash_index_tables[Types]; */

typedef struct UserData
{
  int type;
  void *ptr;
  int table;
} UserData;

static int metatable;
static int metatable_index (lua_State *L);
static int metatable_newindex (lua_State *L);
static int metatable_gc (lua_State *L);
static int metatable_eq (lua_State *L);

static void push_userdata (lua_State *L, int type, void *ptr);
static void *to_userdata (lua_State *L, int type, int idx);


/********************************************************************/
/* Global metatable */

/**
 * Registers a global object metatable to handle ASE objects from Lua
 * scripts.
 */
void register_lua_object_metatable(void)
{
  lua_State *L = get_lua_state();
  /* int c; */

  /* class metatable */
  lua_newtable(L);
  metatable = lua_ref (L, 1);

  lua_getref (L, metatable);

  lua_pushliteral(L, "__index");
  lua_pushcfunction(L, metatable_index);
  lua_settable(L, -3);

  lua_pushliteral(L, "__newindex");
  lua_pushcfunction(L, metatable_newindex);
  lua_settable(L, -3);

  lua_pushliteral(L, "__gc");
  lua_pushcfunction(L, metatable_gc);
  lua_settable(L, -3);

  lua_pushliteral(L, "__eq");
  lua_pushcfunction(L, metatable_eq);
  lua_settable(L, -3);

  lua_pop(L, 1);

  /* create hash tables */
/*   for (c=0; c<Types; c++) */
/*     hash_index_tables[c] = r_hash_new (16); */
}

/**
 * Removes the ASE object metatable.
 */
void unregister_lua_object_metatable(void)
{
/*   int c; */

  /* destroy hash tables */
/*   for (c=0; c<Types; c++) */
/*     r_hash_free (hash_index_tables[c], NULL); */

  lua_unref (get_lua_state (), metatable);
}

/**
 * Update global variables in the Lua state.
 */
void update_global_script_variables(void)
{
  lua_State *L = get_lua_state ();

  lua_pushstring (L, VERSION);
  lua_setglobal (L, "VERSION");

  push_userdata (L, Type_Sprite, current_sprite);
  lua_setglobal (L, "current_sprite");

  push_userdata (L, Type_JWidget, current_editor);
  lua_setglobal (L, "current_editor");

  lua_pushnumber (L, ji_screen ? JI_SCREEN_W: 0);
  lua_setglobal (L, "SCREEN_W");

  lua_pushnumber (L, ji_screen ? JI_SCREEN_H: 0);
  lua_setglobal (L, "SCREEN_H");
}

static void push_userdata(lua_State *L, int type, void *ptr)
{
  if (!ptr)
    lua_pushnil (L);
  else {
    UserData *userdata = jnew (UserData, 1);

    userdata->type = type;
    userdata->ptr = ptr;

    /* create object table */
    lua_newtable (L);
    userdata->table = lua_ref (L, 1);

    lua_boxpointer (L, userdata);
    lua_getref (L, metatable);
    lua_setmetatable (L, -2);

    PRINTF ("New lua_Object (%p)\n", userdata->ptr);
  }
}

static void *to_userdata(lua_State *L, int type, int idx)
{
  if (lua_isuserdata (L, idx)) {
    UserData *userdata = (UserData *)lua_unboxpointer (L, idx);
    return userdata->type == type && userdata->ptr ? userdata->ptr : NULL;
  }
  else
    return NULL;
}


/********************************************************************/
/* Metatable methods */

static int metatable_index(lua_State *L)
{
  UserData *userdata = (UserData *)lua_unboxpointer (L, 1);
  const char *index = lua_tostring (L, 2);
/*   lua_CFunction func; */

/*   func = r_hash_find (hash_index_tables[userdata->type], index); */
/*   if (func) */
/*     return (*func) (L); */

  if (strcmp(index, "id") == 0) {
    if (userdata->type <= Type_Gfxs)
      lua_pushnumber(L, ((GfxObj *)userdata->ptr)->id);
  }
  else if (strcmp (index, "type") == 0) {
    if (userdata->type <= Type_Gfxs)
      lua_pushnumber(L, ((GfxObj *)userdata->ptr)->type);
  }
  else {
    switch (userdata->type) {

      case Type_Image: {
	Image *image = userdata->ptr;

	if (strcmp(index, "imgtype") == 0)
	  lua_pushnumber(L, image->imgtype);
	else if (strcmp(index, "w") == 0)
	  lua_pushnumber(L, image->w);
	else if (strcmp(index, "h") == 0)
	  lua_pushnumber(L, image->h);
	else
	  return 0;

	break;
      }

      case Type_Frame: {
	Frame *frame = userdata->ptr;

	if (strcmp (index, "frpos") == 0)
	  lua_pushnumber (L, frame->frpos);
	else if (strcmp (index, "image") == 0)
	  lua_pushnumber (L, frame->image);
	else if (strcmp (index, "x") == 0)
	  lua_pushnumber (L, frame->x);
	else if (strcmp (index, "y") == 0)
	  lua_pushnumber (L, frame->y);
	else if (strcmp (index, "opacity") == 0)
	  lua_pushnumber (L, frame->opacity);
	/* XXXX */
/* 	else if (strcmp (index, "next") == 0) */
/* 	  lua_pushnumber (L, ); */
/* 	else if (strcmp (index, "prev") == 0) */
/* 	  lua_pushnumber (L, ); */
	else
	  return 0;

	break;
      }

      case Type_Layer: {
	Layer *layer = userdata->ptr;

	if (strcmp (index, "name") == 0)
	  lua_pushstring (L, layer->name);
	else if (strcmp (index, "parent") == 0)
	  push_userdata (L,
			 layer->parent->type == GFXOBJ_LAYER_SET ?
			 Type_Layer: Type_Sprite, layer->parent);
	else if (strcmp (index, "readable") == 0)
	  lua_pushboolean (L, layer->readable);
	else if (strcmp (index, "writable") == 0)
	  lua_pushboolean (L, layer->writable);
	else if (strcmp (index, "prev") == 0)
 	  push_userdata (L, Type_Layer, layer_get_prev (layer));
	else if (strcmp (index, "next") == 0)
	  push_userdata (L, Type_Layer, layer_get_next (layer));
	else {
	  switch (layer->gfxobj.type) {
	    case GFXOBJ_LAYER_IMAGE:
	      if (strcmp (index, "imgtype") == 0)
		lua_pushnumber (L, layer->imgtype);
/* 	      else if (strcmp (index, "w") == 0) */
/* 		lua_pushnumber (L, layer->w); */
/* 	      else if (strcmp (index, "h") == 0) */
/* 		lua_pushnumber (L, layer->h); */
	      else if (strcmp (index, "blend_mode") == 0)
		lua_pushnumber (L, layer->blend_mode);
	      else if (strcmp (index, "stock") == 0)
		push_userdata (L, Type_Stock, layer->stock);
	      else if (strcmp (index, "frames") == 0)
		push_userdata (L, Type_Frame,
			       jlist_first_data(layer->frames));
	      else
		return 0;
	      break;
	    case GFXOBJ_LAYER_SET:
	      if (strcmp(index, "layers") == 0)
		push_userdata(L, Type_Layer,
			      jlist_first_data (layer->layers));
	      else
		return 0;
	      break;
	    case GFXOBJ_LAYER_TEXT:
	      /* XXX */
	      return 0;
	      break;
	  }
	}

	break;
      }

      case Type_Mask: {
	Mask *mask = userdata->ptr;

	if (strcmp (index, "x") == 0)
	  lua_pushnumber (L, mask->x);
	else if (strcmp (index, "y") == 0)
	  lua_pushnumber (L, mask->y);
	else if (strcmp (index, "w") == 0)
	  lua_pushnumber (L, mask->w);
	else if (strcmp (index, "h") == 0)
	  lua_pushnumber (L, mask->h);
	else if (strcmp (index, "bitmap") == 0)
	  push_userdata (L, Type_Image, mask->bitmap);
	else
	  return 0;

	break;
      }

      case Type_Path:
	return 0;

      case Type_Sprite: {
	Sprite *sprite = userdata->ptr;

	if (strcmp(index, "filename") == 0)
	  lua_pushstring(L, sprite->filename);
	else if (strcmp(index, "imgtype") == 0)
	  lua_pushnumber(L, sprite->imgtype);
	else if (strcmp(index, "w") == 0)
	  lua_pushnumber(L, sprite->w);
	else if (strcmp(index, "h") == 0)
	  lua_pushnumber(L, sprite->h);
	else if (strcmp(index, "frames") == 0)
	  lua_pushnumber(L, sprite->frames);
	else if (strcmp(index, "frpos") == 0)
	  lua_pushnumber(L, sprite->frpos);
	else if (strcmp(index, "set") == 0)
	  push_userdata(L, Type_Layer, sprite->set);
	else if (strcmp(index, "layer") == 0)
	  push_userdata(L, Type_Layer, sprite->layer);
	else if (strcmp(index, "mask") == 0)
	  push_userdata(L, Type_Mask, sprite->mask);
	else if (strcmp(index, "undo") == 0)
	  push_userdata(L, Type_Undo, sprite->undo);
	else
	  return 0;

	break;
      }

      case Type_Stock: {
	Stock *stock = userdata->ptr;

	if (strcmp (index, "imgtype") == 0)
	  lua_pushnumber (L, stock->imgtype);
	else if (strcmp (index, "nimage") == 0)
	  lua_pushnumber (L, stock->nimage);
	else
	  return 0;

	break;
      }

      case Type_Undo:
	return 0;

      case Type_Effect:
	return 0;

      case Type_ConvMatr:
	return 0;

      case Type_CurvePoint: {
	CurvePoint *point = userdata->ptr;

	if (strcmp (index, "x") == 0)
	  lua_pushnumber (L, point->x);
	else if (strcmp (index, "y") == 0)
	  lua_pushnumber (L, point->y);
	else
	  return 0;

	break;
      }

      case Type_JMessage:
	return 0;

/*       case Type_JList: { */
/* 	JList list = userdata->ptr; */

/* 	if (strcmp(index, "next") == 0) */
/* 	  push_userdata(L, Type_JList, list->next); */
/* 	else */
/* 	  return 0; */

/* 	break; */
/*       } */

      case Type_JRect: {
	JRect rect = userdata->ptr;

	if (strcmp(index, "x1") == 0)
	  lua_pushnumber(L, rect->x1);
	else if (strcmp(index, "y1") == 0)
	  lua_pushnumber(L, rect->y1);
	if (strcmp(index, "x2") == 0)
	  lua_pushnumber(L, rect->x2);
	else if (strcmp(index, "y2") == 0)
	  lua_pushnumber(L, rect->y2);
	else
	  return 0;

	break;
      }

      case Type_JRegion:
	return 0;

      case Type_JWidget:
	return 0;

      default:
	return 0;
    }
  }

  return 1;
}

static int metatable_newindex(lua_State *L)
{
  UserData *userdata = (UserData *)lua_unboxpointer (L, 1);
  const char *index = lua_tostring (L, 2);

  if (strcmp (index, "id") == 0) {
    if (userdata->type <= Type_Gfxs)
      _gfxobj_set_id (((GfxObj *)userdata->ptr), lua_tonumber (L, 3));
    PRINTF ("SET_ID: %d\n", lua_tonumber (L, 3));
  }
  else
    return 0;

  return 1;
}

static int metatable_gc(lua_State *L)
{
  UserData *userdata = (UserData *)lua_unboxpointer (L, 1);

  PRINTF ("Delete lua_Object (%p)\n", userdata->ptr);

  lua_unref (L, userdata->table);
  jfree (userdata);

  return 0;
}

static int metatable_eq (lua_State *L)
{
  UserData *userdata1 = (UserData *)lua_unboxpointer (L, 1);
  UserData *userdata2 = (UserData *)lua_unboxpointer (L, 2);
  void *ptr1 = userdata1 ? userdata1->ptr: NULL;
  void *ptr2 = userdata2 ? userdata2->ptr: NULL;

  lua_pushboolean (L, ptr1 == ptr2);
  return 1;
}


/********************************************************************/
/* Functions and constants bindings */

#include <allegro.h>
#include <math.h>
/* #include "dirs.h" */

#include "util/misc.h"

static void include (const char *filename)
{
  if (filename)
    do_script_file (filename);
}

static void dofile (const char *filename)
{
  if (filename)
    do_script_file (filename);
}

static void print (const char *buf)
{
  if (buf)
    console_printf ("%s\n", buf);
}

static int bind_rand (lua_State *L)
{
  double min = lua_tonumber (L, 1);
  double max = lua_tonumber (L, 2);
  lua_pushnumber (L,
		  ((rand () % (1+((int)max*1000)-((int)min*1000)))
		   + (min*1000.0)) / 1000.0);
  return 1;
}

static int bind_GetImage2 (lua_State *L)
{
  Sprite *sprite;
  Image *image;
  int x, y, opacity;
  sprite = to_userdata (L, Type_Sprite, 1);
  image = GetImage2 (sprite, &x, &y, &opacity);
  push_userdata (L, Type_Image, image);
  lua_pushnumber (L, x);
  lua_pushnumber (L, y);
  lua_pushnumber (L, opacity);
  return 4;
}


/**********************************************************************/
/* Extra GUI routines */

/* bind load_widget() as "ji_load_widget" */
static int bind_ji_load_widget(lua_State *L)
{
  JWidget return_value;
  const char *filename = lua_tostring(L, 1);
  const char *name = lua_tostring(L, 2);
  return_value = load_widget(filename, name); /* use "load_widget" */
  push_userdata(L, Type_JWidget, return_value);
  return 1;
}

/**
 * The routine "ji_file_select" for LUA scripts really call the
 * GUI_FileSelect routine.
 */
static int bind_ji_file_select(lua_State *L)
{
  char *return_value;
  const char *message = lua_tostring(L, 1);
  const char *init_path = lua_tostring(L, 2);
  const char *exts = lua_tostring(L, 3);
  return_value = GUI_FileSelect(message, init_path, exts);
  if (return_value) {
    lua_pushstring(L, return_value);
    jfree(return_value);
  }
  else {
    lua_pushnil(L);
  }
  return 1;
}

static int bind_ji_color_select(lua_State *L)
{
  char *return_value;
  int imgtype = lua_tonumber(L, 1);
  const char *color = lua_tostring(L, 2);
  return_value = ji_color_select(imgtype, color);
  if (return_value) {
    lua_pushstring(L, return_value);
    jfree(return_value);
  }
  else {
    lua_pushnil(L);
  }
  return 1;
}

#if 0

/**********************************************************************/
/* Interface to hook widget's signals with Lua functions **************/

/* The system is very simple, in the 4th slot of widget's user data
   (widget->user_data[3]), we put a list (GList) of Data3 structures,
   each Data3 structure has the references to Lua data.

   First time we hook a signal from Lua, the JI_SIGNAL_DESTROY event
   is hooked with the "destroy_signal_for_widgets" routine, it's
   necessary to free the references (and memory) used by the widget.
   With this, you can't hook the destroy signal with Lua scripts.

   Routine "handle_signals_for_lua_functions" calls the respectives
   scripts routines.
*/

typedef struct Data3
{
  int signal;
  int function;
  int user_data;
} Data3;

static int destroy_signal_for_widgets (JWidget widget, int user_data)
{
  lua_State *L = get_lua_state ();
  Data3 *data3;
  JLink link;

  for (it=widget->user_data[3]; it; it=it->next) {
    data3 = it->data;
    if (data3->function != LUA_NOREF) lua_unref (L, data3->function);
    if (data3->user_data != LUA_NOREF) lua_unref (L, data3->user_data);
    jfree (data3);
  }

  return FALSE;
}

static int handle_signals_for_lua_functions (JWidget widget, int user_data)
{
  lua_State *L = get_lua_state ();
  int ret = FALSE;
  Data3 *data3;
  JLink link;

  for (it=widget->user_data[3]; it; it=it->next) {
    data3 = it->data;

    PRINTF ("CALL HOOK %d %d %d %d\n",
	    widget->id, data3->signal, data3->function, data3->user_data);

    if ((data3->signal == user_data) && (data3->function != LUA_NOREF)) {
      lua_getref (L, data3->function);
      push_userdata (L, Type_JWidget, widget);
      lua_getref (L, data3->user_data);
      do_script_raw (L, 2, 1);
      ret = lua_toboolean (L, -1);
      lua_pop (L, 1);
    }

    PRINTF ("RET VALUE %d\n", ret);
  }

  return ret;
}

static int bind_jwidget_hook_signal (lua_State *L)
{
  JWidget widget = to_userdata (L, Type_JWidget, 1);
  int signal = (int)lua_tonumber (L, 2);
  int user_data = lua_ref (L, 1); /* lua_pop (L, -1) */
  int function = lua_ref (L, 1); /* lua_pop (L, -1) */
  Data3 *data3;
  JHook hook;

  /* check the references */
  function = (function > 0) ? function: LUA_NOREF;
  user_data = (user_data > 0) ? user_data: LUA_NOREF;

  /* is the 4th slot of data is empty, means that it's the first time
     that we hook a signal in this widget */
  if (!widget->user_data[3]) {
    /* hook the destroy signal to clear all the 4th slot data */
    hook = jhook_new (JI_SIGNAL_DESTROY, destroy_signal_for_widgets, 0);
    jwidget_hook_signal (widget, hook);
  }

  /* create a new Data3 block */
  data3 = jmalloc (sizeof (Data3));
  data3->signal = signal;
  data3->function = function;
  data3->user_data = user_data;

  /* append this to the 4th slot */
  widget->user_data[3] = jlist_append (widget->user_data[3], data3);

  /* append the hook to the signal that the user wants to hook */
  hook = jhook_new (data3->signal, handle_signals_for_lua_functions,
		    data3->signal);
  jwidget_hook_signal (widget, hook);

  PRINTF ("NEW HOOK %d %d %d %d\n",
	  widget, data3->signal, data3->function, data3->user_data);

  return 0;
}
#endif

/********************************************************************/
/* Include generated bindings file */

#ifndef USE_PRECOMPILED_HEADER

#include "dialogs/canvasze.h"
#include "dialogs/dmapgen.h"
#include "dialogs/drawtext.h"
#include "dialogs/filmedit.h"
#include "dialogs/maskcol.h"
#include "dialogs/options.h"
#include "dialogs/playfli.h"
#include "dialogs/quick.h"
#include "dialogs/scrsaver.h"
#include "dialogs/tips.h"
#include "dialogs/vectmap.h"
#include "file/file.h"
#include "intl/intl.h"
#include "modules/rootmenu.h"
#include "util/autocrop.h"
#include "util/clipbrd.h"
#include "util/crop.h"
#include "util/frmove.h"
#include "util/mapgen.h"
#include "util/msk_file.h"
#include "util/quantize.h"
#include "util/recscr.h"
#include "util/setgfx.h"

#endif

#define file_exists exists

#include "script/genbinds.c"
