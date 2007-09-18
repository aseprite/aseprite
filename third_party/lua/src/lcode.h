/*
** $Id: lcode.h,v 1.1 2004/03/09 02:45:59 dacap Exp $
** Code generator for Lua
** See Copyright Notice in lua.h
*/

#ifndef lcode_h
#define lcode_h

#include "llex.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"


/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


/*
** grep "ORDER OPR" if you change these enums
*/
typedef enum BinOpr {
  OPR_ADD, OPR_SUB, OPR_MULT, OPR_DIV, OPR_POW,
  OPR_CONCAT,
  OPR_NE, OPR_EQ,
  OPR_LT, OPR_LE, OPR_GT, OPR_GE,
  OPR_AND, OPR_OR,
  OPR_NOBINOPR
} BinOpr;

#define binopistest(op)	((op) >= OPR_NE)

typedef enum UnOpr { OPR_MINUS, OPR_NOT, OPR_NOUNOPR } UnOpr;


#define getcode(fs,e)	((fs)->f->code[(e)->info])

#define luaK_codeAsBx(fs,o,A,sBx)	luaK_codeABx(fs,o,A,(sBx)+MAXARG_sBx)

int luaK_code (FuncState *fs, Instruction i, int line);
int luaK_codeABx (FuncState *fs, OpCode o, int A, unsigned int Bx);
int luaK_codeABC (FuncState *fs, OpCode o, int A, int B, int C);
void luaK_fixline (FuncState *fs, int line);
void luaK_nil (FuncState *fs, int from, int n);
void luaK_reserveregs (FuncState *fs, int n);
void luaK_checkstack (FuncState *fs, int n);
int luaK_stringK (FuncState *fs, TString *s);
int luaK_numberK (FuncState *fs, lua_Number r);
void luaK_dischargevars (FuncState *fs, expdesc *e);
int luaK_exp2anyreg (FuncState *fs, expdesc *e);
void luaK_exp2nextreg (FuncState *fs, expdesc *e);
void luaK_exp2val (FuncState *fs, expdesc *e);
int luaK_exp2RK (FuncState *fs, expdesc *e);
void luaK_self (FuncState *fs, expdesc *e, expdesc *key);
void luaK_indexed (FuncState *fs, expdesc *t, expdesc *k);
void luaK_goiftrue (FuncState *fs, expdesc *e);
void luaK_goiffalse (FuncState *fs, expdesc *e);
void luaK_storevar (FuncState *fs, expdesc *var, expdesc *e);
void luaK_setcallreturns (FuncState *fs, expdesc *var, int nresults);
int luaK_jump (FuncState *fs);
void luaK_patchlist (FuncState *fs, int list, int target);
void luaK_patchtohere (FuncState *fs, int list);
void luaK_concat (FuncState *fs, int *l1, int l2);
int luaK_getlabel (FuncState *fs);
void luaK_prefix (FuncState *fs, UnOpr op, expdesc *v);
void luaK_infix (FuncState *fs, BinOpr op, expdesc *v);
void luaK_posfix (FuncState *fs, BinOpr op, expdesc *v1, expdesc *v2);


#endif
