/*
** $Id: ltable.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Lua tables (hash)
** See Copyright Notice in lua.h
*/


/*
** Implementation of tables (aka arrays, objects, or hash tables).
** Tables keep its elements in two parts: an array part and a hash part.
** Non-negative integer keys are all candidates to be kept in the array
** part. The actual size of the array is the largest `n' such that at
** least half the slots between 0 and n are in use.
** Hash uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the `original' position that its hash gives
** to it), then the colliding element is in its own main position.
** In other words, there are collisions only when two elements have the
** same main position (i.e. the same hash values for that table size).
** Because of that, the load factor of these tables can be 100% without
** performance penalties.
*/


#define ltable_c

#include "lua.h"

#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "ltable.h"


/*
** max size of array part is 2^MAXBITS
*/
#if BITS_INT > 26
#define MAXBITS		24
#else
#define MAXBITS		(BITS_INT-2)
#endif

/* check whether `x' < 2^MAXBITS */
#define toobig(x)	((((x)-1) >> MAXBITS) != 0)


/* function to convert a lua_Number to int (with any rounding method) */
#ifndef lua_number2int
#define lua_number2int(i,n)	((i)=(int)(n))
#endif



#define hashnum(t,n)	\
	(node(t, lmod(cast(lu_hash, cast(ls_hash, n)), sizenode(t))))
#define hashstr(t,str)	(node(t, lmod((str)->tsv.hash, sizenode(t))))
#define hashboolean(t,p)	(node(t, lmod(p, sizenode(t))))

/*
** avoid modulus by power of 2 for pointers, as they tend to have many
** 2 factors.
*/
#define hashpointer(t,p)	(node(t, (IntPoint(p) % ((sizenode(t)-1)|1))))


/*
** returns the `main' position of an element in a table (that is, the index
** of its hash value)
*/
Node *luaH_mainposition (const Table *t, const TObject *key) {
  switch (ttype(key)) {
    case LUA_TNUMBER: {
      int ikey;
      lua_number2int(ikey, nvalue(key));
      return hashnum(t, ikey);
    }
    case LUA_TSTRING:
      return hashstr(t, tsvalue(key));
    case LUA_TBOOLEAN:
      return hashboolean(t, bvalue(key));
    case LUA_TLIGHTUSERDATA:
      return hashpointer(t, pvalue(key));
    default:
      return hashpointer(t, gcvalue(key));
  }
}


/*
** returns the index for `key' if `key' is an appropriate key to live in
** the array part of the table, -1 otherwise.
*/
static int arrayindex (const TObject *key) {
  if (ttisnumber(key)) {
    int k;
    lua_number2int(k, (nvalue(key)));
    if (cast(lua_Number, k) == nvalue(key) && k >= 1 && !toobig(k))
      return k;
  }
  return -1;  /* `key' did not match some condition */
}


/*
** returns the index of a `key' for table traversals. First goes all
** elements in the array part, then elements in the hash part. The
** beginning and end of a traversal are signalled by -1.
*/
static int luaH_index (lua_State *L, Table *t, StkId key) {
  int i;
  if (ttisnil(key)) return -1;  /* first iteration */
  i = arrayindex(key);
  if (0 <= i && i <= t->sizearray) {  /* is `key' inside array part? */
    return i-1;  /* yes; that's the index (corrected to C) */
  }
  else {
    const TObject *v = luaH_get(t, key);
    if (v == &luaO_nilobject)
      luaG_runerror(L, "invalid key for `next'");
    i = cast(int, (cast(const lu_byte *, v) -
                   cast(const lu_byte *, val(node(t, 0)))) / sizeof(Node));
    return i + t->sizearray;  /* hash elements are numbered after array ones */
  }
}


int luaH_next (lua_State *L, Table *t, StkId key) {
  int i = luaH_index(L, t, key);  /* find original element */
  for (i++; i < t->sizearray; i++) {  /* try first array part */
    if (!ttisnil(&t->array[i])) {  /* a non-nil value? */
      setnvalue(key, i+1);
      setobj2s(key+1, &t->array[i]);
      return 1;
    }
  }
  for (i -= t->sizearray; i < sizenode(t); i++) {  /* then hash part */
    if (!ttisnil(val(node(t, i)))) {  /* a non-nil value? */
      setobj2s(key, key(node(t, i)));
      setobj2s(key+1, val(node(t, i)));
      return 1;
    }
  }
  return 0;  /* no more elements */
}


/*
** {=============================================================
** Rehash
** ==============================================================
*/


static void computesizes  (int nums[], int ntotal, int *narray, int *nhash) {
  int i;
  int a = nums[0];  /* number of elements smaller than 2^i */
  int na = a;  /* number of elements to go to array part */
  int n = (na == 0) ? -1 : 0;  /* (log of) optimal size for array part */
  for (i = 1; i <= MAXBITS && *narray >= twoto(i-1); i++) {
    if (nums[i] > 0) {
      a += nums[i];
      if (a >= twoto(i-1)) {  /* more than half elements in use? */
        n = i;
        na = a;
      }
    }
  }
  lua_assert(na <= *narray && *narray <= ntotal);
  *nhash = ntotal - na;
  *narray = (n == -1) ? 0 : twoto(n);
  lua_assert(na <= *narray && na >= *narray/2);
}


static void numuse (const Table *t, int *narray, int *nhash) {
  int nums[MAXBITS+1];
  int i;
  int totaluse = 0;
  for (i=0; i<=MAXBITS; i++) nums[i] = 0;  /* init `nums' */
  /* count elements in array part */
  i = luaO_log2(t->sizearray) + 1;  /* number of `slices' */
  while (i--) {  /* for each slice [2^(i-1) to 2^i) */
    int to = twoto(i);
    int from = to/2;
    if (to > t->sizearray) to = t->sizearray;
    for (; from < to; from++)
      if (!ttisnil(&t->array[from])) {
        nums[i]++;
        totaluse++;
      }
  }
  *narray = totaluse;  /* all previous uses were in array part */
  /* count elements in hash part */
  i = sizenode(t);
  while (i--) {
    if (!ttisnil(val(&t->node[i]))) {
      int k = arrayindex(key(&t->node[i]));
      if (k >= 0) {  /* is `key' an appropriate array index? */
        nums[luaO_log2(k-1)+1]++;  /* count as such */
        (*narray)++;
      }
      totaluse++;
    }
  }
  computesizes(nums, totaluse, narray, nhash);
}


static void setarrayvector (lua_State *L, Table *t, int size) {
  int i;
  luaM_reallocvector(L, t->array, t->sizearray, size, TObject);
  for (i=t->sizearray; i<size; i++)
     setnilvalue(&t->array[i]);
  t->sizearray = size;
}


static void setnodevector (lua_State *L, Table *t, int lsize) {
  int i;
  int size = twoto(lsize);
  if (lsize > MAXBITS)
    luaG_runerror(L, "table overflow");
  if (lsize == 0) {  /* no elements to hash part? */
    t->node = G(L)->dummynode;  /* use common `dummynode' */
    lua_assert(ttisnil(key(t->node)));  /* assert invariants: */
    lua_assert(ttisnil(val(t->node)));
    lua_assert(t->node->next == NULL);  /* (`dummynode' must be empty) */
  }
  else {
    t->node = luaM_newvector(L, size, Node);
    for (i=0; i<size; i++) {
      t->node[i].next = NULL;
      setnilvalue(key(node(t, i)));
      setnilvalue(val(node(t, i)));
    }
  }
  t->lsizenode = cast(lu_byte, lsize);
  t->firstfree = node(t, size-1);  /* first free position to be used */
}


static void resize (lua_State *L, Table *t, int nasize, int nhsize) {
  int i;
  int oldasize = t->sizearray;
  int oldhsize = t->lsizenode;
  Node *nold;
  Node temp[1];
  if (oldhsize)
    nold = t->node;  /* save old hash ... */
  else {  /* old hash is `dummynode' */
    lua_assert(t->node == G(L)->dummynode);
    temp[0] = t->node[0];  /* copy it to `temp' */
    nold = temp;
    setnilvalue(key(G(L)->dummynode));  /* restate invariant */
    setnilvalue(val(G(L)->dummynode));
    lua_assert(G(L)->dummynode->next == NULL);
  }
  if (nasize > oldasize)  /* array part must grow? */
    setarrayvector(L, t, nasize);
  /* create new hash part with appropriate size */
  setnodevector(L, t, nhsize);  
  /* re-insert elements */
  if (nasize < oldasize) {  /* array part must shrink? */
    t->sizearray = nasize;
    /* re-insert elements from vanishing slice */
    for (i=nasize; i<oldasize; i++) {
      if (!ttisnil(&t->array[i]))
        setobjt2t(luaH_setnum(L, t, i+1), &t->array[i]);
    }
    /* shrink array */
    luaM_reallocvector(L, t->array, oldasize, nasize, TObject);
  }
  /* re-insert elements in hash part */
  for (i = twoto(oldhsize) - 1; i >= 0; i--) {
    Node *old = nold+i;
    if (!ttisnil(val(old)))
      setobjt2t(luaH_set(L, t, key(old)), val(old));
  }
  if (oldhsize)
    luaM_freearray(L, nold, twoto(oldhsize), Node);  /* free old array */
}


static void rehash (lua_State *L, Table *t) {
  int nasize, nhsize;
  numuse(t, &nasize, &nhsize);  /* compute new sizes for array and hash parts */
  resize(L, t, nasize, luaO_log2(nhsize)+1);
}



/*
** }=============================================================
*/


Table *luaH_new (lua_State *L, int narray, int lnhash) {
  Table *t = luaM_new(L, Table);
  luaC_link(L, valtogco(t), LUA_TTABLE);
  t->metatable = hvalue(defaultmeta(L));
  t->flags = cast(lu_byte, ~0);
  /* temporary values (kept only if some malloc fails) */
  t->array = NULL;
  t->sizearray = 0;
  t->lsizenode = 0;
  t->node = NULL;
  setarrayvector(L, t, narray);
  setnodevector(L, t, lnhash);
  return t;
}


void luaH_free (lua_State *L, Table *t) {
  if (t->lsizenode)
    luaM_freearray(L, t->node, sizenode(t), Node);
  luaM_freearray(L, t->array, t->sizearray, TObject);
  luaM_freelem(L, t);
}


#if 0
/*
** try to remove an element from a hash table; cannot move any element
** (because gc can call `remove' during a table traversal)
*/
void luaH_remove (Table *t, Node *e) {
  Node *mp = luaH_mainposition(t, key(e));
  if (e != mp) {  /* element not in its main position? */
    while (mp->next != e) mp = mp->next;  /* find previous */
    mp->next = e->next;  /* remove `e' from its list */
  }
  else {
    if (e->next != NULL) ??
  }
  lua_assert(ttisnil(val(node)));
  setnilvalue(key(e));  /* clear node `e' */
  e->next = NULL;
}
#endif


/*
** inserts a new key into a hash table; first, check whether key's main 
** position is free. If not, check whether colliding node is in its main 
** position or not: if it is not, move colliding node to an empty place and 
** put new key in its main position; otherwise (colliding node is in its main 
** position), new key goes to an empty position. 
*/
static TObject *newkey (lua_State *L, Table *t, const TObject *key) {
  TObject *val;
  Node *mp = luaH_mainposition(t, key);
  if (!ttisnil(val(mp))) {  /* main position is not free? */
    Node *othern = luaH_mainposition(t, key(mp));  /* `mp' of colliding node */
    Node *n = t->firstfree;  /* get a free place */
    if (othern != mp) {  /* is colliding node out of its main position? */
      /* yes; move colliding node into free position */
      while (othern->next != mp) othern = othern->next;  /* find previous */
      othern->next = n;  /* redo the chain with `n' in place of `mp' */
      *n = *mp;  /* copy colliding node into free pos. (mp->next also goes) */
      mp->next = NULL;  /* now `mp' is free */
      setnilvalue(val(mp));
    }
    else {  /* colliding node is in its own main position */
      /* new node will go into free position */
      n->next = mp->next;  /* chain new position */
      mp->next = n;
      mp = n;
    }
  }
  setobj2t(key(mp), key);  /* write barrier */
  lua_assert(ttisnil(val(mp)));
  for (;;) {  /* correct `firstfree' */
    if (ttisnil(key(t->firstfree)))
      return val(mp);  /* OK; table still has a free place */
    else if (t->firstfree == t->node) break;  /* cannot decrement from here */
    else (t->firstfree)--;
  }
  /* no more free places; must create one */
  setbvalue(val(mp), 0);  /* avoid new key being removed */
  rehash(L, t);  /* grow table */
  val = cast(TObject *, luaH_get(t, key));  /* get new position */
  lua_assert(ttisboolean(val));
  setnilvalue(val);
  return val;
}


/*
** generic search function
*/
static const TObject *luaH_getany (Table *t, const TObject *key) {
  if (ttisnil(key)) return &luaO_nilobject;
  else {
    Node *n = luaH_mainposition(t, key);
    do {  /* check whether `key' is somewhere in the chain */
      if (luaO_rawequalObj(key(n), key)) return val(n);  /* that's it */
      else n = n->next;
    } while (n);
    return &luaO_nilobject;
  }
}


/*
** search function for integers
*/
const TObject *luaH_getnum (Table *t, int key) {
  if (1 <= key && key <= t->sizearray)
    return &t->array[key-1];
  else {
    Node *n = hashnum(t, key);
    do {  /* check whether `key' is somewhere in the chain */
      if (ttisnumber(key(n)) && nvalue(key(n)) == (lua_Number)key)
        return val(n);  /* that's it */
      else n = n->next;
    } while (n);
    return &luaO_nilobject;
  }
}


/*
** search function for strings
*/
const TObject *luaH_getstr (Table *t, TString *key) {
  Node *n = hashstr(t, key);
  do {  /* check whether `key' is somewhere in the chain */
    if (ttisstring(key(n)) && tsvalue(key(n)) == key)
      return val(n);  /* that's it */
    else n = n->next;
  } while (n);
  return &luaO_nilobject;
}


/*
** main search function
*/
const TObject *luaH_get (Table *t, const TObject *key) {
  switch (ttype(key)) {
    case LUA_TSTRING: return luaH_getstr(t, tsvalue(key));
    case LUA_TNUMBER: {
      int k;
      lua_number2int(k, (nvalue(key)));
      if (cast(lua_Number, k) == nvalue(key))  /* is an integer index? */
        return luaH_getnum(t, k);  /* use specialized version */
      /* else go through */
    }
    default: return luaH_getany(t, key);
  }
}


TObject *luaH_set (lua_State *L, Table *t, const TObject *key) {
  const TObject *p = luaH_get(t, key);
  t->flags = 0;
  if (p != &luaO_nilobject)
    return cast(TObject *, p);
  else {
    if (ttisnil(key)) luaG_runerror(L, "table index is nil");
    else if (ttisnumber(key) && nvalue(key) != nvalue(key))
      luaG_runerror(L, "table index is NaN");
    return newkey(L, t, key);
  }
}


TObject *luaH_setnum (lua_State *L, Table *t, int key) {
  const TObject *p = luaH_getnum(t, key);
  if (p != &luaO_nilobject)
    return cast(TObject *, p);
  else {
    TObject k;
    setnvalue(&k, key);
    return newkey(L, t, &k);
  }
}

