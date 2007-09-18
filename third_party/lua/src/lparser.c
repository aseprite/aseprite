/*
** $Id: lparser.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Lua Parser
** See Copyright Notice in lua.h
*/


#include <string.h>

#define lparser_c

#include "lua.h"

#include "lcode.h"
#include "ldebug.h"
#include "lfunc.h"
#include "llex.h"
#include "lmem.h"
#include "lobject.h"
#include "lopcodes.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"




#define getlocvar(fs, i)	((fs)->f->locvars[(fs)->actvar[i]])


#define enterlevel(ls)	if (++(ls)->nestlevel > LUA_MAXPARSERLEVEL) \
		luaX_syntaxerror(ls, "too many syntax levels");
#define leavelevel(ls)	((ls)->nestlevel--)


/*
** nodes for block list (list of active blocks)
*/
typedef struct BlockCnt {
  struct BlockCnt *previous;  /* chain */
  int breaklist;  /* list of jumps out of this loop */
  int nactvar;  /* # active local variables outside the breakable structure */
  int upval;  /* true if some variable in the block is an upvalue */
  int isbreakable;  /* true if `block' is a loop */
} BlockCnt;



/*
** prototypes for recursive non-terminal functions
*/
static void chunk (LexState *ls);
static void expr (LexState *ls, expdesc *v);



static void next (LexState *ls) {
  ls->lastline = ls->linenumber;
  if (ls->lookahead.token != TK_EOS) {  /* is there a look-ahead token? */
    ls->t = ls->lookahead;  /* use this one */
    ls->lookahead.token = TK_EOS;  /* and discharge it */
  }
  else
    ls->t.token = luaX_lex(ls, &ls->t.seminfo);  /* read next token */
}


static void lookahead (LexState *ls) {
  lua_assert(ls->lookahead.token == TK_EOS);
  ls->lookahead.token = luaX_lex(ls, &ls->lookahead.seminfo);
}


static void error_expected (LexState *ls, int token) {
  luaX_syntaxerror(ls,
         luaO_pushfstring(ls->L, "`%s' expected", luaX_token2str(ls, token)));
}


static int testnext (LexState *ls, int c) {
  if (ls->t.token == c) {
    next(ls);
    return 1;
  }
  else return 0;
}


static void check (LexState *ls, int c) {
  if (!testnext(ls, c))
    error_expected(ls, c);
}


#define check_condition(ls,c,msg)	{ if (!(c)) luaX_syntaxerror(ls, msg); }



static void check_match (LexState *ls, int what, int who, int where) {
  if (!testnext(ls, what)) {
    if (where == ls->linenumber)
      error_expected(ls, what);
    else {
      luaX_syntaxerror(ls, luaO_pushfstring(ls->L,
             "`%s' expected (to close `%s' at line %d)",
              luaX_token2str(ls, what), luaX_token2str(ls, who), where));
    }
  }
}


static TString *str_checkname (LexState *ls) {
  TString *ts;
  check_condition(ls, (ls->t.token == TK_NAME), "<name> expected");
  ts = ls->t.seminfo.ts;
  next(ls);
  return ts;
}


static void init_exp (expdesc *e, expkind k, int i) {
  e->f = e->t = NO_JUMP;
  e->k = k;
  e->info = i;
}


static void codestring (LexState *ls, expdesc *e, TString *s) {
  init_exp(e, VK, luaK_stringK(ls->fs, s));
}


static void checkname(LexState *ls, expdesc *e) {
  codestring(ls, e, str_checkname(ls));
}


static int luaI_registerlocalvar (LexState *ls, TString *varname) {
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  luaM_growvector(ls->L, f->locvars, fs->nlocvars, f->sizelocvars,
                  LocVar, MAX_INT, "");
  f->locvars[fs->nlocvars].varname = varname;
  return fs->nlocvars++;
}


static void new_localvar (LexState *ls, TString *name, int n) {
  FuncState *fs = ls->fs;
  luaX_checklimit(ls, fs->nactvar+n+1, MAXVARS, "local variables");
  fs->actvar[fs->nactvar+n] = luaI_registerlocalvar(ls, name);
}


static void adjustlocalvars (LexState *ls, int nvars) {
  FuncState *fs = ls->fs;
  fs->nactvar += nvars;
  for (; nvars; nvars--) {
    getlocvar(fs, fs->nactvar - nvars).startpc = fs->pc;
  }
}


static void removevars (LexState *ls, int tolevel) {
  FuncState *fs = ls->fs;
  while (fs->nactvar > tolevel)
    getlocvar(fs, --fs->nactvar).endpc = fs->pc;
}


static void new_localvarstr (LexState *ls, const char *name, int n) {
  new_localvar(ls, luaS_new(ls->L, name), n);
}


static void create_local (LexState *ls, const char *name) {
  new_localvarstr(ls, name, 0);
  adjustlocalvars(ls, 1);
}


static int indexupvalue (FuncState *fs, expdesc *v) {
  int i;
  for (i=0; i<fs->f->nupvalues; i++) {
    if (fs->upvalues[i].k == v->k && fs->upvalues[i].info == v->info)
      return i;
  }
  /* new one */
  luaX_checklimit(fs->ls, fs->f->nupvalues+1, MAXUPVALUES, "upvalues");
  fs->upvalues[fs->f->nupvalues] = *v;
  return fs->f->nupvalues++;
}


static int searchvar (FuncState *fs, TString *n) {
  int i;
  for (i=fs->nactvar-1; i >= 0; i--) {
    if (n == getlocvar(fs, i).varname)
      return i;
  }
  return -1;  /* not found */
}


static void markupval (FuncState *fs, int level) {
  BlockCnt *bl = fs->bl;
  while (bl && bl->nactvar > level) bl = bl->previous;
  if (bl) bl->upval = 1;
}


static void singlevaraux (FuncState *fs, TString *n, expdesc *var, int base) {
  if (fs == NULL)  /* no more levels? */
    init_exp(var, VGLOBAL, NO_REG);  /* default is global variable */
  else {
    int v = searchvar(fs, n);  /* look up at current level */
    if (v >= 0) {
      init_exp(var, VLOCAL, v);
      if (!base)
        markupval(fs, v);  /* local will be used as an upval */
    }
    else {  /* not found at current level; try upper one */
      singlevaraux(fs->prev, n, var, 0);
      if (var->k == VGLOBAL) {
        if (base)
          var->info = luaK_stringK(fs, n);  /* info points to global name */
      }
      else {  /* LOCAL or UPVAL */
        var->info = indexupvalue(fs, var);
        var->k = VUPVAL;  /* upvalue in this level */
      }
    }
  }
}


static void singlevar (LexState *ls, expdesc *var, int base) {
  singlevaraux(ls->fs, str_checkname(ls), var, base);
}


static void adjust_assign (LexState *ls, int nvars, int nexps, expdesc *e) {
  FuncState *fs = ls->fs;
  int extra = nvars - nexps;
  if (e->k == VCALL) {
    extra++;  /* includes call itself */
    if (extra <= 0) extra = 0;
    else luaK_reserveregs(fs, extra-1);
    luaK_setcallreturns(fs, e, extra);  /* call provides the difference */
  }
  else {
    if (e->k != VVOID) luaK_exp2nextreg(fs, e);  /* close last expression */
    if (extra > 0) {
      int reg = fs->freereg;
      luaK_reserveregs(fs, extra);
      luaK_nil(fs, reg, extra);
    }
  }
}


static void code_params (LexState *ls, int nparams, int dots) {
  FuncState *fs = ls->fs;
  adjustlocalvars(ls, nparams);
  luaX_checklimit(ls, fs->nactvar, MAXPARAMS, "parameters");
  fs->f->numparams = cast(lu_byte, fs->nactvar);
  fs->f->is_vararg = cast(lu_byte, dots);
  if (dots)
    create_local(ls, "arg");
  luaK_reserveregs(fs, fs->nactvar);  /* reserve register for parameters */
}


static void enterblock (FuncState *fs, BlockCnt *bl, int isbreakable) {
  bl->breaklist = NO_JUMP;
  bl->isbreakable = isbreakable;
  bl->nactvar = fs->nactvar;
  bl->upval = 0;
  bl->previous = fs->bl;
  fs->bl = bl;
  lua_assert(fs->freereg == fs->nactvar);
}


static void leaveblock (FuncState *fs) {
  BlockCnt *bl = fs->bl;
  fs->bl = bl->previous;
  removevars(fs->ls, bl->nactvar);
  if (bl->upval)
    luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0);
  lua_assert(bl->nactvar == fs->nactvar);
  fs->freereg = fs->nactvar;  /* free registers */
  luaK_patchtohere(fs, bl->breaklist);
}


static void pushclosure (LexState *ls, FuncState *func, expdesc *v) {
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  int i;
  luaM_growvector(ls->L, f->p, fs->np, f->sizep, Proto *,
                  MAXARG_Bx, "constant table overflow");
  f->p[fs->np++] = func->f;
  init_exp(v, VRELOCABLE, luaK_codeABx(fs, OP_CLOSURE, 0, fs->np-1));
  for (i=0; i<func->f->nupvalues; i++) {
    OpCode o = (func->upvalues[i].k == VLOCAL) ? OP_MOVE : OP_GETUPVAL;
    luaK_codeABC(fs, o, 0, func->upvalues[i].info, 0);
  }
}


static void open_func (LexState *ls, FuncState *fs) {
  Proto *f = luaF_newproto(ls->L);
  fs->f = f;
  fs->prev = ls->fs;  /* linked list of funcstates */
  fs->ls = ls;
  fs->L = ls->L;
  ls->fs = fs;
  fs->pc = 0;
  fs->lasttarget = 0;
  fs->jpc = NO_JUMP;
  fs->freereg = 0;
  fs->nk = 0;
  fs->h = luaH_new(ls->L, 0, 0);
  fs->np = 0;
  fs->nlocvars = 0;
  fs->nactvar = 0;
  fs->bl = NULL;
  f->code = NULL;
  f->source = ls->source;
  f->maxstacksize = 2;  /* registers 0/1 are always valid */
  f->numparams = 0;  /* default for main chunk */
  f->is_vararg = 0;  /* default for main chunk */
}


static void close_func (LexState *ls) {
  lua_State *L = ls->L;
  FuncState *fs = ls->fs;
  Proto *f = fs->f;
  removevars(ls, 0);
  luaK_codeABC(fs, OP_RETURN, 0, 1, 0);  /* final return */
  luaM_reallocvector(L, f->code, f->sizecode, fs->pc, Instruction);
  f->sizecode = fs->pc;
  luaM_reallocvector(L, f->lineinfo, f->sizelineinfo, fs->pc, int);
  f->sizelineinfo = fs->pc;
  luaM_reallocvector(L, f->k, f->sizek, fs->nk, TObject);
  f->sizek = fs->nk;
  luaM_reallocvector(L, f->p, f->sizep, fs->np, Proto *);
  f->sizep = fs->np;
  luaM_reallocvector(L, f->locvars, f->sizelocvars, fs->nlocvars, LocVar);
  f->sizelocvars = fs->nlocvars;
  lua_assert(luaG_checkcode(f));
  lua_assert(fs->bl == NULL);
  ls->fs = fs->prev;
}


Proto *luaY_parser (lua_State *L, ZIO *z, Mbuffer *buff) {
  struct LexState lexstate;
  struct FuncState funcstate;
  lexstate.buff = buff;
  lexstate.nestlevel = 0;
  luaX_setinput(L, &lexstate, z, luaS_new(L, zname(z)));
  open_func(&lexstate, &funcstate);
  next(&lexstate);  /* read first token */
  chunk(&lexstate);
  check_condition(&lexstate, (lexstate.t.token == TK_EOS), "<eof> expected");
  close_func(&lexstate);
  lua_assert(funcstate.prev == NULL);
  lua_assert(funcstate.f->nupvalues == 0);
  lua_assert(lexstate.nestlevel == 0);
  return funcstate.f;
}



/*============================================================*/
/* GRAMMAR RULES */
/*============================================================*/


static void luaY_field (LexState *ls, expdesc *v) {
  /* field -> ['.' | ':'] NAME */
  FuncState *fs = ls->fs;
  expdesc key;
  luaK_exp2anyreg(fs, v);
  next(ls);  /* skip the dot or colon */
  checkname(ls, &key);
  luaK_indexed(fs, v, &key);
}


static void luaY_index (LexState *ls, expdesc *v) {
  /* index -> '[' expr ']' */
  next(ls);  /* skip the '[' */
  expr(ls, v);
  luaK_exp2val(ls->fs, v);
  check(ls, ']');
}


/*
** {======================================================================
** Rules for Constructors
** =======================================================================
*/


struct ConsControl {
  expdesc v;  /* last list item read */
  expdesc *t;  /* table descriptor */
  int nh;  /* total number of `record' elements */
  int na;  /* total number of array elements */
  int tostore;  /* number of array elements pending to be stored */
};


static void recfield (LexState *ls, struct ConsControl *cc) {
  /* recfield -> (NAME | `['exp1`]') = exp1 */
  FuncState *fs = ls->fs;
  int reg = ls->fs->freereg;
  expdesc key, val;
  if (ls->t.token == TK_NAME) {
    luaX_checklimit(ls, cc->nh, MAX_INT, "items in a constructor");
    cc->nh++;
    checkname(ls, &key);
  }
  else  /* ls->t.token == '[' */
    luaY_index(ls, &key);
  check(ls, '=');
  luaK_exp2RK(fs, &key);
  expr(ls, &val);
  luaK_codeABC(fs, OP_SETTABLE, cc->t->info, luaK_exp2RK(fs, &key),
                                             luaK_exp2RK(fs, &val));
  fs->freereg = reg;  /* free registers */
}


static void closelistfield (FuncState *fs, struct ConsControl *cc) {
  if (cc->v.k == VVOID) return;  /* there is no list item */
  luaK_exp2nextreg(fs, &cc->v);
  cc->v.k = VVOID;
  if (cc->tostore == LFIELDS_PER_FLUSH) {
    luaK_codeABx(fs, OP_SETLIST, cc->t->info, cc->na-1);  /* flush */
    cc->tostore = 0;  /* no more items pending */
    fs->freereg = cc->t->info + 1;  /* free registers */
  }
}


static void lastlistfield (FuncState *fs, struct ConsControl *cc) {
  if (cc->tostore == 0) return;
  if (cc->v.k == VCALL) {
    luaK_setcallreturns(fs, &cc->v, LUA_MULTRET);
    luaK_codeABx(fs, OP_SETLISTO, cc->t->info, cc->na-1);
  }
  else {
    if (cc->v.k != VVOID)
      luaK_exp2nextreg(fs, &cc->v);
    luaK_codeABx(fs, OP_SETLIST, cc->t->info, cc->na-1);
  }
  fs->freereg = cc->t->info + 1;  /* free registers */
}


static void listfield (LexState *ls, struct ConsControl *cc) {
  expr(ls, &cc->v);
  luaX_checklimit(ls, cc->na, MAXARG_Bx, "items in a constructor");
  cc->na++;
  cc->tostore++;
}


static void constructor (LexState *ls, expdesc *t) {
  /* constructor -> ?? */
  FuncState *fs = ls->fs;
  int line = ls->linenumber;
  int pc = luaK_codeABC(fs, OP_NEWTABLE, 0, 0, 0);
  struct ConsControl cc;
  cc.na = cc.nh = cc.tostore = 0;
  cc.t = t;
  init_exp(t, VRELOCABLE, pc);
  init_exp(&cc.v, VVOID, 0);  /* no value (yet) */
  luaK_exp2nextreg(ls->fs, t);  /* fix it at stack top (for gc) */
  check(ls, '{');
  do {
    lua_assert(cc.v.k == VVOID || cc.tostore > 0);
    testnext(ls, ';');  /* compatibility only */
    if (ls->t.token == '}') break;
    closelistfield(fs, &cc);
    switch(ls->t.token) {
      case TK_NAME: {  /* may be listfields or recfields */
        lookahead(ls);
        if (ls->lookahead.token != '=')  /* expression? */
          listfield(ls, &cc);
        else
          recfield(ls, &cc);
        break;
      }
      case '[': {  /* constructor_item -> recfield */
        recfield(ls, &cc);
        break;
      }
      default: {  /* constructor_part -> listfield */
        listfield(ls, &cc);
        break;
      }
    }
  } while (testnext(ls, ',') || testnext(ls, ';'));
  check_match(ls, '}', '{', line);
  lastlistfield(fs, &cc);
  if (cc.na > 0)
    SETARG_B(fs->f->code[pc], luaO_log2(cc.na-1)+2); /* set initial table size */
  SETARG_C(fs->f->code[pc], luaO_log2(cc.nh)+1);  /* set initial table size */
}

/* }====================================================================== */



static void parlist (LexState *ls) {
  /* parlist -> [ param { `,' param } ] */
  int nparams = 0;
  int dots = 0;
  if (ls->t.token != ')') {  /* is `parlist' not empty? */
    do {
      switch (ls->t.token) {
        case TK_DOTS: dots = 1; next(ls); break;
        case TK_NAME: new_localvar(ls, str_checkname(ls), nparams++); break;
        default: luaX_syntaxerror(ls, "<name> or `...' expected");
      }
    } while (!dots && testnext(ls, ','));
  }
  code_params(ls, nparams, dots);
}


static void body (LexState *ls, expdesc *e, int needself, int line) {
  /* body ->  `(' parlist `)' chunk END */
  FuncState new_fs;
  open_func(ls, &new_fs);
  new_fs.f->lineDefined = line;
  check(ls, '(');
  if (needself)
    create_local(ls, "self");
  parlist(ls);
  check(ls, ')');
  chunk(ls);
  check_match(ls, TK_END, TK_FUNCTION, line);
  close_func(ls);
  pushclosure(ls, &new_fs, e);
}


static int explist1 (LexState *ls, expdesc *v) {
  /* explist1 -> expr { `,' expr } */
  int n = 1;  /* at least one expression */
  expr(ls, v);
  while (testnext(ls, ',')) {
    luaK_exp2nextreg(ls->fs, v);
    expr(ls, v);
    n++;
  }
  return n;
}


static void funcargs (LexState *ls, expdesc *f) {
  FuncState *fs = ls->fs;
  expdesc args;
  int base, nparams;
  int line = ls->linenumber;
  switch (ls->t.token) {
    case '(': {  /* funcargs -> `(' [ explist1 ] `)' */
      if (line != ls->lastline)
        luaX_syntaxerror(ls,"ambiguous syntax (function call x new statement)");
      next(ls);
      if (ls->t.token == ')')  /* arg list is empty? */
        args.k = VVOID;
      else {
        explist1(ls, &args);
        luaK_setcallreturns(fs, &args, LUA_MULTRET);
      }
      check_match(ls, ')', '(', line);
      break;
    }
    case '{': {  /* funcargs -> constructor */
      constructor(ls, &args);
      break;
    }
    case TK_STRING: {  /* funcargs -> STRING */
      codestring(ls, &args, ls->t.seminfo.ts);
      next(ls);  /* must use `seminfo' before `next' */
      break;
    }
    default: {
      luaX_syntaxerror(ls, "function arguments expected");
      return;
    }
  }
  lua_assert(f->k == VNONRELOC);
  base = f->info;  /* base register for call */
  if (args.k == VCALL)
    nparams = LUA_MULTRET;  /* open call */
  else {
    if (args.k != VVOID)
      luaK_exp2nextreg(fs, &args);  /* close last argument */
    nparams = fs->freereg - (base+1);
  }
  init_exp(f, VCALL, luaK_codeABC(fs, OP_CALL, base, nparams+1, 2));
  luaK_fixline(fs, line);
  fs->freereg = base+1;  /* call remove function and arguments and leaves
                            (unless changed) one result */
}




/*
** {======================================================================
** Expression parsing
** =======================================================================
*/


static void prefixexp (LexState *ls, expdesc *v) {
  /* prefixexp -> NAME | '(' expr ')' */
  switch (ls->t.token) {
    case '(': {
      int line = ls->linenumber;
      next(ls);
      expr(ls, v);
      check_match(ls, ')', '(', line);
      luaK_dischargevars(ls->fs, v);
      return;
    }
    case TK_NAME: {
      singlevar(ls, v, 1);
      return;
    }
    case '%': {  /* for compatibility only */
      next(ls);  /* skip `%' */
      singlevar(ls, v, 1);
      check_condition(ls, v->k == VUPVAL, "global upvalues are obsolete");
      return;
    }
    default: {
      luaX_syntaxerror(ls, "unexpected symbol");
      return;
    }
  }
}


static void primaryexp (LexState *ls, expdesc *v) {
  /* primaryexp ->
        prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | funcargs } */
  FuncState *fs = ls->fs;
  prefixexp(ls, v);
  for (;;) {
    switch (ls->t.token) {
      case '.': {  /* field */
        luaY_field(ls, v);
        break;
      }
      case '[': {  /* `[' exp1 `]' */
        expdesc key;
        luaK_exp2anyreg(fs, v);
        luaY_index(ls, &key);
        luaK_indexed(fs, v, &key);
        break;
      }
      case ':': {  /* `:' NAME funcargs */
        expdesc key;
        next(ls);
        checkname(ls, &key);
        luaK_self(fs, v, &key);
        funcargs(ls, v);
        break;
      }
      case '(': case TK_STRING: case '{': {  /* funcargs */
        luaK_exp2nextreg(fs, v);
        funcargs(ls, v);
        break;
      }
      default: return;
    }
  }
}


static void simpleexp (LexState *ls, expdesc *v) {
  /* simpleexp -> NUMBER | STRING | NIL | constructor | FUNCTION body
               | primaryexp */
  switch (ls->t.token) {
    case TK_NUMBER: {
      init_exp(v, VK, luaK_numberK(ls->fs, ls->t.seminfo.r));
      next(ls);  /* must use `seminfo' before `next' */
      break;
    }
    case TK_STRING: {
      codestring(ls, v, ls->t.seminfo.ts);
      next(ls);  /* must use `seminfo' before `next' */
      break;
    }
    case TK_NIL: {
      init_exp(v, VNIL, 0);
      next(ls);
      break;
    }
    case TK_TRUE: {
      init_exp(v, VTRUE, 0);
      next(ls);
      break;
    }
    case TK_FALSE: {
      init_exp(v, VFALSE, 0);
      next(ls);
      break;
    }
    case '{': {  /* constructor */
      constructor(ls, v);
      break;
    }
    case TK_FUNCTION: {
      next(ls);
      body(ls, v, 0, ls->linenumber);
      break;
    }
    default: {
      primaryexp(ls, v);
      break;
    }
  }
}


static UnOpr getunopr (int op) {
  switch (op) {
    case TK_NOT: return OPR_NOT;
    case '-': return OPR_MINUS;
    default: return OPR_NOUNOPR;
  }
}


static BinOpr getbinopr (int op) {
  switch (op) {
    case '+': return OPR_ADD;
    case '-': return OPR_SUB;
    case '*': return OPR_MULT;
    case '/': return OPR_DIV;
    case '^': return OPR_POW;
    case TK_CONCAT: return OPR_CONCAT;
    case TK_NE: return OPR_NE;
    case TK_EQ: return OPR_EQ;
    case '<': return OPR_LT;
    case TK_LE: return OPR_LE;
    case '>': return OPR_GT;
    case TK_GE: return OPR_GE;
    case TK_AND: return OPR_AND;
    case TK_OR: return OPR_OR;
    default: return OPR_NOBINOPR;
  }
}


static const struct {
  lu_byte left;  /* left priority for each binary operator */
  lu_byte right; /* right priority */
} priority[] = {  /* ORDER OPR */
   {6, 6}, {6, 6}, {7, 7}, {7, 7},  /* arithmetic */
   {10, 9}, {5, 4},                 /* power and concat (right associative) */
   {3, 3}, {3, 3},                  /* equality */
   {3, 3}, {3, 3}, {3, 3}, {3, 3},  /* order */
   {2, 2}, {1, 1}                   /* logical (and/or) */
};

#define UNARY_PRIORITY	8  /* priority for unary operators */


/*
** subexpr -> (simplexep | unop subexpr) { binop subexpr }
** where `binop' is any binary operator with a priority higher than `limit'
*/
static BinOpr subexpr (LexState *ls, expdesc *v, int limit) {
  BinOpr op;
  UnOpr uop;
  enterlevel(ls);
  uop = getunopr(ls->t.token);
  if (uop != OPR_NOUNOPR) {
    next(ls);
    subexpr(ls, v, UNARY_PRIORITY);
    luaK_prefix(ls->fs, uop, v);
  }
  else simpleexp(ls, v);
  /* expand while operators have priorities higher than `limit' */
  op = getbinopr(ls->t.token);
  while (op != OPR_NOBINOPR && cast(int, priority[op].left) > limit) {
    expdesc v2;
    BinOpr nextop;
    next(ls);
    luaK_infix(ls->fs, op, v);
    /* read sub-expression with higher priority */
    nextop = subexpr(ls, &v2, cast(int, priority[op].right));
    luaK_posfix(ls->fs, op, v, &v2);
    op = nextop;
  }
  leavelevel(ls);
  return op;  /* return first untreated operator */
}


static void expr (LexState *ls, expdesc *v) {
  subexpr(ls, v, -1);
}

/* }==================================================================== */



/*
** {======================================================================
** Rules for Statements
** =======================================================================
*/


static int block_follow (int token) {
  switch (token) {
    case TK_ELSE: case TK_ELSEIF: case TK_END:
    case TK_UNTIL: case TK_EOS:
      return 1;
    default: return 0;
  }
}


static void block (LexState *ls) {
  /* block -> chunk */
  FuncState *fs = ls->fs;
  BlockCnt bl;
  enterblock(fs, &bl, 0);
  chunk(ls);
  lua_assert(bl.breaklist == NO_JUMP);
  leaveblock(fs);
}


/*
** structure to chain all variables in the left-hand side of an
** assignment
*/
struct LHS_assign {
  struct LHS_assign *prev;
  expdesc v;  /* variable (global, local, upvalue, or indexed) */
};


/*
** check whether, in an assignment to a local variable, the local variable
** is needed in a previous assignment (to a table). If so, save original
** local value in a safe place and use this safe copy in the previous
** assignment.
*/
static void check_conflict (LexState *ls, struct LHS_assign *lh, expdesc *v) {
  FuncState *fs = ls->fs;
  int extra = fs->freereg;  /* eventual position to save local variable */
  int conflict = 0;
  for (; lh; lh = lh->prev) {
    if (lh->v.k == VINDEXED) {
      if (lh->v.info == v->info) {  /* conflict? */
        conflict = 1;
        lh->v.info = extra;  /* previous assignment will use safe copy */
      }
      if (lh->v.aux == v->info) {  /* conflict? */
        conflict = 1;
        lh->v.aux = extra;  /* previous assignment will use safe copy */
      }
    }
  }
  if (conflict) {
    luaK_codeABC(fs, OP_MOVE, fs->freereg, v->info, 0);  /* make copy */
    luaK_reserveregs(fs, 1);
  }
}


static void assignment (LexState *ls, struct LHS_assign *lh, int nvars) {
  expdesc e;
  check_condition(ls, VLOCAL <= lh->v.k && lh->v.k <= VINDEXED,
                      "syntax error");
  if (testnext(ls, ',')) {  /* assignment -> `,' primaryexp assignment */
    struct LHS_assign nv;
    nv.prev = lh;
    primaryexp(ls, &nv.v);
    if (nv.v.k == VLOCAL)
      check_conflict(ls, lh, &nv.v);
    assignment(ls, &nv, nvars+1);
  }
  else {  /* assignment -> `=' explist1 */
    int nexps;
    check(ls, '=');
    nexps = explist1(ls, &e);
    if (nexps != nvars) {
      adjust_assign(ls, nvars, nexps, &e);
      if (nexps > nvars)
        ls->fs->freereg -= nexps - nvars;  /* remove extra values */
    }
    else {
      luaK_setcallreturns(ls->fs, &e, 1);  /* close last expression */
      luaK_storevar(ls->fs, &lh->v, &e);
      return;  /* avoid default */
    }
  }
  init_exp(&e, VNONRELOC, ls->fs->freereg-1);  /* default assignment */
  luaK_storevar(ls->fs, &lh->v, &e);
}


static void cond (LexState *ls, expdesc *v) {
  /* cond -> exp */
  expr(ls, v);  /* read condition */
  if (v->k == VNIL) v->k = VFALSE;  /* `falses' are all equal here */
  luaK_goiftrue(ls->fs, v);
  luaK_patchtohere(ls->fs, v->t);
}


/*
** The while statement optimizes its code by coding the condition
** after its body (and thus avoiding one jump in the loop).
*/

/*
** maximum size of expressions for optimizing `while' code
*/
#ifndef MAXEXPWHILE
#define MAXEXPWHILE	100
#endif

/*
** the call `luaK_goiffalse' may grow the size of an expression by
** at most this:
*/
#define EXTRAEXP	5

static void whilestat (LexState *ls, int line) {
  /* whilestat -> WHILE cond DO block END */
  Instruction codeexp[MAXEXPWHILE + EXTRAEXP];
  int lineexp = 0;
  int i;
  int sizeexp;
  FuncState *fs = ls->fs;
  int whileinit, blockinit, expinit;
  expdesc v;
  BlockCnt bl;
  next(ls);  /* skip WHILE */
  whileinit = luaK_jump(fs);  /* jump to condition (which will be moved) */
  expinit = luaK_getlabel(fs);
  expr(ls, &v);  /* parse condition */
  if (v.k == VK) v.k = VTRUE;  /* `trues' are all equal here */
  lineexp = ls->linenumber;
  luaK_goiffalse(fs, &v);
  luaK_concat(fs, &v.f, fs->jpc);
  fs->jpc = NO_JUMP;
  sizeexp = fs->pc - expinit;  /* size of expression code */
  if (sizeexp > MAXEXPWHILE) 
    luaX_syntaxerror(ls, "`while' condition too complex");
  for (i = 0; i < sizeexp; i++)  /* save `exp' code */
    codeexp[i] = fs->f->code[expinit + i];
  fs->pc = expinit;  /* remove `exp' code */
  enterblock(fs, &bl, 1);
  check(ls, TK_DO);
  blockinit = luaK_getlabel(fs);
  block(ls);
  luaK_patchtohere(fs, whileinit);  /* initial jump jumps to here */
  /* move `exp' back to code */
  if (v.t != NO_JUMP) v.t += fs->pc - expinit;
  if (v.f != NO_JUMP) v.f += fs->pc - expinit;
  for (i=0; i<sizeexp; i++)
    luaK_code(fs, codeexp[i], lineexp);
  check_match(ls, TK_END, TK_WHILE, line);
  leaveblock(fs);
  luaK_patchlist(fs, v.t, blockinit);  /* true conditions go back to loop */
  luaK_patchtohere(fs, v.f);  /* false conditions finish the loop */
}


static void repeatstat (LexState *ls, int line) {
  /* repeatstat -> REPEAT block UNTIL cond */
  FuncState *fs = ls->fs;
  int repeat_init = luaK_getlabel(fs);
  expdesc v;
  BlockCnt bl;
  enterblock(fs, &bl, 1);
  next(ls);
  block(ls);
  check_match(ls, TK_UNTIL, TK_REPEAT, line);
  cond(ls, &v);
  luaK_patchlist(fs, v.f, repeat_init);
  leaveblock(fs);
}


static int exp1 (LexState *ls) {
  expdesc e;
  int k;
  expr(ls, &e);
  k = e.k;
  luaK_exp2nextreg(ls->fs, &e);
  return k;
}


static void forbody (LexState *ls, int base, int line, int nvars, int isnum) {
  BlockCnt bl;
  FuncState *fs = ls->fs;
  int prep, endfor;
  adjustlocalvars(ls, nvars);  /* scope for all variables */
  check(ls, TK_DO);
  enterblock(fs, &bl, 1);  /* loop block */
  prep = luaK_getlabel(fs);
  block(ls);
  luaK_patchtohere(fs, prep-1);
  endfor = (isnum) ? luaK_codeAsBx(fs, OP_FORLOOP, base, NO_JUMP) :
                     luaK_codeABC(fs, OP_TFORLOOP, base, 0, nvars - 3);
  luaK_fixline(fs, line);  /* pretend that `OP_FOR' starts the loop */
  luaK_patchlist(fs, (isnum) ? endfor : luaK_jump(fs), prep);
  leaveblock(fs);
}


static void fornum (LexState *ls, TString *varname, int line) {
  /* fornum -> NAME = exp1,exp1[,exp1] DO body */
  FuncState *fs = ls->fs;
  int base = fs->freereg;
  new_localvar(ls, varname, 0);
  new_localvarstr(ls, "(for limit)", 1);
  new_localvarstr(ls, "(for step)", 2);
  check(ls, '=');
  exp1(ls);  /* initial value */
  check(ls, ',');
  exp1(ls);  /* limit */
  if (testnext(ls, ','))
    exp1(ls);  /* optional step */
  else {  /* default step = 1 */
    luaK_codeABx(fs, OP_LOADK, fs->freereg, luaK_numberK(fs, 1));
    luaK_reserveregs(fs, 1);
  }
  luaK_codeABC(fs, OP_SUB, fs->freereg - 3, fs->freereg - 3, fs->freereg - 1);
  luaK_jump(fs);
  forbody(ls, base, line, 3, 1);
}


static void forlist (LexState *ls, TString *indexname) {
  /* forlist -> NAME {,NAME} IN explist1 DO body */
  FuncState *fs = ls->fs;
  expdesc e;
  int nvars = 0;
  int line;
  int base = fs->freereg;
  new_localvarstr(ls, "(for generator)", nvars++);
  new_localvarstr(ls, "(for state)", nvars++);
  new_localvar(ls, indexname, nvars++);
  while (testnext(ls, ','))
    new_localvar(ls, str_checkname(ls), nvars++);
  check(ls, TK_IN);
  line = ls->linenumber;
  adjust_assign(ls, nvars, explist1(ls, &e), &e);
  luaK_checkstack(fs, 3);  /* extra space to call generator */
  luaK_codeAsBx(fs, OP_TFORPREP, base, NO_JUMP);
  forbody(ls, base, line, nvars, 0);
}


static void forstat (LexState *ls, int line) {
  /* forstat -> fornum | forlist */
  FuncState *fs = ls->fs;
  TString *varname;
  BlockCnt bl;
  enterblock(fs, &bl, 0);  /* block to control variable scope */
  next(ls);  /* skip `for' */
  varname = str_checkname(ls);  /* first variable name */
  switch (ls->t.token) {
    case '=': fornum(ls, varname, line); break;
    case ',': case TK_IN: forlist(ls, varname); break;
    default: luaX_syntaxerror(ls, "`=' or `in' expected");
  }
  check_match(ls, TK_END, TK_FOR, line);
  leaveblock(fs);
}


static void test_then_block (LexState *ls, expdesc *v) {
  /* test_then_block -> [IF | ELSEIF] cond THEN block */
  next(ls);  /* skip IF or ELSEIF */
  cond(ls, v);
  check(ls, TK_THEN);
  block(ls);  /* `then' part */
}


static void ifstat (LexState *ls, int line) {
  /* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */
  FuncState *fs = ls->fs;
  expdesc v;
  int escapelist = NO_JUMP;
  test_then_block(ls, &v);  /* IF cond THEN block */
  while (ls->t.token == TK_ELSEIF) {
    luaK_concat(fs, &escapelist, luaK_jump(fs));
    luaK_patchtohere(fs, v.f);
    test_then_block(ls, &v);  /* ELSEIF cond THEN block */
  }
  if (ls->t.token == TK_ELSE) {
    luaK_concat(fs, &escapelist, luaK_jump(fs));
    luaK_patchtohere(fs, v.f);
    next(ls);  /* skip ELSE (after patch, for correct line info) */
    block(ls);  /* `else' part */
  }
  else
    luaK_concat(fs, &escapelist, v.f);
  luaK_patchtohere(fs, escapelist);
  check_match(ls, TK_END, TK_IF, line);
}


static void localfunc (LexState *ls) {
  expdesc v, b;
  new_localvar(ls, str_checkname(ls), 0);
  init_exp(&v, VLOCAL, ls->fs->freereg++);
  adjustlocalvars(ls, 1);
  body(ls, &b, 0, ls->linenumber);
  luaK_storevar(ls->fs, &v, &b);
}


static void localstat (LexState *ls) {
  /* stat -> LOCAL NAME {`,' NAME} [`=' explist1] */
  int nvars = 0;
  int nexps;
  expdesc e;
  do {
    new_localvar(ls, str_checkname(ls), nvars++);
  } while (testnext(ls, ','));
  if (testnext(ls, '='))
    nexps = explist1(ls, &e);
  else {
    e.k = VVOID;
    nexps = 0;
  }
  adjust_assign(ls, nvars, nexps, &e);
  adjustlocalvars(ls, nvars);
}


static int funcname (LexState *ls, expdesc *v) {
  /* funcname -> NAME {field} [`:' NAME] */
  int needself = 0;
  singlevar(ls, v, 1);
  while (ls->t.token == '.')
    luaY_field(ls, v);
  if (ls->t.token == ':') {
    needself = 1;
    luaY_field(ls, v);
  }
  return needself;
}


static void funcstat (LexState *ls, int line) {
  /* funcstat -> FUNCTION funcname body */
  int needself;
  expdesc v, b;
  next(ls);  /* skip FUNCTION */
  needself = funcname(ls, &v);
  body(ls, &b, needself, line);
  luaK_storevar(ls->fs, &v, &b);
  luaK_fixline(ls->fs, line);  /* definition `happens' in the first line */
}


static void exprstat (LexState *ls) {
  /* stat -> func | assignment */
  FuncState *fs = ls->fs;
  struct LHS_assign v;
  primaryexp(ls, &v.v);
  if (v.v.k == VCALL) {  /* stat -> func */
    luaK_setcallreturns(fs, &v.v, 0);  /* call statement uses no results */
  }
  else {  /* stat -> assignment */
    v.prev = NULL;
    assignment(ls, &v, 1);
  }
}


static void retstat (LexState *ls) {
  /* stat -> RETURN explist */
  FuncState *fs = ls->fs;
  expdesc e;
  int first, nret;  /* registers with returned values */
  next(ls);  /* skip RETURN */
  if (block_follow(ls->t.token) || ls->t.token == ';')
    first = nret = 0;  /* return no values */
  else {
    nret = explist1(ls, &e);  /* optional return values */
    if (e.k == VCALL) {
      luaK_setcallreturns(fs, &e, LUA_MULTRET);
      if (nret == 1) {  /* tail call? */
        SET_OPCODE(getcode(fs,&e), OP_TAILCALL);
        lua_assert(GETARG_A(getcode(fs,&e)) == fs->nactvar);
      }
      first = fs->nactvar;
      nret = LUA_MULTRET;  /* return all values */
    }
    else {
      if (nret == 1)  /* only one single value? */
        first = luaK_exp2anyreg(fs, &e);
      else {
        luaK_exp2nextreg(fs, &e);  /* values must go to the `stack' */
        first = fs->nactvar;  /* return all `active' values */
        lua_assert(nret == fs->freereg - first);
      }
    }
  }
  luaK_codeABC(fs, OP_RETURN, first, nret+1, 0);
}


static void breakstat (LexState *ls) {
  /* stat -> BREAK [NAME] */
  FuncState *fs = ls->fs;
  BlockCnt *bl = fs->bl;
  int upval = 0;
  next(ls);  /* skip BREAK */
  while (bl && !bl->isbreakable) {
    upval |= bl->upval;
    bl = bl->previous;
  }
  if (!bl)
    luaX_syntaxerror(ls, "no loop to break");
  if (upval)
    luaK_codeABC(fs, OP_CLOSE, bl->nactvar, 0, 0);
  luaK_concat(fs, &bl->breaklist, luaK_jump(fs));
}


static int statement (LexState *ls) {
  int line = ls->linenumber;  /* may be needed for error messages */
  switch (ls->t.token) {
    case TK_IF: {  /* stat -> ifstat */
      ifstat(ls, line);
      return 0;
    }
    case TK_WHILE: {  /* stat -> whilestat */
      whilestat(ls, line);
      return 0;
    }
    case TK_DO: {  /* stat -> DO block END */
      next(ls);  /* skip DO */
      block(ls);
      check_match(ls, TK_END, TK_DO, line);
      return 0;
    }
    case TK_FOR: {  /* stat -> forstat */
      forstat(ls, line);
      return 0;
    }
    case TK_REPEAT: {  /* stat -> repeatstat */
      repeatstat(ls, line);
      return 0;
    }
    case TK_FUNCTION: {
      funcstat(ls, line);  /* stat -> funcstat */
      return 0;
    }
    case TK_LOCAL: {  /* stat -> localstat */
      next(ls);  /* skip LOCAL */
      if (testnext(ls, TK_FUNCTION))  /* local function? */
        localfunc(ls);
      else
        localstat(ls);
      return 0;
    }
    case TK_RETURN: {  /* stat -> retstat */
      retstat(ls);
      return 1;  /* must be last statement */
    }
    case TK_BREAK: {  /* stat -> breakstat */
      breakstat(ls);
      return 1;  /* must be last statement */
    }
    default: {
      exprstat(ls);
      return 0;  /* to avoid warnings */
    }
  }
}


static void chunk (LexState *ls) {
  /* chunk -> { stat [`;'] } */
  int islast = 0;
  enterlevel(ls);
  while (!islast && !block_follow(ls->t.token)) {
    islast = statement(ls);
    testnext(ls, ';');
    lua_assert(ls->fs->freereg >= ls->fs->nactvar);
    ls->fs->freereg = ls->fs->nactvar;  /* free registers */
  }
  leavelevel(ls);
}

/* }====================================================================== */
