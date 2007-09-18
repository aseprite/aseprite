/*
** $Id: lopcodes.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** extracted automatically from lopcodes.h by mkprint.lua
** DO NOT EDIT
** See Copyright Notice in lua.h
*/


#define lopcodes_c

#include "lua.h"

#include "lobject.h"
#include "lopcodes.h"


#ifdef LUA_OPNAMES

const char *const luaP_opnames[] = {
  "MOVE",
  "LOADK",
  "LOADBOOL",
  "LOADNIL",
  "GETUPVAL",
  "GETGLOBAL",
  "GETTABLE",
  "SETGLOBAL",
  "SETUPVAL",
  "SETTABLE",
  "NEWTABLE",
  "SELF",
  "ADD",
  "SUB",
  "MUL",
  "DIV",
  "POW",
  "UNM",
  "NOT",
  "CONCAT",
  "JMP",
  "EQ",
  "LT",
  "LE",
  "TEST",
  "CALL",
  "TAILCALL",
  "RETURN",
  "FORLOOP",
  "TFORLOOP",
  "TFORPREP",
  "SETLIST",
  "SETLISTO",
  "CLOSE",
  "CLOSURE"
};

#endif

#define opmode(t,b,bk,ck,sa,k,m) (((t)<<OpModeT) | \
   ((b)<<OpModeBreg) | ((bk)<<OpModeBrk) | ((ck)<<OpModeCrk) | \
   ((sa)<<OpModesetA) | ((k)<<OpModeK) | (m))


const lu_byte luaP_opmodes[NUM_OPCODES] = {
/*       T  B Bk Ck sA  K  mode			   opcode    */
  opmode(0, 1, 0, 0, 1, 0, iABC)		/* OP_MOVE */
 ,opmode(0, 0, 0, 0, 1, 1, iABx)		/* OP_LOADK */
 ,opmode(0, 0, 0, 0, 1, 0, iABC)		/* OP_LOADBOOL */
 ,opmode(0, 1, 0, 0, 1, 0, iABC)		/* OP_LOADNIL */
 ,opmode(0, 0, 0, 0, 1, 0, iABC)		/* OP_GETUPVAL */
 ,opmode(0, 0, 0, 0, 1, 1, iABx)		/* OP_GETGLOBAL */
 ,opmode(0, 1, 0, 1, 1, 0, iABC)		/* OP_GETTABLE */
 ,opmode(0, 0, 0, 0, 0, 1, iABx)		/* OP_SETGLOBAL */
 ,opmode(0, 0, 0, 0, 0, 0, iABC)		/* OP_SETUPVAL */
 ,opmode(0, 0, 1, 1, 0, 0, iABC)		/* OP_SETTABLE */
 ,opmode(0, 0, 0, 0, 1, 0, iABC)		/* OP_NEWTABLE */
 ,opmode(0, 1, 0, 1, 1, 0, iABC)		/* OP_SELF */
 ,opmode(0, 0, 1, 1, 1, 0, iABC)		/* OP_ADD */
 ,opmode(0, 0, 1, 1, 1, 0, iABC)		/* OP_SUB */
 ,opmode(0, 0, 1, 1, 1, 0, iABC)		/* OP_MUL */
 ,opmode(0, 0, 1, 1, 1, 0, iABC)		/* OP_DIV */
 ,opmode(0, 0, 1, 1, 1, 0, iABC)		/* OP_POW */
 ,opmode(0, 1, 0, 0, 1, 0, iABC)		/* OP_UNM */
 ,opmode(0, 1, 0, 0, 1, 0, iABC)		/* OP_NOT */
 ,opmode(0, 1, 0, 1, 1, 0, iABC)		/* OP_CONCAT */
 ,opmode(0, 0, 0, 0, 0, 0, iAsBx)		/* OP_JMP */
 ,opmode(1, 0, 1, 1, 0, 0, iABC)		/* OP_EQ */
 ,opmode(1, 0, 1, 1, 0, 0, iABC)		/* OP_LT */
 ,opmode(1, 0, 1, 1, 0, 0, iABC)		/* OP_LE */
 ,opmode(1, 1, 0, 0, 1, 0, iABC)		/* OP_TEST */
 ,opmode(0, 0, 0, 0, 0, 0, iABC)		/* OP_CALL */
 ,opmode(0, 0, 0, 0, 0, 0, iABC)		/* OP_TAILCALL */
 ,opmode(0, 0, 0, 0, 0, 0, iABC)		/* OP_RETURN */
 ,opmode(0, 0, 0, 0, 0, 0, iAsBx)		/* OP_FORLOOP */
 ,opmode(1, 0, 0, 0, 0, 0, iABC)		/* OP_TFORLOOP */
 ,opmode(0, 0, 0, 0, 0, 0, iAsBx)		/* OP_TFORPREP */
 ,opmode(0, 0, 0, 0, 0, 0, iABx)		/* OP_SETLIST */
 ,opmode(0, 0, 0, 0, 0, 0, iABx)		/* OP_SETLISTO */
 ,opmode(0, 0, 0, 0, 0, 0, iABC)		/* OP_CLOSE */
 ,opmode(0, 0, 0, 0, 1, 0, iABx)		/* OP_CLOSURE */
};

