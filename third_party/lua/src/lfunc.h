/*
** $Id: lfunc.h,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/

#ifndef lfunc_h
#define lfunc_h


#include "lobject.h"


Proto *luaF_newproto (lua_State *L);
Closure *luaF_newCclosure (lua_State *L, int nelems);
Closure *luaF_newLclosure (lua_State *L, int nelems, TObject *gt);
UpVal *luaF_findupval (lua_State *L, StkId level);
void luaF_close (lua_State *L, StkId level);
void luaF_freeproto (lua_State *L, Proto *f);
void luaF_freeclosure (lua_State *L, Closure *c);

const char *luaF_getlocalname (const Proto *func, int local_number, int pc);


#endif
