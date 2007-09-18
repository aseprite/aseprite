/*
** $Id: lmem.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/


#include <stdlib.h>

#define lmem_c

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"



/*
** definition for realloc function. It must assure that l_realloc(NULL,
** 0, x) allocates a new block (ANSI C assures that). (`os' is the old
** block size; some allocators may use that.)
*/
#ifndef l_realloc
#define l_realloc(b,os,s)	realloc(b,s)
#endif

/*
** definition for free function. (`os' is the old block size; some
** allocators may use that.)
*/
#ifndef l_free
#define l_free(b,os)	free(b)
#endif


#define MINSIZEARRAY	4


void *luaM_growaux (lua_State *L, void *block, int *size, int size_elems,
                    int limit, const char *errormsg) {
  void *newblock;
  int newsize = (*size)*2;
  if (newsize < MINSIZEARRAY)
    newsize = MINSIZEARRAY;  /* minimum size */
  else if (*size >= limit/2) {  /* cannot double it? */
    if (*size < limit - MINSIZEARRAY)  /* try something smaller... */
      newsize = limit;  /* still have at least MINSIZEARRAY free places */
    else luaG_runerror(L, errormsg);
  }
  newblock = luaM_realloc(L, block,
                          cast(lu_mem, *size)*cast(lu_mem, size_elems),
                          cast(lu_mem, newsize)*cast(lu_mem, size_elems));
  *size = newsize;  /* update only when everything else is OK */
  return newblock;
}


/*
** generic allocation routine.
*/
void *luaM_realloc (lua_State *L, void *block, lu_mem oldsize, lu_mem size) {
  lua_assert((oldsize == 0) == (block == NULL));
  if (size == 0) {
    if (block != NULL) {
      l_free(block, oldsize);
      block = NULL;
    }
    else return NULL;  /* avoid `nblocks' computations when oldsize==size==0 */
  }
  else if (size >= MAX_SIZET)
    luaG_runerror(L, "memory allocation error: block too big");
  else {
    block = l_realloc(block, oldsize, size);
    if (block == NULL) {
      if (L)
        luaD_throw(L, LUA_ERRMEM);
      else return NULL;  /* error before creating state! */
    }
  }
  if (L) {
    lua_assert(G(L) != NULL && G(L)->nblocks > 0);
    G(L)->nblocks -= oldsize;
    G(L)->nblocks += size;
  }
  return block;
}

