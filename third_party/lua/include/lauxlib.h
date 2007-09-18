/*
** $Id: lauxlib.h,v 1.1 2004/03/09 02:45:59 dacap Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/


#ifndef lauxlib_h
#define lauxlib_h


#include <stddef.h>
#include <stdio.h>

#include "lua.h"


#ifndef LUALIB_API
#define LUALIB_API	extern
#endif



typedef struct luaL_reg {
  const char *name;
  lua_CFunction func;
} luaL_reg;


LUALIB_API void luaL_openlib (lua_State *L, const char *libname,
                               const luaL_reg *l, int nup);
LUALIB_API int luaL_getmetafield (lua_State *L, int obj, const char *e);
LUALIB_API int luaL_callmeta (lua_State *L, int obj, const char *e);
LUALIB_API int luaL_typerror (lua_State *L, int narg, const char *tname);
LUALIB_API int luaL_argerror (lua_State *L, int numarg, const char *extramsg);
LUALIB_API const char *luaL_checklstring (lua_State *L, int numArg, size_t *l);
LUALIB_API const char *luaL_optlstring (lua_State *L, int numArg,
                                           const char *def, size_t *l);
LUALIB_API lua_Number luaL_checknumber (lua_State *L, int numArg);
LUALIB_API lua_Number luaL_optnumber (lua_State *L, int nArg, lua_Number def);

LUALIB_API void luaL_checkstack (lua_State *L, int sz, const char *msg);
LUALIB_API void luaL_checktype (lua_State *L, int narg, int t);
LUALIB_API void luaL_checkany (lua_State *L, int narg);

LUALIB_API void luaL_where (lua_State *L, int lvl);
LUALIB_API int luaL_error (lua_State *L, const char *fmt, ...);

LUALIB_API int luaL_findstring (const char *st, const char *const lst[]);

LUALIB_API int luaL_ref (lua_State *L, int t);
LUALIB_API void luaL_unref (lua_State *L, int t, int ref);

LUALIB_API int luaL_loadfile (lua_State *L, const char *filename);
LUALIB_API int luaL_loadbuffer (lua_State *L, const char *buff, size_t sz,
                                const char *name);



/*
** ===============================================================
** some useful macros
** ===============================================================
*/

#define luaL_argcheck(L, cond,numarg,extramsg) if (!(cond)) \
                                               luaL_argerror(L, numarg,extramsg)
#define luaL_checkstring(L,n)	(luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L,n,d)	(luaL_optlstring(L, (n), (d), NULL))
#define luaL_checkint(L,n)	((int)luaL_checknumber(L, n))
#define luaL_checklong(L,n)	((long)luaL_checknumber(L, n))
#define luaL_optint(L,n,d)	((int)luaL_optnumber(L, n,d))
#define luaL_optlong(L,n,d)	((long)luaL_optnumber(L, n,d))


/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#ifndef LUAL_BUFFERSIZE
#define LUAL_BUFFERSIZE	  BUFSIZ
#endif


typedef struct luaL_Buffer {
  char *p;			/* current position in buffer */
  int lvl;  /* number of strings in the stack (level) */
  lua_State *L;
  char buffer[LUAL_BUFFERSIZE];
} luaL_Buffer;

#define luaL_putchar(B,c) \
  ((void)((B)->p < ((B)->buffer+LUAL_BUFFERSIZE) || luaL_prepbuffer(B)), \
   (*(B)->p++ = (char)(c)))

#define luaL_addsize(B,n)	((B)->p += (n))

LUALIB_API void luaL_buffinit (lua_State *L, luaL_Buffer *B);
LUALIB_API char *luaL_prepbuffer (luaL_Buffer *B);
LUALIB_API void luaL_addlstring (luaL_Buffer *B, const char *s, size_t l);
LUALIB_API void luaL_addstring (luaL_Buffer *B, const char *s);
LUALIB_API void luaL_addvalue (luaL_Buffer *B);
LUALIB_API void luaL_pushresult (luaL_Buffer *B);


/* }====================================================== */



/*
** Compatibility macros
*/

LUALIB_API int   lua_dofile (lua_State *L, const char *filename);
LUALIB_API int   lua_dostring (lua_State *L, const char *str);
LUALIB_API int   lua_dobuffer (lua_State *L, const char *buff, size_t sz,
                               const char *n);

/*
#define luaL_check_lstr 	luaL_checklstring
#define luaL_opt_lstr 	luaL_optlstring 
#define luaL_check_number 	luaL_checknumber 
#define luaL_opt_number	luaL_optnumber
#define luaL_arg_check	luaL_argcheck
#define luaL_check_string	luaL_checkstring
#define luaL_opt_string	luaL_optstring
#define luaL_check_int	luaL_checkint
#define luaL_check_long	luaL_checklong
#define luaL_opt_int	luaL_optint
#define luaL_opt_long	luaL_optlong
*/

#endif


