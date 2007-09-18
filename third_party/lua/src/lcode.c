/*
** $Id: lcode.c,v 1.1 2004/03/09 02:45:59 dacap Exp $
** Code generator for Lua
** See Copyright Notice in lua.h
*/


#include <stdlib.h>

#define lcode_c

#include "lua.h"

#include "lcode.h"
#include "ldebug.h"
#include "ldo.h"
#include "llex.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "ltable.h"


#define hasjumps(e)	((e)->t != (e)->f)


void luaK_nil (FuncState *fs, int from, int n) {
  Instruction *previous;
  if (fs->pc > fs->lasttarget &&  /* no jumps to current position? */
      GET_OPCODE(*(previous = &fs->f->code[fs->pc-1])) == OP_LOADNIL) {
    int pfrom = GETARG_A(*previous);
    int pto = GETARG_B(*previous);
    if (pfrom <= from && from <= pto+1) {  /* can connect both? */
      if (from+n-1 > pto)
        SETARG_B(*previous, from+n-1);
      return;
    }
  }
  luaK_codeABC(fs, OP_LOADNIL, from, from+n-1, 0);  /* else no optimization */
}


int luaK_jump (FuncState *fs) {
  int jpc = fs->jpc;  /* save list of jumps to here */
  int j;
  fs->jpc = NO_JUMP;
  j = luaK_codeAsBx(fs, OP_JMP, 0, NO_JUMP);
  luaK_concat(fs, &j, jpc);  /* keep them on hold */
  return j;
}


static int luaK_condjump (FuncState *fs, OpCode op, int A, int B, int C) {
  luaK_codeABC(fs, op, A, B, C);
  return luaK_jump(fs);
}


static void luaK_fixjump (FuncState *fs, int pc, int dest) {
  Instruction *jmp = &fs->f->code[pc];
  int offset = dest-(pc+1);
  lua_assert(dest != NO_JUMP);
  if (abs(offset) > MAXARG_sBx)
    luaX_syntaxerror(fs->ls, "control structure too long");
  SETARG_sBx(*jmp, offset);
}


/*
** returns current `pc' and marks it as a jump target (to avoid wrong
** optimizations with consecutive instructions not in the same basic block).
*/
int luaK_getlabel (FuncState *fs) {
  fs->lasttarget = fs->pc;
  return fs->pc;
}


static int luaK_getjump (FuncState *fs, int pc) {
  int offset = GETARG_sBx(fs->f->code[pc]);
  if (offset == NO_JUMP)  /* point to itself represents end of list */
    return NO_JUMP;  /* end of list */
  else
    return (pc+1)+offset;  /* turn offset into absolute position */
}


static Instruction *getjumpcontrol (FuncState *fs, int pc) {
  Instruction *pi = &fs->f->code[pc];
  if (pc >= 1 && testOpMode(GET_OPCODE(*(pi-1)), OpModeT))
    return pi-1;
  else
    return pi;
}


/*
** check whether list has any jump that do not produce a value
** (or produce an inverted value)
*/
static int need_value (FuncState *fs, int list, int cond) {
  for (; list != NO_JUMP; list = luaK_getjump(fs, list)) {
    Instruction i = *getjumpcontrol(fs, list);
    if (GET_OPCODE(i) != OP_TEST || GETARG_C(i) != cond) return 1;
  }
  return 0;  /* not found */
}


static void patchtestreg (Instruction *i, int reg) {
  if (reg == NO_REG) reg = GETARG_B(*i);
  SETARG_A(*i, reg);
}


static void luaK_patchlistaux (FuncState *fs, int list,
          int ttarget, int treg, int ftarget, int freg, int dtarget) {
  while (list != NO_JUMP) {
    int next = luaK_getjump(fs, list);
    Instruction *i = getjumpcontrol(fs, list);
    if (GET_OPCODE(*i) != OP_TEST) {
      lua_assert(dtarget != NO_JUMP);
      luaK_fixjump(fs, list, dtarget);  /* jump to default target */
    }
    else {
      if (GETARG_C(*i)) {
        lua_assert(ttarget != NO_JUMP);
        patchtestreg(i, treg);
        luaK_fixjump(fs, list, ttarget);
      }
      else {
        lua_assert(ftarget != NO_JUMP);
        patchtestreg(i, freg);
        luaK_fixjump(fs, list, ftarget);
      }
    }
    list = next;
  }
}


static void luaK_dischargejpc (FuncState *fs) {
  luaK_patchlistaux(fs, fs->jpc, fs->pc, NO_REG, fs->pc, NO_REG, fs->pc);
  fs->jpc = NO_JUMP;
}


void luaK_patchlist (FuncState *fs, int list, int target) {
  if (target == fs->pc)
    luaK_patchtohere(fs, list);
  else {
    lua_assert(target < fs->pc);
    luaK_patchlistaux(fs, list, target, NO_REG, target, NO_REG, target);
  }
}


void luaK_patchtohere (FuncState *fs, int list) {
  luaK_getlabel(fs);
  luaK_concat(fs, &fs->jpc, list);
}


void luaK_concat (FuncState *fs, int *l1, int l2) {
  if (l2 == NO_JUMP) return;
  else if (*l1 == NO_JUMP)
    *l1 = l2;
  else {
    int list = *l1;
    int next;
    while ((next = luaK_getjump(fs, list)) != NO_JUMP)  /* find last element */
      list = next;
    luaK_fixjump(fs, list, l2);
  }
}


void luaK_checkstack (FuncState *fs, int n) {
  int newstack = fs->freereg + n;
  if (newstack > fs->f->maxstacksize) {
    if (newstack >= MAXSTACK)
      luaX_syntaxerror(fs->ls, "function or expression too complex");
    fs->f->maxstacksize = cast(lu_byte, newstack);
  }
}


void luaK_reserveregs (FuncState *fs, int n) {
  luaK_checkstack(fs, n);
  fs->freereg += n;
}


static void freereg (FuncState *fs, int reg) {
  if (reg >= fs->nactvar && reg < MAXSTACK) {
    fs->freereg--;
    lua_assert(reg == fs->freereg);
  }
}


static void freeexp (FuncState *fs, expdesc *e) {
  if (e->k == VNONRELOC)
    freereg(fs, e->info);
}


static int addk (FuncState *fs, TObject *k, TObject *v) {
  const TObject *index = luaH_get(fs->h, k);
  if (ttisnumber(index)) {
    lua_assert(luaO_rawequalObj(&fs->f->k[cast(int, nvalue(index))], v));
    return cast(int, nvalue(index));
  }
  else {  /* constant not found; create a new entry */
    Proto *f = fs->f;
    luaM_growvector(fs->L, f->k, fs->nk, f->sizek, TObject,
                    MAXARG_Bx, "constant table overflow");
    setobj2n(&f->k[fs->nk], v);
    setnvalue(luaH_set(fs->L, fs->h, k), fs->nk);
    return fs->nk++;
  }
}


int luaK_stringK (FuncState *fs, TString *s) {
  TObject o;
  setsvalue(&o, s);
  return addk(fs, &o, &o);
}


int luaK_numberK (FuncState *fs, lua_Number r) {
  TObject o;
  setnvalue(&o, r);
  return addk(fs, &o, &o);
}


static int nil_constant (FuncState *fs) {
  TObject k, v;
  setnilvalue(&v);
  sethvalue(&k, fs->h);  /* cannot use nil as key; instead use table itself */
  return addk(fs, &k, &v);
}


void luaK_setcallreturns (FuncState *fs, expdesc *e, int nresults) {
  if (e->k == VCALL) {  /* expression is an open function call? */
    SETARG_C(getcode(fs, e), nresults+1);
    if (nresults == 1) {  /* `regular' expression? */
      e->k = VNONRELOC;
      e->info = GETARG_A(getcode(fs, e));
    }
  }
}


void luaK_dischargevars (FuncState *fs, expdesc *e) {
  switch (e->k) {
    case VLOCAL: {
      e->k = VNONRELOC;
      break;
    }
    case VUPVAL: {
      e->info = luaK_codeABC(fs, OP_GETUPVAL, 0, e->info, 0);
      e->k = VRELOCABLE;
      break;
    }
    case VGLOBAL: {
      e->info = luaK_codeABx(fs, OP_GETGLOBAL, 0, e->info);
      e->k = VRELOCABLE;
      break;
    }
    case VINDEXED: {
      freereg(fs, e->aux);
      freereg(fs, e->info);
      e->info = luaK_codeABC(fs, OP_GETTABLE, 0, e->info, e->aux);
      e->k = VRELOCABLE;
      break;
    }
    case VCALL: {
      luaK_setcallreturns(fs, e, 1);
      break;
    }
    default: break;  /* there is one value available (somewhere) */
  }
}


static int code_label (FuncState *fs, int A, int b, int jump) {
  luaK_getlabel(fs);  /* those instructions may be jump targets */
  return luaK_codeABC(fs, OP_LOADBOOL, A, b, jump);
}


static void discharge2reg (FuncState *fs, expdesc *e, int reg) {
  luaK_dischargevars(fs, e);
  switch (e->k) {
    case VNIL: {
      luaK_nil(fs, reg, 1);
      break;
    }
    case VFALSE:  case VTRUE: {
      luaK_codeABC(fs, OP_LOADBOOL, reg, e->k == VTRUE, 0);
      break;
    }
    case VK: {
      luaK_codeABx(fs, OP_LOADK, reg, e->info);
      break;
    }
    case VRELOCABLE: {
      Instruction *pc = &getcode(fs, e);
      SETARG_A(*pc, reg);
      break;
    }
    case VNONRELOC: {
      if (reg != e->info)
        luaK_codeABC(fs, OP_MOVE, reg, e->info, 0);
      break;
    }
    default: {
      lua_assert(e->k == VVOID || e->k == VJMP);
      return;  /* nothing to do... */
    }
  }
  e->info = reg;
  e->k = VNONRELOC;
}


static void discharge2anyreg (FuncState *fs, expdesc *e) {
  if (e->k != VNONRELOC) {
    luaK_reserveregs(fs, 1);
    discharge2reg(fs, e, fs->freereg-1);
  }
}


static void luaK_exp2reg (FuncState *fs, expdesc *e, int reg) {
  discharge2reg(fs, e, reg);
  if (e->k == VJMP)
    luaK_concat(fs, &e->t, e->info);  /* put this jump in `t' list */
  if (hasjumps(e)) {
    int final;  /* position after whole expression */
    int p_f = NO_JUMP;  /* position of an eventual LOAD false */
    int p_t = NO_JUMP;  /* position of an eventual LOAD true */
    if (need_value(fs, e->t, 1) || need_value(fs, e->f, 0)) {
      int fj = NO_JUMP;  /* first jump (over LOAD ops.) */
      if (e->k != VJMP)
        fj = luaK_jump(fs);
      p_f = code_label(fs, reg, 0, 1);
      p_t = code_label(fs, reg, 1, 0);
      luaK_patchtohere(fs, fj);
    }
    final = luaK_getlabel(fs);
    luaK_patchlistaux(fs, e->f, p_f, NO_REG, final, reg, p_f);
    luaK_patchlistaux(fs, e->t, final, reg, p_t, NO_REG, p_t);
  }
  e->f = e->t = NO_JUMP;
  e->info = reg;
  e->k = VNONRELOC;
}


void luaK_exp2nextreg (FuncState *fs, expdesc *e) {
  luaK_dischargevars(fs, e);
  freeexp(fs, e);
  luaK_reserveregs(fs, 1);
  luaK_exp2reg(fs, e, fs->freereg - 1);
}


int luaK_exp2anyreg (FuncState *fs, expdesc *e) {
  luaK_dischargevars(fs, e);
  if (e->k == VNONRELOC) {
    if (!hasjumps(e)) return e->info;  /* exp is already in a register */ 
    if (e->info >= fs->nactvar) {  /* reg. is not a local? */
      luaK_exp2reg(fs, e, e->info);  /* put value on it */
      return e->info;
    }
  }
  luaK_exp2nextreg(fs, e);  /* default */
  return e->info;
}


void luaK_exp2val (FuncState *fs, expdesc *e) {
  if (hasjumps(e))
    luaK_exp2anyreg(fs, e);
  else
    luaK_dischargevars(fs, e);
}


int luaK_exp2RK (FuncState *fs, expdesc *e) {
  luaK_exp2val(fs, e);
  switch (e->k) {
    case VNIL: {
      if (fs->nk + MAXSTACK <= MAXARG_C) {  /* constant fit in argC? */
        e->info = nil_constant(fs);
        e->k = VK;
        return e->info + MAXSTACK;
      }
      else break;
    }
    case VK: {
      if (e->info + MAXSTACK <= MAXARG_C)  /* constant fit in argC? */
        return e->info + MAXSTACK;
      else break;
    }
    default: break;
  }
  /* not a constant in the right range: put it in a register */
  return luaK_exp2anyreg(fs, e);
}


void luaK_storevar (FuncState *fs, expdesc *var, expdesc *exp) {
  switch (var->k) {
    case VLOCAL: {
      freeexp(fs, exp);
      luaK_exp2reg(fs, exp, var->info);
      return;
    }
    case VUPVAL: {
      int e = luaK_exp2anyreg(fs, exp);
      luaK_codeABC(fs, OP_SETUPVAL, e, var->info, 0);
      break;
    }
    case VGLOBAL: {
      int e = luaK_exp2anyreg(fs, exp);
      luaK_codeABx(fs, OP_SETGLOBAL, e, var->info);
      break;
    }
    case VINDEXED: {
      int e = luaK_exp2RK(fs, exp);
      luaK_codeABC(fs, OP_SETTABLE, var->info, var->aux, e);
      break;
    }
    default: {
      lua_assert(0);  /* invalid var kind to store */
      break;
    }
  }
  freeexp(fs, exp);
}


void luaK_self (FuncState *fs, expdesc *e, expdesc *key) {
  int func;
  luaK_exp2anyreg(fs, e);
  freeexp(fs, e);
  func = fs->freereg;
  luaK_reserveregs(fs, 2);
  luaK_codeABC(fs, OP_SELF, func, e->info, luaK_exp2RK(fs, key));
  freeexp(fs, key);
  e->info = func;
  e->k = VNONRELOC;
}


static void invertjump (FuncState *fs, expdesc *e) {
  Instruction *pc = getjumpcontrol(fs, e->info);
  lua_assert(testOpMode(GET_OPCODE(*pc), OpModeT) &&
             GET_OPCODE(*pc) != OP_TEST);
  SETARG_A(*pc, !(GETARG_A(*pc)));
}


static int jumponcond (FuncState *fs, expdesc *e, int cond) {
  if (e->k == VRELOCABLE) {
    Instruction ie = getcode(fs, e);
    if (GET_OPCODE(ie) == OP_NOT) {
      fs->pc--;  /* remove previous OP_NOT */
      return luaK_condjump(fs, OP_TEST, NO_REG, GETARG_B(ie), !cond);
    }
    /* else go through */
  }
  discharge2anyreg(fs, e);
  freeexp(fs, e);
  return luaK_condjump(fs, OP_TEST, NO_REG, e->info, cond);
}


void luaK_goiftrue (FuncState *fs, expdesc *e) {
  int pc;  /* pc of last jump */
  luaK_dischargevars(fs, e);
  switch (e->k) {
    case VK: case VTRUE: {
      pc = NO_JUMP;  /* always true; do nothing */
      break;
    }
    case VFALSE: {
      pc = luaK_jump(fs);  /* always jump */
      break;
    }
    case VJMP: {
      invertjump(fs, e);
      pc = e->info;
      break;
    }
    default: {
      pc = jumponcond(fs, e, 0);
      break;
    }
  }
  luaK_concat(fs, &e->f, pc);  /* insert last jump in `f' list */
}


void luaK_goiffalse (FuncState *fs, expdesc *e) {
  int pc;  /* pc of last jump */
  luaK_dischargevars(fs, e);
  switch (e->k) {
    case VNIL: case VFALSE: {
      pc = NO_JUMP;  /* always false; do nothing */
      break;
    }
    case VTRUE: {
      pc = luaK_jump(fs);  /* always jump */
      break;
    }
    case VJMP: {
      pc = e->info;
      break;
    }
    default: {
      pc = jumponcond(fs, e, 1);
      break;
    }
  }
  luaK_concat(fs, &e->t, pc);  /* insert last jump in `t' list */
}


static void codenot (FuncState *fs, expdesc *e) {
  luaK_dischargevars(fs, e);
  switch (e->k) {
    case VNIL: case VFALSE: {
      e->k = VTRUE;
      break;
    }
    case VK: case VTRUE: {
      e->k = VFALSE;
      break;
    }
    case VJMP: {
      invertjump(fs, e);
      break;
    }
    case VRELOCABLE:
    case VNONRELOC: {
      discharge2anyreg(fs, e);
      freeexp(fs, e);
      e->info = luaK_codeABC(fs, OP_NOT, 0, e->info, 0);
      e->k = VRELOCABLE;
      break;
    }
    default: {
      lua_assert(0);  /* cannot happen */
      break;
    }
  }
  /* interchange true and false lists */
  { int temp = e->f; e->f = e->t; e->t = temp; }
}


void luaK_indexed (FuncState *fs, expdesc *t, expdesc *k) {
  t->aux = luaK_exp2RK(fs, k);
  t->k = VINDEXED;
}


void luaK_prefix (FuncState *fs, UnOpr op, expdesc *e) {
  if (op == OPR_MINUS) {
    luaK_exp2val(fs, e);
    if (e->k == VK && ttisnumber(&fs->f->k[e->info]))
      e->info = luaK_numberK(fs, -nvalue(&fs->f->k[e->info]));
    else {
      luaK_exp2anyreg(fs, e);
      freeexp(fs, e);
      e->info = luaK_codeABC(fs, OP_UNM, 0, e->info, 0);
      e->k = VRELOCABLE;
    }
  }
  else  /* op == NOT */
    codenot(fs, e);
}


void luaK_infix (FuncState *fs, BinOpr op, expdesc *v) {
  switch (op) {
    case OPR_AND: {
      luaK_goiftrue(fs, v);
      luaK_patchtohere(fs, v->t);
      v->t = NO_JUMP;
      break;
    }
    case OPR_OR: {
      luaK_goiffalse(fs, v);
      luaK_patchtohere(fs, v->f);
      v->f = NO_JUMP;
      break;
    }
    case OPR_CONCAT: {
      luaK_exp2nextreg(fs, v);  /* operand must be on the `stack' */
      break;
    }
    default: {
      luaK_exp2RK(fs, v);
      break;
    }
  }
}


static void codebinop (FuncState *fs, expdesc *res, BinOpr op,
                       int o1, int o2) {
  if (op <= OPR_POW) {  /* arithmetic operator? */
    OpCode opc = cast(OpCode, (op - OPR_ADD) + OP_ADD);  /* ORDER OP */
    res->info = luaK_codeABC(fs, opc, 0, o1, o2);
    res->k = VRELOCABLE;
  }
  else {  /* test operator */
    static const OpCode ops[] = {OP_EQ, OP_EQ, OP_LT, OP_LE, OP_LT, OP_LE};
    int cond = 1;
    if (op >= OPR_GT) {  /* `>' or `>='? */
      int temp;  /* exchange args and replace by `<' or `<=' */
      temp = o1; o1 = o2; o2 = temp;  /* o1 <==> o2 */
    }
    else if (op == OPR_NE) cond = 0;
    res->info = luaK_condjump(fs, ops[op - OPR_NE], cond, o1, o2);
    res->k = VJMP;
  }
}


void luaK_posfix (FuncState *fs, BinOpr op, expdesc *e1, expdesc *e2) {
  switch (op) {
    case OPR_AND: {
      lua_assert(e1->t == NO_JUMP);  /* list must be closed */
      luaK_dischargevars(fs, e2);
      luaK_concat(fs, &e1->f, e2->f);
      e1->k = e2->k; e1->info = e2->info; e1->aux = e2->aux; e1->t = e2->t;
      break;
    }
    case OPR_OR: {
      lua_assert(e1->f == NO_JUMP);  /* list must be closed */
      luaK_dischargevars(fs, e2);
      luaK_concat(fs, &e1->t, e2->t);
      e1->k = e2->k; e1->info = e2->info; e1->aux = e2->aux; e1->f = e2->f;
      break;
    }
    case OPR_CONCAT: {
      luaK_exp2val(fs, e2);
      if (e2->k == VRELOCABLE && GET_OPCODE(getcode(fs, e2)) == OP_CONCAT) {
        lua_assert(e1->info == GETARG_B(getcode(fs, e2))-1);
        freeexp(fs, e1);
        SETARG_B(getcode(fs, e2), e1->info);
        e1->k = e2->k; e1->info = e2->info;
      }
      else {
        luaK_exp2nextreg(fs, e2);
        freeexp(fs, e2);
        freeexp(fs, e1);
        e1->info = luaK_codeABC(fs, OP_CONCAT, 0, e1->info, e2->info);
        e1->k = VRELOCABLE;
      }
      break;
    }
    default: {
      int o1 = luaK_exp2RK(fs, e1);
      int o2 = luaK_exp2RK(fs, e2);
      freeexp(fs, e2);
      freeexp(fs, e1);
      codebinop(fs, e1, op, o1, o2);
    }
  }
}


void luaK_fixline (FuncState *fs, int line) {
  fs->f->lineinfo[fs->pc - 1] = line;
}


int luaK_code (FuncState *fs, Instruction i, int line) {
  Proto *f = fs->f;
  luaK_dischargejpc(fs);  /* `pc' will change */
  /* put new instruction in code array */
  luaM_growvector(fs->L, f->code, fs->pc, f->sizecode, Instruction,
                  MAX_INT, "code size overflow");
  f->code[fs->pc] = i;
  /* save corresponding line information */
  luaM_growvector(fs->L, f->lineinfo, fs->pc, f->sizelineinfo, int,
                  MAX_INT, "code size overflow");
  f->lineinfo[fs->pc] = line;
  return fs->pc++;
}


int luaK_codeABC (FuncState *fs, OpCode o, int a, int b, int c) {
  lua_assert(getOpMode(o) == iABC);
  return luaK_code(fs, CREATE_ABC(o, a, b, c), fs->ls->lastline);
}


int luaK_codeABx (FuncState *fs, OpCode o, int a, unsigned int bc) {
  lua_assert(getOpMode(o) == iABx || getOpMode(o) == iAsBx);
  return luaK_code(fs, CREATE_ABx(o, a, bc), fs->ls->lastline);
}

