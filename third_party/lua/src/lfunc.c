/*
** $Id: lfunc.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Auxiliary functions to manipulate prototypes and closures
** See Copyright Notice in lua.h
*/


#include <stdlib.h>

#define lfunc_c

#include "lua.h"

#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"


#define sizeCclosure(n)	(cast(int, sizeof(CClosure)) + \
                         cast(int, sizeof(TObject)*((n)-1)))

#define sizeLclosure(n)	(cast(int, sizeof(LClosure)) + \
                         cast(int, sizeof(TObject *)*((n)-1)))



Closure *luaF_newCclosure (lua_State *L, int nelems) {
  Closure *c = cast(Closure *, luaM_malloc(L, sizeCclosure(nelems)));
  luaC_link(L, valtogco(c), LUA_TFUNCTION);
  c->c.isC = 1;
  c->c.nupvalues = cast(lu_byte, nelems);
  return c;
}


Closure *luaF_newLclosure (lua_State *L, int nelems, TObject *gt) {
  Closure *c = cast(Closure *, luaM_malloc(L, sizeLclosure(nelems)));
  luaC_link(L, valtogco(c), LUA_TFUNCTION);
  c->l.isC = 0;
  c->l.g = *gt;
  c->l.nupvalues = cast(lu_byte, nelems);
  return c;
}


UpVal *luaF_findupval (lua_State *L, StkId level) {
  GCObject **pp = &L->openupval;
  UpVal *p;
  UpVal *v;
  while ((p = ngcotouv(*pp)) != NULL && p->v >= level) {
    if (p->v == level) return p;
    pp = &p->next;
  }
  v = luaM_new(L, UpVal);  /* not found: create a new one */
  v->tt = LUA_TUPVAL;
  v->marked = 1;  /* open upvalues should not be collected */
  v->v = level;  /* current value lives in the stack */
  v->next = *pp;  /* chain it in the proper position */
  *pp = valtogco(v);
  return v;
}


void luaF_close (lua_State *L, StkId level) {
  UpVal *p;
  while ((p = ngcotouv(L->openupval)) != NULL && p->v >= level) {
    setobj(&p->value, p->v);  /* save current value (write barrier) */
    p->v = &p->value;  /* now current value lives here */
    L->openupval = p->next;  /* remove from `open' list */
    luaC_link(L, valtogco(p), LUA_TUPVAL);
  }
}


Proto *luaF_newproto (lua_State *L) {
  Proto *f = luaM_new(L, Proto);
  luaC_link(L, valtogco(f), LUA_TPROTO);
  f->k = NULL;
  f->sizek = 0;
  f->p = NULL;
  f->sizep = 0;
  f->code = NULL;
  f->sizecode = 0;
  f->sizelineinfo = 0;
  f->nupvalues = 0;
  f->numparams = 0;
  f->is_vararg = 0;
  f->maxstacksize = 0;
  f->lineinfo = NULL;
  f->sizelocvars = 0;
  f->locvars = NULL;
  f->lineDefined = 0;
  f->source = NULL;
  return f;
}


void luaF_freeproto (lua_State *L, Proto *f) {
  luaM_freearray(L, f->code, f->sizecode, Instruction);
  luaM_freearray(L, f->p, f->sizep, Proto *);
  luaM_freearray(L, f->k, f->sizek, TObject);
  luaM_freearray(L, f->lineinfo, f->sizelineinfo, int);
  luaM_freearray(L, f->locvars, f->sizelocvars, struct LocVar);
  luaM_freelem(L, f);
}


void luaF_freeclosure (lua_State *L, Closure *c) {
  int size = (c->c.isC) ? sizeCclosure(c->c.nupvalues) :
                          sizeLclosure(c->l.nupvalues);
  luaM_free(L, c, size);
}


/*
** Look for n-th local variable at line `line' in function `func'.
** Returns NULL if not found.
*/
const char *luaF_getlocalname (const Proto *f, int local_number, int pc) {
  int i;
  for (i = 0; i<f->sizelocvars && f->locvars[i].startpc <= pc; i++) {
    if (pc < f->locvars[i].endpc) {  /* is variable active? */
      local_number--;
      if (local_number == 0)
        return getstr(f->locvars[i].varname);
    }
  }
  return NULL;  /* not found */
}

