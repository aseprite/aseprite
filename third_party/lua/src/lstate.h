/*
** $Id: lstate.h,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Global State
** See Copyright Notice in lua.h
*/

#ifndef lstate_h
#define lstate_h

#include "lua.h"

#include "lobject.h"
#include "ltm.h"
#include "lzio.h"


/*
** macros for thread synchronization inside Lua core machine:
** all accesses to the global state and to global objects are synchronized.
** Because threads can read the stack of other threads
** (when running garbage collection),
** a thread must also synchronize any write-access to its own stack.
** Unsynchronized accesses are allowed only when reading its own stack,
** or when reading immutable fields from global objects
** (such as string values and udata values). 
*/
#ifndef lua_lock
#define lua_lock(L)	((void) 0)
#endif

#ifndef lua_unlock
#define lua_unlock(L)	((void) 0)
#endif


#ifndef lua_userstateopen
#define lua_userstateopen(l)
#endif



struct lua_longjmp;  /* defined in ldo.c */


/* default meta table (both for tables and udata) */
#define defaultmeta(L)	(&G(L)->_defaultmeta)

/* table of globals */
#define gt(L)	(&L->_gt)

/* registry */
#define registry(L)	(&G(L)->_registry)


/* extra stack space to handle TM calls and some other extras */
#define EXTRA_STACK   5


#define BASIC_CI_SIZE           8

#define BASIC_STACK_SIZE        (2*LUA_MINSTACK)



typedef struct stringtable {
  GCObject **hash;
  ls_nstr nuse;  /* number of elements */
  int size;
} stringtable;


/*
** informations about a call
*/
typedef struct CallInfo {
  StkId base;  /* base for called function */
  StkId	top;  /* top for this function */
  int state;  /* bit fields; see below */
  union {
    struct {  /* for Lua functions */
      const Instruction *savedpc;
      const Instruction **pc;  /* points to `pc' variable in `luaV_execute' */
    } l;
    struct {  /* for C functions */
      int dummy;  /* just to avoid an empty struct */
    } c;
  } u;
} CallInfo;


/*
** bit fields for `CallInfo.state'
*/
#define CI_C		(1<<0)  /* 1 if function is a C function */
/* 1 if (Lua) function has an active `luaV_execute' running it */
#define CI_HASFRAME	(1<<1)
/* 1 if Lua function is calling another Lua function (and therefore its
   `pc' is being used by the other, and therefore CI_SAVEDPC is 1 too) */
#define CI_CALLING	(1<<2)
#define CI_SAVEDPC	(1<<3)  /* 1 if `savedpc' is updated */
#define CI_YIELD	(1<<4)  /* 1 if thread is suspended */


#define ci_func(ci)	(clvalue((ci)->base - 1))


/*
** `global state', shared by all threads of this state
*/
typedef struct global_State {
  stringtable strt;  /* hash table for strings */
  GCObject *rootgc;  /* list of (almost) all collectable objects */
  GCObject *rootudata;   /* (separated) list of all userdata */
  GCObject *tmudata;  /* list of userdata to be GC */
  Mbuffer buff;  /* temporary buffer for string concatentation */
  lu_mem GCthreshold;
  lu_mem nblocks;  /* number of `bytes' currently allocated */
  lua_CFunction panic;  /* to be called in unprotected errors */
  TObject _registry;
  TObject _defaultmeta;
  struct lua_State *mainthread;
  Node dummynode[1];  /* common node array for all empty tables */
  TString *tmname[TM_N];  /* array with tag-method names */
} global_State;


/*
** `per thread' state
*/
struct lua_State {
  CommonHeader;
  StkId top;  /* first free slot in the stack */
  StkId base;  /* base of current function */
  global_State *l_G;
  CallInfo *ci;  /* call info for current function */
  StkId stack_last;  /* last free slot in the stack */
  StkId stack;  /* stack base */
  int stacksize;
  CallInfo *end_ci;  /* points after end of ci array*/
  CallInfo *base_ci;  /* array of CallInfo's */
  unsigned short size_ci;  /* size of array `base_ci' */
  unsigned short nCcalls;  /* number of nested C calls */
  lu_byte hookmask;
  lu_byte allowhook;
  lu_byte hookinit;
  int basehookcount;
  int hookcount;
  lua_Hook hook;
  TObject _gt;  /* table of globals */
  GCObject *openupval;  /* list of open upvalues in this stack */
  GCObject *gclist;
  struct lua_longjmp *errorJmp;  /* current error recover point */
  ptrdiff_t errfunc;  /* current error handling function (stack index) */
};


#define G(L)	(L->l_G)


/*
** Union of all collectable objects
*/
union GCObject {
  GCheader gch;
  union TString ts;
  union Udata u;
  union Closure cl;
  struct Table h;
  struct Proto p;
  struct UpVal uv;
  struct lua_State th;  /* thread */
};


/* macros to convert a GCObject into a specific value */
#define gcotots(o)	check_exp((o)->gch.tt == LUA_TSTRING, &((o)->ts))
#define gcotou(o)	check_exp((o)->gch.tt == LUA_TUSERDATA, &((o)->u))
#define gcotocl(o)	check_exp((o)->gch.tt == LUA_TFUNCTION, &((o)->cl))
#define gcotoh(o)	check_exp((o)->gch.tt == LUA_TTABLE, &((o)->h))
#define gcotop(o)	check_exp((o)->gch.tt == LUA_TPROTO, &((o)->p))
#define gcotouv(o)	check_exp((o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define ngcotouv(o) \
	check_exp((o) == NULL || (o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define gcototh(o)	check_exp((o)->gch.tt == LUA_TTHREAD, &((o)->th))

/* macro to convert any value into a GCObject */
#define valtogco(v)	(cast(GCObject *, (v)))


lua_State *luaE_newthread (lua_State *L);
void luaE_freethread (lua_State *L, lua_State *L1);

#endif

