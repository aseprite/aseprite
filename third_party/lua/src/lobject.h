/*
** $Id: lobject.h,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/

#ifndef lobject_h
#define lobject_h


#include "llimits.h"
#include "lua.h"


/* tags for values visible from Lua */
#define NUM_TAGS	LUA_TTHREAD


/*
** Extra tags for non-values
*/
#define LUA_TPROTO	(NUM_TAGS+1)
#define LUA_TUPVAL	(NUM_TAGS+2)


/*
** Union of all collectable objects
*/
typedef union GCObject GCObject;


/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader	GCObject *next; lu_byte tt; lu_byte marked


/*
** Common header in struct form
*/
typedef struct GCheader {
  CommonHeader;
} GCheader;




/*
** Union of all Lua values
*/
typedef union {
  GCObject *gc;
  void *p;
  lua_Number n;
  int b;
} Value;


/*
** Lua values (or `tagged objects')
*/
typedef struct lua_TObject {
  int tt;
  Value value;
} TObject;


/* Macros to test type */
#define ttisnil(o)	(ttype(o) == LUA_TNIL)
#define ttisnumber(o)	(ttype(o) == LUA_TNUMBER)
#define ttisstring(o)	(ttype(o) == LUA_TSTRING)
#define ttistable(o)	(ttype(o) == LUA_TTABLE)
#define ttisfunction(o)	(ttype(o) == LUA_TFUNCTION)
#define ttisboolean(o)	(ttype(o) == LUA_TBOOLEAN)
#define ttisuserdata(o)	(ttype(o) == LUA_TUSERDATA)
#define ttisthread(o)	(ttype(o) == LUA_TTHREAD)
#define ttislightuserdata(o)	(ttype(o) == LUA_TLIGHTUSERDATA)

/* Macros to access values */
#define ttype(o)	((o)->tt)
#define gcvalue(o)	check_exp(iscollectable(o), (o)->value.gc)
#define pvalue(o)	check_exp(ttislightuserdata(o), (o)->value.p)
#define nvalue(o)	check_exp(ttisnumber(o), (o)->value.n)
#define tsvalue(o)	check_exp(ttisstring(o), &(o)->value.gc->ts)
#define uvalue(o)	check_exp(ttisuserdata(o), &(o)->value.gc->u)
#define clvalue(o)	check_exp(ttisfunction(o), &(o)->value.gc->cl)
#define hvalue(o)	check_exp(ttistable(o), &(o)->value.gc->h)
#define bvalue(o)	check_exp(ttisboolean(o), (o)->value.b)
#define thvalue(o)	check_exp(ttisthread(o), &(o)->value.gc->th)

#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

/* Macros to set values */
#define setnvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TNUMBER; i_o->value.n=(x); }

#define chgnvalue(obj,x) \
	check_exp(ttype(obj)==LUA_TNUMBER, (obj)->value.n=(x))

#define setpvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TLIGHTUSERDATA; i_o->value.p=(x); }

#define setbvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TBOOLEAN; i_o->value.b=(x); }

#define setsvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TSTRING; \
    i_o->value.gc=cast(GCObject *, (x)); \
    lua_assert(i_o->value.gc->gch.tt == LUA_TSTRING); }

#define setuvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TUSERDATA; \
    i_o->value.gc=cast(GCObject *, (x)); \
    lua_assert(i_o->value.gc->gch.tt == LUA_TUSERDATA); }

#define setthvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TTHREAD; \
    i_o->value.gc=cast(GCObject *, (x)); \
    lua_assert(i_o->value.gc->gch.tt == LUA_TTHREAD); }

#define setclvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TFUNCTION; \
    i_o->value.gc=cast(GCObject *, (x)); \
    lua_assert(i_o->value.gc->gch.tt == LUA_TFUNCTION); }

#define sethvalue(obj,x) \
  { TObject *i_o=(obj); i_o->tt=LUA_TTABLE; \
    i_o->value.gc=cast(GCObject *, (x)); \
    lua_assert(i_o->value.gc->gch.tt == LUA_TTABLE); }

#define setnilvalue(obj) ((obj)->tt=LUA_TNIL)



/*
** for internal debug only
*/
#define checkconsistency(obj) \
  lua_assert(!iscollectable(obj) || (ttype(obj) == (obj)->value.gc->gch.tt))


#define setobj(obj1,obj2) \
  { const TObject *o2=(obj2); TObject *o1=(obj1); \
    checkconsistency(o2); \
    o1->tt=o2->tt; o1->value = o2->value; }


/*
** different types of sets, according to destination
*/

/* from stack to (same) stack */
#define setobjs2s	setobj
/* to stack (not from same stack) */
#define setobj2s	setobj
#define setsvalue2s	setsvalue
/* from table to same table */
#define setobjt2t	setobj
/* to table */
#define setobj2t	setobj
/* to new object */
#define setobj2n	setobj
#define setsvalue2n	setsvalue

#define setttype(obj, tt) (ttype(obj) = (tt))


#define iscollectable(o)	(ttype(o) >= LUA_TSTRING)



typedef TObject *StkId;  /* index to stack elements */


/*
** String headers for string table
*/
typedef union TString {
  L_Umaxalign dummy;  /* ensures maximum alignment for strings */
  struct {
    CommonHeader;
    lu_byte reserved;
    lu_hash hash;
    size_t len;
  } tsv;
} TString;


#define getstr(ts)	cast(const char *, (ts) + 1)
#define svalue(o)       getstr(tsvalue(o))



typedef union Udata {
  L_Umaxalign dummy;  /* ensures maximum alignment for `local' udata */
  struct {
    CommonHeader;
    struct Table *metatable;
    size_t len;
  } uv;
} Udata;




/*
** Function Prototypes
*/
typedef struct Proto {
  CommonHeader;
  TObject *k;  /* constants used by the function */
  Instruction *code;
  struct Proto **p;  /* functions defined inside the function */
  int *lineinfo;  /* map from opcodes to source lines */
  struct LocVar *locvars;  /* information about local variables */
  TString  *source;
  int sizek;  /* size of `k' */
  int sizecode;
  int sizelineinfo;
  int sizep;  /* size of `p' */
  int sizelocvars;
  int lineDefined;
  GCObject *gclist;
  lu_byte nupvalues;
  lu_byte numparams;
  lu_byte is_vararg;
  lu_byte maxstacksize;
} Proto;


typedef struct LocVar {
  TString *varname;
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
} LocVar;



/*
** Upvalues
*/

typedef struct UpVal {
  CommonHeader;
  TObject *v;  /* points to stack or to its own value */
  TObject value;  /* the value (when closed) */
} UpVal;


/*
** Closures
*/

#define ClosureHeader \
	CommonHeader; lu_byte isC; lu_byte nupvalues; GCObject *gclist

typedef struct CClosure {
  ClosureHeader;
  lua_CFunction f;
  TObject upvalue[1];
} CClosure;


typedef struct LClosure {
  ClosureHeader;
  struct Proto *p;
  TObject g;  /* global table for this closure */
  UpVal *upvals[1];
} LClosure;


typedef union Closure {
  CClosure c;
  LClosure l;
} Closure;


#define iscfunction(o)	(ttype(o) == LUA_TFUNCTION && clvalue(o)->c.isC)
#define isLfunction(o)	(ttype(o) == LUA_TFUNCTION && !clvalue(o)->c.isC)


/*
** Tables
*/

typedef struct Node {
  TObject i_key;
  TObject i_val;
  struct Node *next;  /* for chaining */
} Node;


typedef struct Table {
  CommonHeader;
  lu_byte flags;  /* 1<<p means tagmethod(p) is not present */ 
  lu_byte lsizenode;  /* log2 of size of `node' array */
  struct Table *metatable;
  TObject *array;  /* array part */
  Node *node;
  Node *firstfree;  /* this position is free; all positions after it are full */
  GCObject *gclist;
  int sizearray;  /* size of `array' array */
} Table;



/*
** `module' operation for hashing (size is always a power of 2)
*/
#define lmod(s,size) \
	check_exp((size&(size-1))==0, (cast(int, (s) & ((size)-1))))


#define twoto(x)	(1<<(x))
#define sizenode(t)	(twoto((t)->lsizenode))
#define sizearray(t)	((t)->sizearray)



extern const TObject luaO_nilobject;

int luaO_log2 (unsigned int x);


int luaO_rawequalObj (const TObject *t1, const TObject *t2);
int luaO_str2d (const char *s, lua_Number *result);

const char *luaO_pushvfstring (lua_State *L, const char *fmt, va_list argp);
const char *luaO_pushfstring (lua_State *L, const char *fmt, ...);
void luaO_chunkid (char *out, const char *source, int len);


#endif
