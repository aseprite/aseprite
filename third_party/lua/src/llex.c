/*
** $Id: llex.c,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/


#include <ctype.h>
#include <string.h>

#define llex_c

#include "lua.h"

#include "ldo.h"
#include "llex.h"
#include "lobject.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "lzio.h"



#define next(LS) (LS->current = zgetc(LS->z))



/* ORDER RESERVED */
static const char *const token2string [] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while", "*name",
    "..", "...", "==", ">=", "<=", "~=",
    "*number", "*string", "<eof>"
};


void luaX_init (lua_State *L) {
  int i;
  for (i=0; i<NUM_RESERVED; i++) {
    TString *ts = luaS_new(L, token2string[i]);
    luaS_fix(ts);  /* reserved words are never collected */
    lua_assert(strlen(token2string[i])+1 <= TOKEN_LEN);
    ts->tsv.reserved = cast(lu_byte, i+1);  /* reserved word */
  }
}


#define MAXSRC          80


void luaX_checklimit (LexState *ls, int val, int limit, const char *msg) {
  if (val > limit) {
    msg = luaO_pushfstring(ls->L, "too many %s (limit=%d)", msg, limit);
    luaX_syntaxerror(ls, msg);
  }
}


static void luaX_error (LexState *ls, const char *s, const char *token) {
  lua_State *L = ls->L;
  char buff[MAXSRC];
  luaO_chunkid(buff, getstr(ls->source), MAXSRC);
  luaO_pushfstring(L, "%s:%d: %s near `%s'", buff, ls->linenumber, s, token); 
  luaD_throw(L, LUA_ERRSYNTAX);
}


void luaX_syntaxerror (LexState *ls, const char *msg) {
  const char *lasttoken;
  switch (ls->t.token) {
    case TK_NAME:
      lasttoken = getstr(ls->t.seminfo.ts);
      break;
    case TK_STRING:
    case TK_NUMBER:
      lasttoken = luaZ_buffer(ls->buff);
      break;
    default:
      lasttoken = luaX_token2str(ls, ls->t.token);
      break;
  }
  luaX_error(ls, msg, lasttoken);
}


const char *luaX_token2str (LexState *ls, int token) {
  if (token < FIRST_RESERVED) {
    lua_assert(token == (char)token);
    return luaO_pushfstring(ls->L, "%c", token);
  }
  else
    return token2string[token-FIRST_RESERVED];
}


static void luaX_lexerror (LexState *ls, const char *s, int token) {
  if (token == TK_EOS)
    luaX_error(ls, s, luaX_token2str(ls, token));
  else
    luaX_error(ls, s, luaZ_buffer(ls->buff));
}


static void inclinenumber (LexState *LS) {
  next(LS);  /* skip `\n' */
  ++LS->linenumber;
  luaX_checklimit(LS, LS->linenumber, MAX_INT, "lines in a chunk");
}


void luaX_setinput (lua_State *L, LexState *LS, ZIO *z, TString *source) {
  LS->L = L;
  LS->lookahead.token = TK_EOS;  /* no look-ahead token */
  LS->z = z;
  LS->fs = NULL;
  LS->linenumber = 1;
  LS->lastline = 1;
  LS->source = source;
  next(LS);  /* read first char */
  if (LS->current == '#') {
    do {  /* skip first line */
      next(LS);
    } while (LS->current != '\n' && LS->current != EOZ);
  }
}



/*
** =======================================================
** LEXICAL ANALYZER
** =======================================================
*/


/* use buffer to store names, literal strings and numbers */

/* extra space to allocate when growing buffer */
#define EXTRABUFF	32

/* maximum number of chars that can be read without checking buffer size */
#define MAXNOCHECK	5

#define checkbuffer(LS, len)	\
    if (((len)+MAXNOCHECK)*sizeof(char) > luaZ_sizebuffer((LS)->buff)) \
      luaZ_openspace((LS)->L, (LS)->buff, (len)+EXTRABUFF)

#define save(LS, c, l) \
	(luaZ_buffer((LS)->buff)[l++] = cast(char, c))
#define save_and_next(LS, l)  (save(LS, LS->current, l), next(LS))


static size_t readname (LexState *LS) {
  size_t l = 0;
  checkbuffer(LS, l);
  do {
    checkbuffer(LS, l);
    save_and_next(LS, l);
  } while (isalnum(LS->current) || LS->current == '_');
  save(LS, '\0', l);
  return l-1;
}


/* LUA_NUMBER */
static void read_numeral (LexState *LS, int comma, SemInfo *seminfo) {
  size_t l = 0;
  checkbuffer(LS, l);
  if (comma) save(LS, '.', l);
  while (isdigit(LS->current)) {
    checkbuffer(LS, l);
    save_and_next(LS, l);
  }
  if (LS->current == '.') {
    save_and_next(LS, l);
    if (LS->current == '.') {
      save_and_next(LS, l);
      save(LS, '\0', l);
      luaX_lexerror(LS,
                 "ambiguous syntax (decimal point x string concatenation)",
                 TK_NUMBER);
    }
  }
  while (isdigit(LS->current)) {
    checkbuffer(LS, l);
    save_and_next(LS, l);
  }
  if (LS->current == 'e' || LS->current == 'E') {
    save_and_next(LS, l);  /* read `E' */
    if (LS->current == '+' || LS->current == '-')
      save_and_next(LS, l);  /* optional exponent sign */
    while (isdigit(LS->current)) {
      checkbuffer(LS, l);
      save_and_next(LS, l);
    }
  }
  save(LS, '\0', l);
  if (!luaO_str2d(luaZ_buffer(LS->buff), &seminfo->r))
    luaX_lexerror(LS, "malformed number", TK_NUMBER);
}


static void read_long_string (LexState *LS, SemInfo *seminfo) {
  int cont = 0;
  size_t l = 0;
  checkbuffer(LS, l);
  save(LS, '[', l);  /* save first `[' */
  save_and_next(LS, l);  /* pass the second `[' */
  if (LS->current == '\n')  /* string starts with a newline? */
    inclinenumber(LS);  /* skip it */
  for (;;) {
    checkbuffer(LS, l);
    switch (LS->current) {
      case EOZ:
        save(LS, '\0', l);
        luaX_lexerror(LS, (seminfo) ? "unfinished long string" :
                                   "unfinished long comment", TK_EOS);
        break;  /* to avoid warnings */
      case '[':
        save_and_next(LS, l);
        if (LS->current == '[') {
          cont++;
          save_and_next(LS, l);
        }
        continue;
      case ']':
        save_and_next(LS, l);
        if (LS->current == ']') {
          if (cont == 0) goto endloop;
          cont--;
          save_and_next(LS, l);
        }
        continue;
      case '\n':
        save(LS, '\n', l);
        inclinenumber(LS);
        if (!seminfo) l = 0;  /* reset buffer to avoid wasting space */
        continue;
      default:
        save_and_next(LS, l);
    }
  } endloop:
  save_and_next(LS, l);  /* skip the second `]' */
  save(LS, '\0', l);
  if (seminfo)
    seminfo->ts = luaS_newlstr(LS->L, luaZ_buffer(LS->buff) + 2, l - 5);
}


static void read_string (LexState *LS, int del, SemInfo *seminfo) {
  size_t l = 0;
  checkbuffer(LS, l);
  save_and_next(LS, l);
  while (LS->current != del) {
    checkbuffer(LS, l);
    switch (LS->current) {
      case EOZ:
        save(LS, '\0', l);
        luaX_lexerror(LS, "unfinished string", TK_EOS);
        break;  /* to avoid warnings */
      case '\n':
        save(LS, '\0', l);
        luaX_lexerror(LS, "unfinished string", TK_STRING);
        break;  /* to avoid warnings */
      case '\\':
        next(LS);  /* do not save the `\' */
        switch (LS->current) {
          case 'a': save(LS, '\a', l); next(LS); break;
          case 'b': save(LS, '\b', l); next(LS); break;
          case 'f': save(LS, '\f', l); next(LS); break;
          case 'n': save(LS, '\n', l); next(LS); break;
          case 'r': save(LS, '\r', l); next(LS); break;
          case 't': save(LS, '\t', l); next(LS); break;
          case 'v': save(LS, '\v', l); next(LS); break;
          case '\n': save(LS, '\n', l); inclinenumber(LS); break;
          case EOZ: break;  /* will raise an error next loop */
          default: {
            if (!isdigit(LS->current))
              save_and_next(LS, l);  /* handles \\, \", \', and \? */
            else {  /* \xxx */
              int c = 0;
              int i = 0;
              do {
                c = 10*c + (LS->current-'0');
                next(LS);
              } while (++i<3 && isdigit(LS->current));
              if (c > UCHAR_MAX) {
                save(LS, '\0', l);
                luaX_lexerror(LS, "escape sequence too large", TK_STRING);
              }
              save(LS, c, l);
            }
          }
        }
        break;
      default:
        save_and_next(LS, l);
    }
  }
  save_and_next(LS, l);  /* skip delimiter */
  save(LS, '\0', l);
  seminfo->ts = luaS_newlstr(LS->L, luaZ_buffer(LS->buff) + 1, l - 3);
}


int luaX_lex (LexState *LS, SemInfo *seminfo) {
  for (;;) {
    switch (LS->current) {

      case '\n': {
        inclinenumber(LS);
        continue;
      }
      case '-': {
        next(LS);
        if (LS->current != '-') return '-';
        /* else is a comment */
        next(LS);
        if (LS->current == '[' && (next(LS), LS->current == '['))
          read_long_string(LS, NULL);  /* long comment */
        else  /* short comment */
          while (LS->current != '\n' && LS->current != EOZ)
            next(LS);
        continue;
      }
      case '[': {
        next(LS);
        if (LS->current != '[') return '[';
        else {
          read_long_string(LS, seminfo);
          return TK_STRING;
        }
      }
      case '=': {
        next(LS);
        if (LS->current != '=') return '=';
        else { next(LS); return TK_EQ; }
      }
      case '<': {
        next(LS);
        if (LS->current != '=') return '<';
        else { next(LS); return TK_LE; }
      }
      case '>': {
        next(LS);
        if (LS->current != '=') return '>';
        else { next(LS); return TK_GE; }
      }
      case '~': {
        next(LS);
        if (LS->current != '=') return '~';
        else { next(LS); return TK_NE; }
      }
      /* -XXX- PATCH -XXX- */
      case '!': {
        next(LS);
        if (LS->current != '=') return TK_NOT;
        else { next(LS); return TK_NE; }
      }
      /* -XXXXXXXXXXXXXXXXXXXXX- */
      case '"':
      case '\'': {
        read_string(LS, LS->current, seminfo);
        return TK_STRING;
      }
      case '.': {
        next(LS);
        if (LS->current == '.') {
          next(LS);
          if (LS->current == '.') {
            next(LS);
            return TK_DOTS;   /* ... */
          }
          else return TK_CONCAT;   /* .. */
        }
        else if (!isdigit(LS->current)) return '.';
        else {
          read_numeral(LS, 1, seminfo);
          return TK_NUMBER;
        }
      }
      case EOZ: {
        return TK_EOS;
      }
      default: {
        if (isspace(LS->current)) {
          next(LS);
          continue;
        }
        else if (isdigit(LS->current)) {
          read_numeral(LS, 0, seminfo);
          return TK_NUMBER;
        }
        else if (isalpha(LS->current) || LS->current == '_') {
          /* identifier or reserved word */
          size_t l = readname(LS);
          TString *ts = luaS_newlstr(LS->L, luaZ_buffer(LS->buff), l);
          if (ts->tsv.reserved > 0)  /* reserved word? */
            return ts->tsv.reserved - 1 + FIRST_RESERVED;
          seminfo->ts = ts;
          return TK_NAME;
        }
        else {
          int c = LS->current;
          if (iscntrl(c))
            luaX_error(LS, "invalid control char",
                           luaO_pushfstring(LS->L, "char(%d)", c));
          next(LS);
          return c;  /* single-char tokens (+ - / ...) */
        }
      }
    }
  }
}

#undef next
