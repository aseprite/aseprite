/*
** $Id: lundump.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** load pre-compiled Lua chunks
** See Copyright Notice in lua.h
*/

#define lundump_c

#include "lua.h"

#include "ldebug.h"
#include "lfunc.h"
#include "lmem.h"
#include "lopcodes.h"
#include "lstring.h"
#include "lundump.h"
#include "lzio.h"

#define	LoadByte	(lu_byte) ezgetc

typedef struct {
 lua_State* L;
 ZIO* Z;
 Mbuffer* b;
 int swap;
 const char* name;
} LoadState;

static void unexpectedEOZ (LoadState* S)
{
 luaG_runerror(S->L,"unexpected end of file in %s",S->name);
}

static int ezgetc (LoadState* S)
{
 int c=zgetc(S->Z);
 if (c==EOZ) unexpectedEOZ(S);
 return c;
}

static void ezread (LoadState* S, void* b, int n)
{
 int r=luaZ_read(S->Z,b,n);
 if (r!=0) unexpectedEOZ(S);
}

static void LoadBlock (LoadState* S, void* b, size_t size)
{
 if (S->swap)
 {
  char* p=(char*) b+size-1;
  int n=size;
  while (n--) *p--=(char)ezgetc(S);
 }
 else
  ezread(S,b,size);
}

static void LoadVector (LoadState* S, void* b, int m, size_t size)
{
 if (S->swap)
 {
  char* q=(char*) b;
  while (m--)
  {
   char* p=q+size-1;
   int n=size;
   while (n--) *p--=(char)ezgetc(S);
   q+=size;
  }
 }
 else
  ezread(S,b,m*size);
}

static int LoadInt (LoadState* S)
{
 int x;
 LoadBlock(S,&x,sizeof(x));
 if (x<0) luaG_runerror(S->L,"bad integer in %s",S->name);
 return x;
}

static size_t LoadSize (LoadState* S)
{
 size_t x;
 LoadBlock(S,&x,sizeof(x));
 return x;
}

static lua_Number LoadNumber (LoadState* S)
{
 lua_Number x;
 LoadBlock(S,&x,sizeof(x));
 return x;
}

static TString* LoadString (LoadState* S)
{
 size_t size=LoadSize(S);
 if (size==0)
  return NULL;
 else
 {
  char* s=luaZ_openspace(S->L,S->b,size);
  ezread(S,s,size);
  return luaS_newlstr(S->L,s,size-1);	/* remove trailing '\0' */
 }
}

static void LoadCode (LoadState* S, Proto* f)
{
 int size=LoadInt(S);
 f->code=luaM_newvector(S->L,size,Instruction);
 f->sizecode=size;
 LoadVector(S,f->code,size,sizeof(*f->code));
}

static void LoadLocals (LoadState* S, Proto* f)
{
 int i,n;
 n=LoadInt(S);
 f->locvars=luaM_newvector(S->L,n,LocVar);
 f->sizelocvars=n;
 for (i=0; i<n; i++)
 {
  f->locvars[i].varname=LoadString(S);
  f->locvars[i].startpc=LoadInt(S);
  f->locvars[i].endpc=LoadInt(S);
 }
}

static void LoadLines (LoadState* S, Proto* f)
{
 int size=LoadInt(S);
 f->lineinfo=luaM_newvector(S->L,size,int);
 f->sizelineinfo=size;
 LoadVector(S,f->lineinfo,size,sizeof(*f->lineinfo));
}

static Proto* LoadFunction (LoadState* S, TString* p);

static void LoadConstants (LoadState* S, Proto* f)
{
 int i,n;
 n=LoadInt(S);
 f->k=luaM_newvector(S->L,n,TObject);
 f->sizek=n;
 for (i=0; i<n; i++)
 {
  TObject* o=&f->k[i];
  int t=LoadByte(S);
  switch (t)
  {
   case LUA_TNUMBER:
	setnvalue(o,LoadNumber(S));
	break;
   case LUA_TSTRING:
	setsvalue2n(o,LoadString(S));
	break;
   case LUA_TNIL:
   	setnilvalue(o);
	break;
   default:
	luaG_runerror(S->L,"bad constant type (%d) in %s",t,S->name);
	break;
  }
 }
 n=LoadInt(S);
 f->p=luaM_newvector(S->L,n,Proto*);
 f->sizep=n;
 for (i=0; i<n; i++) f->p[i]=LoadFunction(S,f->source);
}

static Proto* LoadFunction (LoadState* S, TString* p)
{
 Proto* f=luaF_newproto(S->L);
 f->source=LoadString(S); if (f->source==NULL) f->source=p;
 f->lineDefined=LoadInt(S);
 f->nupvalues=LoadByte(S);
 f->numparams=LoadByte(S);
 f->is_vararg=LoadByte(S);
 f->maxstacksize=LoadByte(S);
 LoadLocals(S,f);
 LoadLines(S,f);
 LoadConstants(S,f);
 LoadCode(S,f);
#ifndef TRUST_BINARIES
 if (!luaG_checkcode(f)) luaG_runerror(S->L,"bad code in %s",S->name);
#endif
 return f;
}

static void LoadSignature (LoadState* S)
{
 const char* s=LUA_SIGNATURE;
 while (*s!=0 && ezgetc(S)==*s)
  ++s;
 if (*s!=0) luaG_runerror(S->L,"bad signature in %s",S->name);
}

static void TestSize (LoadState* S, int s, const char* what)
{
 int r=LoadByte(S);
 if (r!=s)
  luaG_runerror(S->L,"virtual machine mismatch in %s: "
	"size of %s is %d but read %d",S->name,what,s,r);
}

#define TESTSIZE(s,w)	TestSize(S,s,w)
#define V(v)		v/16,v%16

static void LoadHeader (LoadState* S)
{
 int version;
 lua_Number x=0,tx=TEST_NUMBER;
 LoadSignature(S);
 version=LoadByte(S);
 if (version>VERSION)
  luaG_runerror(S->L,"%s too new: "
	"read version %d.%d; expected at most %d.%d",
	S->name,V(version),V(VERSION));
 if (version<VERSION0)			/* check last major change */
  luaG_runerror(S->L,"%s too old: "
	"read version %d.%d; expected at least %d.%d",
	S->name,V(version),V(VERSION0));
 S->swap=(luaU_endianness()!=LoadByte(S));	/* need to swap bytes? */
 TESTSIZE(sizeof(int),"int");
 TESTSIZE(sizeof(size_t), "size_t");
 TESTSIZE(sizeof(Instruction), "Instruction");
 TESTSIZE(SIZE_OP, "OP");
 TESTSIZE(SIZE_A, "A");
 TESTSIZE(SIZE_B, "B");
 TESTSIZE(SIZE_C, "C");
 TESTSIZE(sizeof(lua_Number), "number");
 x=LoadNumber(S);
 if ((long)x!=(long)tx)		/* disregard errors in last bits of fraction */
  luaG_runerror(S->L,"unknown number format in %s",S->name);
}

static Proto* LoadChunk (LoadState* S)
{
 LoadHeader(S);
 return LoadFunction(S,NULL);
}

/*
** load precompiled chunk
*/
Proto* luaU_undump (lua_State* L, ZIO* Z, Mbuffer* buff)
{
 LoadState S;
 const char* s=zname(Z);
 if (*s=='@' || *s=='=')
  S.name=s+1;
 else if (*s==LUA_SIGNATURE[0])
  S.name="binary string";
 else
  S.name=s;
 S.L=L;
 S.Z=Z;
 S.b=buff;
 return LoadChunk(&S);
}

/*
** find byte order
*/
int luaU_endianness (void)
{
 int x=1;
 return *(char*)&x;
}
