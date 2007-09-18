/*
** $Id: lbaselib.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Basic library
** See Copyright Notice in lua.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lbaselib_c

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"




/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int luaB_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, "`tostring' must return a string to `print'");
    if (i>1) fputs("\t", stdout);
    fputs(s, stdout);
    lua_pop(L, 1);  /* pop result */
  }
  fputs("\n", stdout);
  return 0;
}


static int luaB_tonumber (lua_State *L) {
  int base = luaL_optint(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    luaL_checkany(L, 1);
    if (lua_isnumber(L, 1)) {
      lua_pushnumber(L, lua_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = luaL_checkstring(L, 1);
    char *s2;
    unsigned long n;
    luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)(*s2))) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        lua_pushnumber(L, n);
        return 1;
      }
    }
  }
  lua_pushnil(L);  /* else not a number */
  return 1;
}


static int luaB_error (lua_State *L) {
  int level = luaL_optint(L, 2, 1);
  luaL_checkany(L, 1);
  if (!lua_isstring(L, 1) || level == 0)
    lua_pushvalue(L, 1);  /* propagate error message without changes */
  else {  /* add extra information */
    luaL_where(L, level);
    lua_pushvalue(L, 1);
    lua_concat(L, 2);
  }
  return lua_error(L);
}


static int luaB_getmetatable (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    lua_pushnil(L);
    return 1;  /* no metatable */
  }
  luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable (lua_State *L) {
  int t = lua_type(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_argcheck(L, t == LUA_TNIL || t == LUA_TTABLE, 2,
                    "nil or table expected");
  if (luaL_getmetafield(L, 1, "__metatable"))
    luaL_error(L, "cannot change a protected metatable");
  lua_settop(L, 2);
  lua_setmetatable(L, 1);
  return 1;
}


static void getfunc (lua_State *L) {
  if (lua_isfunction(L, 1)) lua_pushvalue(L, 1);
  else {
    lua_Debug ar;
    int level = luaL_optint(L, 1, 1);
    luaL_argcheck(L, level >= 0, 1, "level must be non-negative");
    if (lua_getstack(L, level, &ar) == 0)
      luaL_argerror(L, 1, "invalid level");
    lua_getinfo(L, "f", &ar);
  }
}


static int aux_getglobals (lua_State *L) {
  lua_getglobals(L, -1);
  lua_pushliteral(L, "__globals");
  lua_rawget(L, -2);
  return !lua_isnil(L, -1);
}


static int luaB_getglobals (lua_State *L) {
  getfunc(L);
  if (!aux_getglobals(L))  /* __globals not defined? */
    lua_pop(L, 1);  /* remove it, to return real globals */
  return 1;
}


static int luaB_setglobals (lua_State *L) {
  luaL_checktype(L, 2, LUA_TTABLE);
  getfunc(L);
  if (aux_getglobals(L))  /* __globals defined? */
    luaL_error(L, "cannot change a protected global table");
  else
    lua_pop(L, 2);  /* remove __globals and real global table */
  lua_pushvalue(L, 2);
  if (lua_setglobals(L, -2) == 0)
    luaL_error(L, "cannot change global table of given function");
  return 0;
}


static int luaB_rawequal (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_checkany(L, 2);
  lua_pushboolean(L, lua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawget (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  lua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  lua_rawset(L, 1);
  return 1;
}


static int luaB_gcinfo (lua_State *L) {
  lua_pushnumber(L, lua_getgccount(L));
  lua_pushnumber(L, lua_getgcthreshold(L));
  return 2;
}


static int luaB_collectgarbage (lua_State *L) {
  lua_setgcthreshold(L, luaL_optint(L, 1, 0));
  return 0;
}


static int luaB_type (lua_State *L) {
  luaL_checkany(L, 1);
  lua_pushstring(L, lua_typename(L, lua_type(L, 1)));
  return 1;
}


static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int luaB_pairs (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushliteral(L, "next");
  lua_rawget(L, LUA_GLOBALSINDEX);  /* return generator, */
  lua_pushvalue(L, 1);  /* state, */
  lua_pushnil(L);  /* and initial value */
  return 3;
}


static int luaB_ipairs (lua_State *L) {
  lua_Number i = lua_tonumber(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  if (i == 0 && lua_isnone(L, 2)) {  /* `for' start? */
    lua_pushliteral(L, "ipairs");
    lua_rawget(L, LUA_GLOBALSINDEX);  /* return generator, */
    lua_pushvalue(L, 1);  /* state, */
    lua_pushnumber(L, 0);  /* and initial value */
    return 3;
  }
  else {  /* `for' step */
    i++;  /* next value */
    lua_pushnumber(L, i);
    lua_rawgeti(L, 1, (int)i);
    return (lua_isnil(L, -1)) ? 0 : 2;
  }
}


static int load_aux (lua_State *L, int status) {
  if (status == 0) {  /* OK? */
    lua_Debug ar;
    lua_getstack(L, 1, &ar);
    lua_getinfo(L, "f", &ar);  /* get calling function */
    lua_getglobals(L, -1);  /* get its global table */
    lua_setglobals(L, -3);  /* set it as the global table of the new chunk */
    lua_pop(L, 1);  /* remove calling function */
    return 1;
  }
  else {
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
  }
}


static int luaB_loadstring (lua_State *L) {
  size_t l;
  const char *s = luaL_checklstring(L, 1, &l);
  const char *chunkname = luaL_optstring(L, 2, s);
  return load_aux(L, luaL_loadbuffer(L, s, l, chunkname));
}


static int luaB_loadfile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  return load_aux(L, luaL_loadfile(L, fname));
}


static int luaB_dofile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  int status = luaL_loadfile(L, fname);
  if (status != 0) lua_error(L);
  lua_call(L, 0, LUA_MULTRET);
  return lua_gettop(L) - 1;
}


static int luaB_assert (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_toboolean(L, 1))
    return luaL_error(L, "%s", luaL_optstring(L, 2, "assertion failed!"));
  lua_settop(L, 1);
  return 1;
}


static int luaB_unpack (lua_State *L) {
  int n, i;
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushliteral(L, "n");
  lua_rawget(L, 1);
  n = (lua_isnumber(L, -1)) ?  (int)lua_tonumber(L, -1) : -1;
  for (i=0; i<n || n==-1; i++) {  /* push arg[1...n] */
    luaL_checkstack(L, LUA_MINSTACK, "table too big to unpack");
    lua_rawgeti(L, 1, i+1);
    if (n == -1) {  /* no explicit limit? */
      if (lua_isnil(L, -1)) {  /* stop at first `nil' element */
        lua_pop(L, 1);  /* remove `nil' */
        break;
      }
    }
  }
  return i;
}


static int luaB_pcall (lua_State *L) {
  int status;
  luaL_checkany(L, 1);
  status = lua_pcall(L, lua_gettop(L) - 1, LUA_MULTRET, 0);
  lua_pushboolean(L, (status == 0));
  lua_insert(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static int luaB_xpcall (lua_State *L) {
  int status;
  luaL_checkany(L, 2);
  lua_settop(L, 2);
  lua_insert(L, 1);  /* put error function under function to be called */
  status = lua_pcall(L, 0, LUA_MULTRET, 1);
  lua_pushboolean(L, (status == 0));
  lua_replace(L, 1);
  return lua_gettop(L);  /* return status + all results */
}


static int luaB_tostring (lua_State *L) {
  char buff[64];
  luaL_checkany(L, 1);
  if (luaL_callmeta(L, 1, "__tostring"))  /* is there a metafield? */
    return 1;  /* use its value */
  switch (lua_type(L, 1)) {
    case LUA_TNUMBER:
      lua_pushstring(L, lua_tostring(L, 1));
      return 1;
    case LUA_TSTRING:
      lua_pushvalue(L, 1);
      return 1;
    case LUA_TBOOLEAN:
      lua_pushstring(L, (lua_toboolean(L, 1) ? "true" : "false"));
      return 1;
    case LUA_TTABLE:
      sprintf(buff, "table: %p", lua_topointer(L, 1));
      break;
    case LUA_TFUNCTION:
      sprintf(buff, "function: %p", lua_topointer(L, 1));
      break;
    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
      sprintf(buff, "userdata: %p", lua_touserdata(L, 1));
      break;
    case LUA_TTHREAD:
      sprintf(buff, "thread: %p", (void *)lua_tothread(L, 1));
      break;
    case LUA_TNIL:
      lua_pushliteral(L, "nil");
      return 1;
  }
  lua_pushstring(L, buff);
  return 1;
}


static int luaB_newproxy (lua_State *L) {
  lua_settop(L, 1);
  lua_newuserdata(L, 0);  /* create proxy */
  if (lua_toboolean(L, 1) == 0)
    return 1;  /* no metatable */
  else if (lua_isboolean(L, 1)) {
    lua_newtable(L);  /* create a new metatable `m' ... */
    lua_pushvalue(L, -1);  /* ... and mark `m' as a valid metatable */
    lua_pushboolean(L, 1);
    lua_rawset(L, lua_upvalueindex(1));  /* weaktable[m] = true */
  }
  else {
    int validproxy = 0;  /* to check if weaktable[metatable(u)] == true */
    if (lua_getmetatable(L, 1)) {
      lua_rawget(L, lua_upvalueindex(1));
      validproxy = lua_toboolean(L, -1);
      lua_pop(L, 1);  /* remove value */
    }
    luaL_argcheck(L, validproxy, 1, "boolean or proxy expected");
    lua_getmetatable(L, 1);  /* metatable is valid; get it */
  }
  lua_setmetatable(L, 2);
  return 1;
}


/*
** {======================================================
** `require' function
** =======================================================
*/


/* name of global that holds table with loaded packages */
#define REQTAB		"_LOADED"

/* name of global that holds the search path for packages */
#define LUA_PATH	"LUA_PATH"

#ifndef LUA_PATH_SEP
#define LUA_PATH_SEP	';'
#endif

#ifndef LUA_PATH_DEFAULT
#define LUA_PATH_DEFAULT	"?;?.lua"
#endif


static const char *getpath (lua_State *L) {
  const char *path;
  lua_getglobal(L, LUA_PATH);  /* try global variable */
  path = lua_tostring(L, -1);
  lua_pop(L, 1);
  if (path) return path;
  path = getenv(LUA_PATH);  /* else try environment variable */
  if (path) return path;
  return LUA_PATH_DEFAULT;  /* else use default */
}


static const char *pushnextpath (lua_State *L, const char *path) {
  const char *l;
  if (*path == '\0') return NULL;  /* no more paths */
  if (*path == LUA_PATH_SEP) path++;  /* skip separator */
  l = strchr(path, LUA_PATH_SEP);  /* find next separator */
  if (l == NULL) l = path+strlen(path);
  lua_pushlstring(L, path, l - path);  /* directory name */
  return l;
}


static void pushcomposename (lua_State *L) {
  const char *path = lua_tostring(L, -1);
  const char *wild = strchr(path, '?');
  if (wild == NULL) return;  /* no wild char; path is the file name */
  lua_pushlstring(L, path, wild - path);
  lua_pushvalue(L, 1);  /* package name */
  lua_pushstring(L, wild + 1);
  lua_concat(L, 3);
}


static int luaB_require (lua_State *L) {
  const char *path;
  int status = LUA_ERRFILE;  /* not found (yet) */
  luaL_checkstring(L, 1);
  lua_settop(L, 1);
  lua_pushvalue(L, 1);
  lua_setglobal(L, "_REQUIREDNAME");
  lua_getglobal(L, REQTAB);
  if (!lua_istable(L, 2)) return luaL_error(L, "`" REQTAB "' is not a table");
  path = getpath(L);
  lua_pushvalue(L, 1);  /* check package's name in book-keeping table */
  lua_rawget(L, 2);
  if (!lua_isnil(L, -1))  /* is it there? */
    return 0;  /* package is already loaded */
  else {  /* must load it */
    while (status == LUA_ERRFILE) {
      lua_settop(L, 3);  /* reset stack position */
      if ((path = pushnextpath(L, path)) == NULL) break;
      pushcomposename(L);
      status = luaL_loadfile(L, lua_tostring(L, -1));  /* try to load it */
    }
  }
  switch (status) {
    case 0: {
      lua_call(L, 0, 0);  /* run loaded module */
      lua_pushvalue(L, 1);
      lua_pushboolean(L, 1);
      lua_rawset(L, 2);  /* mark it as loaded */
      return 0;
    }
    case LUA_ERRFILE: {  /* file not found */
      return luaL_error(L, "could not load package `%s' from path `%s'",
                            lua_tostring(L, 1), getpath(L));
    }
    default: {
      return luaL_error(L, "error loading package\n%s", lua_tostring(L, -1));
    }
  }
}

/* }====================================================== */


static const luaL_reg base_funcs[] = {
  {"error", luaB_error},
  {"getmetatable", luaB_getmetatable},
  {"setmetatable", luaB_setmetatable},
  {"getglobals", luaB_getglobals},
  {"setglobals", luaB_setglobals},
  {"next", luaB_next},
  {"ipairs", luaB_ipairs},
  {"pairs", luaB_pairs},
  {"print", luaB_print},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"assert", luaB_assert},
  {"unpack", luaB_unpack},
  {"rawequal", luaB_rawequal},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"pcall", luaB_pcall},
  {"xpcall", luaB_xpcall},
  {"collectgarbage", luaB_collectgarbage},
  {"gcinfo", luaB_gcinfo},
  {"loadfile", luaB_loadfile},
  {"dofile", luaB_dofile},
  {"loadstring", luaB_loadstring},
  {"require", luaB_require},
  {NULL, NULL}
};


/*
** {======================================================
** Coroutine library
** =======================================================
*/

static int auxresume (lua_State *L, lua_State *co, int narg) {
  int status;
  if (!lua_checkstack(co, narg))
    luaL_error(L, "too many arguments to resume");
  lua_xmove(L, co, narg);
  status = lua_resume(co, narg);
  if (status == 0) {
    int nres = lua_gettop(co);
    if (!lua_checkstack(L, narg))
      luaL_error(L, "too many results to resume");
    lua_xmove(co, L, nres);  /* move yielded values */
    return nres;
  }
  else {
    lua_xmove(co, L, 1);  /* move error message */
    return -1;  /* error flag */
  }
}


static int luaB_coresume (lua_State *L) {
  lua_State *co = lua_tothread(L, 1);
  int r;
  luaL_argcheck(L, co, 1, "coroutine expected");
  r = auxresume(L, co, lua_gettop(L) - 1);
  if (r < 0) {
    lua_pushboolean(L, 0);
    lua_insert(L, -2);
    return 2;  /* return false + error message */
  }
  else {
    lua_pushboolean(L, 1);
    lua_insert(L, -(r + 1));
    return r + 1;  /* return true + `resume' returns */
  }
}


static int luaB_auxwrap (lua_State *L) {
  lua_State *co = lua_tothread(L, lua_upvalueindex(1));
  int r = auxresume(L, co, lua_gettop(L));
  if (r < 0) lua_error(L);  /* propagate error */
  return r;
}


static int luaB_cocreate (lua_State *L) {
  lua_State *NL = lua_newthread(L);
  luaL_argcheck(L, lua_isfunction(L, 1) && !lua_iscfunction(L, 1), 1,
    "Lua function expected");
  lua_pushvalue(L, 1);  /* move function to top */
  lua_xmove(L, NL, 1);  /* move function from L to NL */
  return 1;
}


static int luaB_cowrap (lua_State *L) {
  luaB_cocreate(L);
  lua_pushcclosure(L, luaB_auxwrap, 1);
  return 1;
}


static int luaB_yield (lua_State *L) {
  return lua_yield(L, lua_gettop(L));
}


static int luaB_costatus (lua_State *L) {
  lua_State *co = lua_tothread(L, 1);
  luaL_argcheck(L, co, 1, "coroutine expected");
  if (L == co) lua_pushliteral(L, "running");
  else {
    lua_Debug ar;
    if (lua_getstack(co, 0, &ar) == 0 && lua_gettop(co) == 0)
      lua_pushliteral(L, "dead");
    else
      lua_pushliteral(L, "suspended");
  }
  return 1;
}


static const luaL_reg co_funcs[] = {
  {"create", luaB_cocreate},
  {"wrap", luaB_cowrap},
  {"resume", luaB_coresume},
  {"yield", luaB_yield},
  {"status", luaB_costatus},
  {NULL, NULL}
};

/* }====================================================== */



static void base_open (lua_State *L) {
  lua_pushliteral(L, "_G");
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  luaL_openlib(L, NULL, base_funcs, 0);  /* open lib into global table */
  lua_pushliteral(L, "_VERSION");
  lua_pushliteral(L, LUA_VERSION);
  lua_rawset(L, -3);  /* set global _VERSION */
  /* `newproxy' needs a weaktable as upvalue */
  lua_pushliteral(L, "newproxy");
  lua_newtable(L);  /* new table `w' */
  lua_pushvalue(L, -1);  /* `w' will be its own metatable */
  lua_setmetatable(L, -2);
  lua_pushliteral(L, "__mode");
  lua_pushliteral(L, "k");
  lua_rawset(L, -3);  /* metatable(w).__mode = "k" */
  lua_pushcclosure(L, luaB_newproxy, 1);
  lua_rawset(L, -3);  /* set global `newproxy' */
  lua_rawset(L, -1);  /* set global _G */
}


LUALIB_API int lua_baselibopen (lua_State *L) {
  base_open(L);
  luaL_openlib(L, LUA_COLIBNAME, co_funcs, 0);
  lua_newtable(L);
  lua_setglobal(L, REQTAB);
  return 0;
}

