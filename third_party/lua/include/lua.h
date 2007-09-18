/*
** $Id: lua.h,v 1.1 2004/03/09 02:45:59 dacap Exp $
** Lua - An Extensible Extension Language
** Tecgraf: Computer Graphics Technology Group, PUC-Rio, Brazil
** http://www.lua.org	mailto:info@lua.org
** See Copyright Notice at the end of this file
*/


#ifndef lua_h
#define lua_h

#include <stdarg.h>
#include <stddef.h>


#define LUA_VERSION	"Lua 5.0 (beta)"
#define LUA_COPYRIGHT	"Copyright (C) 1994-2002 Tecgraf, PUC-Rio"
#define LUA_AUTHORS 	"R. Ierusalimschy, L. H. de Figueiredo & W. Celes"



/* option for multiple returns in `lua_pcall' and `lua_call' */
#define LUA_MULTRET	(-1)


/*
** pseudo-indices
*/
#define LUA_REGISTRYINDEX	(-10000)
#define LUA_GLOBALSINDEX	(-10001)
#define lua_upvalueindex(i)	(LUA_GLOBALSINDEX-(i))


/* error codes for `lua_load' and `lua_pcall' */
#define LUA_ERRRUN	1
#define LUA_ERRFILE	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5


typedef struct lua_State lua_State;

typedef int (*lua_CFunction) (lua_State *L);


/*
** functions that read/write blocks when loading/dumping Lua chunks
*/
typedef const char * (*lua_Chunkreader) (lua_State *L, void *ud, size_t *sz);

typedef int (*lua_Chunkwriter) (lua_State *L, const void* p,
                                size_t sz, void* ud);


/*
** basic types
*/
#define LUA_TNONE	(-1)

#define LUA_TNIL	0
#define LUA_TBOOLEAN	1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER	3
#define LUA_TSTRING	4
#define LUA_TTABLE	5
#define LUA_TFUNCTION	6
#define LUA_TUSERDATA	7
#define LUA_TTHREAD	8


/* minimum Lua stack available to a C function */
#define LUA_MINSTACK	20


/*
** generic extra include file
*/
#ifdef LUA_USER_H
#include LUA_USER_H
#endif


/* type of numbers in Lua */
#ifndef LUA_NUMBER
typedef double lua_Number;
#else
typedef LUA_NUMBER lua_Number;
#endif


/* mark for all API functions */
#ifndef LUA_API
#define LUA_API		extern
#endif


/*
** state manipulation
*/
LUA_API lua_State *lua_open (void);
LUA_API void       lua_close (lua_State *L);
LUA_API lua_State *lua_newthread (lua_State *L);

LUA_API lua_CFunction lua_atpanic (lua_State *L, lua_CFunction panicf);


/*
** basic stack manipulation
*/
LUA_API int   lua_gettop (lua_State *L);
LUA_API void  lua_settop (lua_State *L, int idx);
LUA_API void  lua_pushvalue (lua_State *L, int idx);
LUA_API void  lua_remove (lua_State *L, int idx);
LUA_API void  lua_insert (lua_State *L, int idx);
LUA_API void  lua_replace (lua_State *L, int idx);
LUA_API int   lua_checkstack (lua_State *L, int sz);

LUA_API void  lua_xmove (lua_State *from, lua_State *to, int n);


/*
** access functions (stack -> C)
*/

LUA_API int             lua_isnumber (lua_State *L, int idx);
LUA_API int             lua_isstring (lua_State *L, int idx);
LUA_API int             lua_iscfunction (lua_State *L, int idx);
LUA_API int             lua_isuserdata (lua_State *L, int idx);
LUA_API int             lua_type (lua_State *L, int idx);
LUA_API const char     *lua_typename (lua_State *L, int tp);

LUA_API int            lua_equal (lua_State *L, int idx1, int idx2);
LUA_API int            lua_rawequal (lua_State *L, int idx1, int idx2);
LUA_API int            lua_lessthan (lua_State *L, int idx1, int idx2);

LUA_API lua_Number      lua_tonumber (lua_State *L, int idx);
LUA_API int             lua_toboolean (lua_State *L, int idx);
LUA_API const char     *lua_tostring (lua_State *L, int idx);
LUA_API size_t          lua_strlen (lua_State *L, int idx);
LUA_API lua_CFunction   lua_tocfunction (lua_State *L, int idx);
LUA_API void	       *lua_touserdata (lua_State *L, int idx);
LUA_API lua_State      *lua_tothread (lua_State *L, int idx);
LUA_API const void     *lua_topointer (lua_State *L, int idx);


/*
** push functions (C -> stack)
*/
LUA_API void  lua_pushnil (lua_State *L);
LUA_API void  lua_pushnumber (lua_State *L, lua_Number n);
LUA_API void  lua_pushlstring (lua_State *L, const char *s, size_t l);
LUA_API void  lua_pushstring (lua_State *L, const char *s);
LUA_API const char *lua_pushvfstring (lua_State *L, const char *fmt,
                                                    va_list argp);
LUA_API const char *lua_pushfstring (lua_State *L, const char *fmt, ...);
LUA_API void  lua_pushcclosure (lua_State *L, lua_CFunction fn, int n);
LUA_API void  lua_pushboolean (lua_State *L, int b);
LUA_API void  lua_pushlightuserdata (lua_State *L, void *p);


/*
** get functions (Lua -> stack)
*/
LUA_API void  lua_gettable (lua_State *L, int idx);
LUA_API void  lua_rawget (lua_State *L, int idx);
LUA_API void  lua_rawgeti (lua_State *L, int idx, int n);
LUA_API void  lua_newtable (lua_State *L);
LUA_API int   lua_getmetatable (lua_State *L, int objindex);
LUA_API void  lua_getglobals (lua_State *L, int idx);


/*
** set functions (stack -> Lua)
*/
LUA_API void  lua_settable (lua_State *L, int idx);
LUA_API void  lua_rawset (lua_State *L, int idx);
LUA_API void  lua_rawseti (lua_State *L, int idx, int n);
LUA_API int   lua_setmetatable (lua_State *L, int objindex);
LUA_API int   lua_setglobals (lua_State *L, int idx);


/*
** `load' and `call' functions (load and run Lua code)
*/
LUA_API void  lua_call (lua_State *L, int nargs, int nresults);
LUA_API int   lua_pcall (lua_State *L, int nargs, int nresults, int errfunc);
LUA_API int lua_cpcall (lua_State *L, lua_CFunction func, void *ud);
LUA_API int   lua_load (lua_State *L, lua_Chunkreader reader, void *dt,
                        const char *chunkname);

LUA_API int lua_dump (lua_State *L, lua_Chunkwriter writer, void *data);


/*
** coroutine functions
*/
LUA_API int  lua_yield (lua_State *L, int nresults);
LUA_API int  lua_resume (lua_State *L, int narg);

/*
** garbage-collection functions
*/
LUA_API int   lua_getgcthreshold (lua_State *L);
LUA_API int   lua_getgccount (lua_State *L);
LUA_API void  lua_setgcthreshold (lua_State *L, int newthreshold);

/*
** miscellaneous functions
*/

LUA_API const char *lua_version (void);

LUA_API int   lua_error (lua_State *L);

LUA_API int   lua_next (lua_State *L, int idx);

LUA_API void  lua_concat (lua_State *L, int n);

LUA_API void *lua_newuserdata (lua_State *L, size_t sz);



/* 
** ===============================================================
** some useful macros
** ===============================================================
*/

#define lua_boxpointer(L,u) \
	(*(void **)(lua_newuserdata(L, sizeof(void *))) = (u))

#define lua_unboxpointer(L,i)	(*(void **)(lua_touserdata(L, i)))

#define lua_pop(L,n)		lua_settop(L, -(n)-1)

#define lua_register(L,n,f) \
	(lua_pushstring(L, n), \
	 lua_pushcfunction(L, f), \
	 lua_settable(L, LUA_GLOBALSINDEX))

#define lua_pushcfunction(L,f)	lua_pushcclosure(L, f, 0)

#define lua_isfunction(L,n)	(lua_type(L,n) == LUA_TFUNCTION)
#define lua_istable(L,n)	(lua_type(L,n) == LUA_TTABLE)
#define lua_islightuserdata(L,n)	(lua_type(L,n) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)		(lua_type(L,n) == LUA_TNIL)
#define lua_isboolean(L,n)	(lua_type(L,n) == LUA_TBOOLEAN)
#define lua_isnone(L,n)		(lua_type(L,n) == LUA_TNONE)
#define lua_isnoneornil(L, n)	(lua_type(L,n) <= 0)

#define lua_pushliteral(L, s)	\
	lua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)



/*
** compatibility macros and functions
*/


LUA_API int lua_pushupvalues (lua_State *L);

#define lua_getregistry(L)	lua_pushvalue(L, LUA_REGISTRYINDEX)
#define lua_setglobal(L,s)	\
   (lua_pushstring(L, s), lua_insert(L, -2), lua_settable(L, LUA_GLOBALSINDEX))

#define lua_getglobal(L,s)	\
		(lua_pushstring(L, s), lua_gettable(L, LUA_GLOBALSINDEX))


/* compatibility with ref system */

/* pre-defined references */
#define LUA_NOREF	(-2)
#define LUA_REFNIL	(-1)

#define lua_ref(L,lock)	((lock) ? luaL_ref(L, LUA_REGISTRYINDEX) : \
      (lua_pushstring(L, "unlocked references are obsolete"), lua_error(L), 0))

#define lua_unref(L,ref)	luaL_unref(L, LUA_REGISTRYINDEX, (ref))

#define lua_getref(L,ref)	lua_rawgeti(L, LUA_REGISTRYINDEX, ref)



/*
** {======================================================================
** useful definitions for Lua kernel and libraries
** =======================================================================
*/

/* formats for Lua numbers */
#ifndef LUA_NUMBER_SCAN
#define LUA_NUMBER_SCAN		"%lf"
#endif

#ifndef LUA_NUMBER_FMT
#define LUA_NUMBER_FMT		"%.14g"
#endif

/* }====================================================================== */


/*
** {======================================================================
** Debug API
** =======================================================================
*/


/*
** Event codes
*/
#define LUA_HOOKCALL	0
#define LUA_HOOKRET	1
#define LUA_HOOKLINE	2
#define LUA_HOOKCOUNT	3


/*
** Event masks
*/
#define LUA_MASKCALL	(1 << LUA_HOOKCALL)
#define LUA_MASKRET	(1 << LUA_HOOKRET)
#define LUA_MASKLINE	(1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT	(1 << LUA_HOOKCOUNT)

typedef struct lua_Debug lua_Debug;  /* activation record */

typedef void (*lua_Hook) (lua_State *L, lua_Debug *ar);


LUA_API int lua_getstack (lua_State *L, int level, lua_Debug *ar);
LUA_API int lua_getinfo (lua_State *L, const char *what, lua_Debug *ar);
LUA_API const char *lua_getlocal (lua_State *L, const lua_Debug *ar, int n);
LUA_API const char *lua_setlocal (lua_State *L, const lua_Debug *ar, int n);

LUA_API int lua_sethook (lua_State *L, lua_Hook func, int mask, int count);
LUA_API lua_Hook lua_gethook (lua_State *L);
LUA_API int lua_gethookmask (lua_State *L);
LUA_API int lua_gethookcount (lua_State *L);


#define LUA_IDSIZE	60

struct lua_Debug {
  int event;
  const char *name;	/* (n) */
  const char *namewhat;	/* (n) `global', `local', `field', `method' */
  const char *what;	/* (S) `Lua' function, `C' function, Lua `main' */
  const char *source;	/* (S) */
  int currentline;	/* (l) */
  int nups;		/* (u) number of upvalues */
  int linedefined;	/* (S) */
  char short_src[LUA_IDSIZE]; /* (S) */
  /* private part */
  int i_ci;  /* active function */
};

/* }====================================================================== */


/******************************************************************************
* Copyright (C) 2002 Tecgraf, PUC-Rio.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/


#endif
