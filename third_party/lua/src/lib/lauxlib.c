/*
** $Id: lauxlib.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/* This file uses only the official API of Lua.
** Any function declared here could be written as an application function.
*/

#define lauxlib_c

#include "lua.h"

#include "lauxlib.h"


/*
** {======================================================
** Error-report functions
** =======================================================
*/


LUALIB_API int luaL_argerror (lua_State *L, int narg, const char *extramsg) {
  lua_Debug ar;
  lua_getstack(L, 0, &ar);
  lua_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    narg--;  /* do not count `self' */
    if (narg == 0)  /* error is in the self argument itself? */
      return luaL_error(L,
        "calling %s on bad self (perhaps using `:' instead of `.')",
        ar.name);
  }
  if (ar.name == NULL)
    ar.name = "?";
  return luaL_error(L, "bad argument #%d to `%s' (%s)",
                        narg, ar.name, extramsg);
}


LUALIB_API int luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, lua_typename(L, lua_type(L,narg)));
  return luaL_argerror(L, narg, msg);
}


static void tag_error (lua_State *L, int narg, int tag) {
  luaL_typerror(L, narg, lua_typename(L, tag)); 
}


LUALIB_API void luaL_where (lua_State *L, int level) {
  lua_Debug ar;
  if (lua_getstack(L, level, &ar)) {  /* check function at level */
    lua_getinfo(L, "Snl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  lua_pushliteral(L, "");  /* else, no information available... */
}


LUALIB_API int luaL_error (lua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  luaL_where(L, 1);
  lua_pushvfstring(L, fmt, argp);
  va_end(argp);
  lua_concat(L, 2);
  return lua_error(L);
}

/* }====================================================== */


LUALIB_API int luaL_findstring (const char *name, const char *const list[]) {
  int i;
  for (i=0; list[i]; i++)
    if (strcmp(list[i], name) == 0)
      return i;
  return -1;  /* name not found */
}


LUALIB_API void luaL_checkstack (lua_State *L, int space, const char *mes) {
  if (!lua_checkstack(L, space))
    luaL_error(L, "stack overflow (%s)", mes);
}


LUALIB_API void luaL_checktype (lua_State *L, int narg, int t) {
  if (lua_type(L, narg) != t)
    tag_error(L, narg, t);
}


LUALIB_API void luaL_checkany (lua_State *L, int narg) {
  if (lua_type(L, narg) == LUA_TNONE)
    luaL_argerror(L, narg, "value expected");
}


LUALIB_API const char *luaL_checklstring (lua_State *L, int narg, size_t *len) {
  const char *s = lua_tostring(L, narg);
  if (!s) tag_error(L, narg, LUA_TSTRING);
  if (len) *len = lua_strlen(L, narg);
  return s;
}


LUALIB_API const char *luaL_optlstring (lua_State *L, int narg,
                                        const char *def, size_t *len) {
  if (lua_isnoneornil(L, narg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return luaL_checklstring(L, narg, len);
}


LUALIB_API lua_Number luaL_checknumber (lua_State *L, int narg) {
  lua_Number d = lua_tonumber(L, narg);
  if (d == 0 && !lua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, LUA_TNUMBER);
  return d;
}


LUALIB_API lua_Number luaL_optnumber (lua_State *L, int narg, lua_Number def) {
  if (lua_isnoneornil(L, narg)) return def;
  else return luaL_checknumber(L, narg);
}


LUALIB_API int luaL_getmetafield (lua_State *L, int obj, const char *event) {
  if (!lua_getmetatable(L, obj))  /* no metatable? */
    return 0;
  lua_pushstring(L, event);
  lua_rawget(L, -2);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2);  /* remove metatable and metafield */
    return 0;
  }
  return 1;
}


LUALIB_API int luaL_callmeta (lua_State *L, int obj, const char *event) {
  if (!luaL_getmetafield(L, obj, event))  /* no metafield? */
    return 0;
  lua_pushvalue(L, obj);
  lua_call(L, 1, 1);
  return 1;
}


LUALIB_API void luaL_openlib (lua_State *L, const char *libname,
                              const luaL_reg *l, int nup) {
  if (libname) {
    lua_pushstring(L, libname);
    lua_gettable(L, LUA_GLOBALSINDEX);  /* check whether lib already exists */
    if (lua_isnil(L, -1)) {  /* no? */
      lua_pop(L, 1);
      lua_newtable(L);  /* create it */
    }
    lua_insert(L, -(nup+1));  /* move library table to below upvalues */
  }
  for (; l->name; l++) {
    int i;
    lua_pushstring(L, l->name);
    for (i=0; i<nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -(nup+1));
    lua_pushcclosure(L, l->func, nup);
    lua_settable(L, -(nup+3));
  }
  lua_pop(L, nup);  /* remove upvalues */
  if (libname) {
    lua_pushstring(L, libname);
    lua_pushvalue(L, -2);
    lua_settable(L, LUA_GLOBALSINDEX);
  }
}



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#define buffempty(B)	((B)->p == (B)->buffer)
#define bufflen(B)	((B)->p - (B)->buffer)
#define bufffree(B)	((size_t)(LUAL_BUFFERSIZE - bufflen(B)))

#define LIMIT	(LUA_MINSTACK/2)


static int emptybuffer (luaL_Buffer *B) {
  size_t l = bufflen(B);
  if (l == 0) return 0;  /* put nothing on stack */
  else {
    lua_pushlstring(B->L, B->buffer, l);
    B->p = B->buffer;
    B->lvl++;
    return 1;
  }
}


static void adjuststack (luaL_Buffer *B) {
  if (B->lvl > 1) {
    lua_State *L = B->L;
    int toget = 1;  /* number of levels to concat */
    size_t toplen = lua_strlen(L, -1);
    do {
      size_t l = lua_strlen(L, -(toget+1));
      if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
        toplen += l;
        toget++;
      }
      else break;
    } while (toget < B->lvl);
    lua_concat(L, toget);
    B->lvl = B->lvl - toget + 1;
  }
}


LUALIB_API char *luaL_prepbuffer (luaL_Buffer *B) {
  if (emptybuffer(B))
    adjuststack(B);
  return B->buffer;
}


LUALIB_API void luaL_addlstring (luaL_Buffer *B, const char *s, size_t l) {
  while (l--)
    luaL_putchar(B, *s++);
}


LUALIB_API void luaL_addstring (luaL_Buffer *B, const char *s) {
  luaL_addlstring(B, s, strlen(s));
}


LUALIB_API void luaL_pushresult (luaL_Buffer *B) {
  emptybuffer(B);
  lua_concat(B->L, B->lvl);
  B->lvl = 1;
}


LUALIB_API void luaL_addvalue (luaL_Buffer *B) {
  lua_State *L = B->L;
  size_t vl = lua_strlen(L, -1);
  if (vl <= bufffree(B)) {  /* fit into buffer? */
    memcpy(B->p, lua_tostring(L, -1), vl);  /* put it there */
    B->p += vl;
    lua_pop(L, 1);  /* remove from stack */
  }
  else {
    if (emptybuffer(B))
      lua_insert(L, -2);  /* put buffer before new value */
    B->lvl++;  /* add new value into B stack */
    adjuststack(B);
  }
}


LUALIB_API void luaL_buffinit (lua_State *L, luaL_Buffer *B) {
  B->L = L;
  B->p = B->buffer;
  B->lvl = 0;
}

/* }====================================================== */


LUALIB_API int luaL_ref (lua_State *L, int t) {
  int ref;
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);  /* remove from stack */
    return LUA_REFNIL;  /* `nil' has a unique fixed reference */
  }
  lua_rawgeti(L, t, 0);  /* get first free element */
  ref = (int)lua_tonumber(L, -1);  /* ref = t[0] */
  lua_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    lua_rawgeti(L, t, ref);  /* remove it from list */
    lua_rawseti(L, t, 0);  /* (that is, t[0] = t[ref]) */
  }
  else {  /* no free elements */
    lua_pushliteral(L, "n");
    lua_pushvalue(L, -1);
    lua_rawget(L, t);  /* get t.n */
    ref = (int)lua_tonumber(L, -1) + 1;  /* ref = t.n + 1 */
    lua_pop(L, 1);  /* pop t.n */
    lua_pushnumber(L, ref);
    lua_rawset(L, t);  /* t.n = t.n + 1 */
  }
  lua_rawseti(L, t, ref);
  return ref;
}


LUALIB_API void luaL_unref (lua_State *L, int t, int ref) {
  if (ref >= 0) {
    lua_rawgeti(L, t, 0);
    lua_rawseti(L, t, ref);  /* t[ref] = t[0] */
    lua_pushnumber(L, ref);
    lua_rawseti(L, t, 0);  /* t[0] = ref */
  }
}



/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  FILE *f;
  char buff[LUAL_BUFFERSIZE];
} LoadF;


static const char *getF (lua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;
  if (feof(lf->f)) return NULL;
  *size = fread(lf->buff, 1, LUAL_BUFFERSIZE, lf->f);
  return (*size > 0) ? lf->buff : NULL;
}


static int errfile (lua_State *L, int fnameindex) {
  const char *filename = lua_tostring(L, fnameindex) + 1;
  lua_pushfstring(L, "cannot read %s: %s", filename, strerror(errno));
  lua_remove(L, fnameindex);
  return LUA_ERRFILE;
}


LUALIB_API int luaL_loadfile (lua_State *L, const char *filename) {
  LoadF lf;
  int status, readstatus;
  int c;
  int fnameindex = lua_gettop(L) + 1;  /* index of filename on the stack */
  if (filename == NULL) {
    lua_pushliteral(L, "=stdin");
    lf.f = stdin;
  }
  else {
    lua_pushfstring(L, "@%s", filename);
    lf.f = fopen(filename, "r");
  }
  if (lf.f == NULL) return errfile(L, fnameindex);  /* unable to open file */
  c = ungetc(getc(lf.f), lf.f);
  if (!(isspace(c) || isprint(c)) && lf.f != stdin) {  /* binary file? */
    fclose(lf.f);
    lf.f = fopen(filename, "rb");  /* reopen in binary mode */
    if (lf.f == NULL) return errfile(L, fnameindex); /* unable to reopen file */
  }
  status = lua_load(L, getF, &lf, lua_tostring(L, -1));
  readstatus = ferror(lf.f);
  if (lf.f != stdin) fclose(lf.f);  /* close file (even in case of errors) */
  if (readstatus) {
    lua_settop(L, fnameindex);  /* ignore results from `lua_load' */
    return errfile(L, fnameindex);
  }
  lua_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (lua_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


LUALIB_API int luaL_loadbuffer (lua_State *L, const char *buff, size_t size,
                                const char *name) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return lua_load(L, getS, &ls, name);
}

/* }====================================================== */


/*
** {======================================================
** compatibility code
** =======================================================
*/


static void callalert (lua_State *L, int status) {
  if (status != 0) {
    lua_getglobal(L, "_ALERT");
    if (lua_isfunction(L, -1)) {
      lua_insert(L, -2);
      lua_call(L, 1, 0);
    }
    else {  /* no _ALERT function; print it on stderr */
      fprintf(stderr, "%s\n", lua_tostring(L, -2));
      lua_pop(L, 2);  /* remove error message and _ALERT */
    }
  }
}


static int aux_do (lua_State *L, int status) {
  if (status == 0) {  /* parse OK? */
    status = lua_pcall(L, 0, LUA_MULTRET, 0);  /* call main */
  }
  callalert(L, status);
  return status;
}


LUALIB_API int lua_dofile (lua_State *L, const char *filename) {
  return aux_do(L, luaL_loadfile(L, filename));
}


LUALIB_API int lua_dobuffer (lua_State *L, const char *buff, size_t size,
                          const char *name) {
  return aux_do(L, luaL_loadbuffer(L, buff, size, name));
}


LUALIB_API int lua_dostring (lua_State *L, const char *str) {
  return lua_dobuffer(L, str, strlen(str), str);
}

/* }====================================================== */
