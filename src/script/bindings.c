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

#include <allegro/gfx.h>

#include "jinete/jinete.h"

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
#include "script/functions.h"

/* script objects types */
enum {

  /* graphic objects */
  Type_Image,
  Type_Cel,
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
static int metatable_index(lua_State *L);
static int metatable_newindex(lua_State *L);
static int metatable_gc(lua_State *L);
static int metatable_eq(lua_State *L);

static void push_userdata(lua_State *L, int type, void *ptr);
static void *to_userdata(lua_State *L, int type, int idx);


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
  metatable = lua_ref(L, 1);

  lua_getref(L, metatable);

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

  lua_unref(get_lua_state(), metatable);
}

/**
 * Update global variables in the Lua state.
 */
void update_global_script_variables(void)
{
  lua_State *L = get_lua_state();

  lua_pushstring(L, VERSION);
  lua_setglobal(L, "VERSION");

  push_userdata(L, Type_Sprite, current_sprite);
  lua_setglobal(L, "current_sprite");

  push_userdata(L, Type_JWidget, current_editor);
  lua_setglobal(L, "current_editor");

  lua_pushnumber(L, ji_screen ? JI_SCREEN_W: 0);
  lua_setglobal(L, "SCREEN_W");

  lua_pushnumber(L, ji_screen ? JI_SCREEN_H: 0);
  lua_setglobal(L, "SCREEN_H");
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

      case Type_Cel: {
	Cel *cel = userdata->ptr;

	if (strcmp(index, "frame") == 0)
	  lua_pushnumber(L, cel->frame);
	else if (strcmp(index, "image") == 0)
	  lua_pushnumber(L, cel->image);
	else if (strcmp(index, "x") == 0)
	  lua_pushnumber(L, cel->x);
	else if (strcmp(index, "y") == 0)
	  lua_pushnumber(L, cel->y);
	else if (strcmp(index, "opacity") == 0)
	  lua_pushnumber(L, cel->opacity);
	/* TODO */
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

	if (strcmp(index, "name") == 0)
	  lua_pushstring(L, layer->name);
	else if (strcmp(index, "parent") == 0)
	  push_userdata(L,
			layer->parent->type == GFXOBJ_LAYER_SET ?
			Type_Layer: Type_Sprite, layer->parent);
	else if (strcmp(index, "readable") == 0)
	  lua_pushboolean(L, layer->readable);
	else if (strcmp(index, "writable") == 0)
	  lua_pushboolean(L, layer->writable);
	else if (strcmp(index, "prev") == 0)
 	  push_userdata(L, Type_Layer, layer_get_prev(layer));
	else if (strcmp(index, "next") == 0)
	  push_userdata(L, Type_Layer, layer_get_next(layer));
	else {
	  switch (layer->gfxobj.type) {
	    case GFXOBJ_LAYER_IMAGE:
	      if (strcmp(index, "imgtype") == 0)
		lua_pushnumber(L, layer->sprite->imgtype);
/* 	      else if (strcmp (index, "w") == 0) */
/* 		lua_pushnumber (L, layer->w); */
/* 	      else if (strcmp (index, "h") == 0) */
/* 		lua_pushnumber (L, layer->h); */
	      else if (strcmp(index, "blend_mode") == 0)
		lua_pushnumber(L, layer->blend_mode);
	      else if (strcmp(index, "cels") == 0)
		push_userdata(L, Type_Cel,
			      jlist_first_data(layer->cels));
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
	else if (strcmp(index, "stock") == 0)
	  push_userdata(L, Type_Stock, sprite->stock);
	else if (strcmp(index, "frames") == 0)
	  lua_pushnumber(L, sprite->frames);
	else if (strcmp(index, "frame") == 0)
	  lua_pushnumber(L, sprite->frame);
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
  UserData *userdata = (UserData *)lua_unboxpointer(L, 1);

  PRINTF("Delete lua_Object (%p)\n", userdata->ptr);

  lua_unref(L, userdata->table);
  jfree(userdata);

  return 0;
}

static int metatable_eq(lua_State *L)
{
  UserData *userdata1 = (UserData *)lua_unboxpointer(L, 1);
  UserData *userdata2 = (UserData *)lua_unboxpointer(L, 2);
  void *ptr1 = userdata1 ? userdata1->ptr: NULL;
  void *ptr2 = userdata2 ? userdata2->ptr: NULL;

  lua_pushboolean(L, ptr1 == ptr2);
  return 1;
}


/********************************************************************/
/* Functions and constants bindings */

#include <allegro.h>
#include <math.h>
/* #include "dirs.h" */

#include "util/misc.h"

static void include(const char *filename)
{
  if (filename)
    do_script_file(filename);
}

static void dofile(const char *filename)
{
  if (filename)
    do_script_file(filename);
}

static void print(const char *buf)
{
  if (buf)
    console_printf("%s\n", buf);
}

static int bind_rand(lua_State *L)
{
  double min = lua_tonumber(L, 1);
  double max = lua_tonumber(L, 2);
  lua_pushnumber(L,
		 ((rand() % (1+((int)max*1000)-((int)min*1000)))
		  + (min*1000.0)) / 1000.0);
  return 1;
}

/* static int bind_GetImage2(lua_State *L) */
/* { */
/*   Sprite *sprite; */
/*   Image *image; */
/*   int x, y, opacity; */
/*   sprite = to_userdata(L, Type_Sprite, 1); */
/*   image = GetImage2(sprite, &x, &y, &opacity); */
/*   push_userdata(L, Type_Image, image); */
/*   lua_pushnumber(L, x); */
/*   lua_pushnumber(L, y); */
/*   lua_pushnumber(L, opacity); */
/*   return 4; */
/* } */

/********************************************************************/
/* Include generated bindings file */

#define file_exists exists

#include "script/genbinds.c"
