/*
** $Id: llimits.h,v 1.1 2004/03/09 02:46:00 dacap Exp $
** Limits, basic types, and some other `installation-dependent' definitions
** See Copyright Notice in lua.h
*/

#ifndef llimits_h
#define llimits_h


#include <limits.h>
#include <stddef.h>


#include "lua.h"


/*
** try to find number of bits in an integer
*/
#ifndef BITS_INT
/* avoid overflows in comparison */
#if INT_MAX-20 < 32760
#define	BITS_INT	16
#else
#if INT_MAX > 2147483640L
/* machine has at least 32 bits */
#define BITS_INT	32
#else
#error "you must define BITS_INT with number of bits in an integer"
#endif
#endif
#endif


/*
** the following types define integer types for values that may not
** fit in a `small int' (16 bits), but may waste space in a
** `large long' (64 bits). The current definitions should work in
** any machine, but may not be optimal.
*/

/* an unsigned integer to hold hash values */
typedef unsigned int lu_hash;
/* its signed equivalent */
typedef int ls_hash;

/* an unsigned integer big enough to count the total memory used by Lua; */
/* it should be at least as large as size_t */
typedef unsigned long lu_mem;

/* an integer big enough to count the number of strings in use */
typedef long ls_nstr;

/* chars used as small naturals (so that `char' is reserved for characters) */
typedef unsigned char lu_byte;


#define MAX_SIZET	((size_t)(~(size_t)0)-2)


#define MAX_INT (INT_MAX-2)  /* maximum value of an int (-2 for safety) */

/*
** conversion of pointer to integer
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define IntPoint(p)  ((lu_hash)(p))



/* type to ensure maximum alignment */
#ifndef LUSER_ALIGNMENT_T
typedef union { double u; void *s; long l; } L_Umaxalign;
#else
typedef LUSER_ALIGNMENT_T L_Umaxalign;
#endif


/* result of `usual argument conversion' over lua_Number */
#ifndef LUA_UACNUMBER
typedef double l_uacNumber;
#else
typedef LUA_UACNUMBER l_uacNumber;
#endif


#ifndef lua_assert
#define lua_assert(c)		/* empty */
#endif


#ifndef check_exp
#define check_exp(c,e)	(e)
#endif


#ifndef UNUSED
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif


#ifndef cast
#define cast(t, exp)	((t)(exp))
#endif



/*
** type for virtual-machine instructions
** must be an unsigned with (at least) 4 bytes (see details in lopcodes.h)
*/
typedef unsigned long Instruction;


/* maximum depth for calls (unsigned short) */
#ifndef LUA_MAXCALLS
#define LUA_MAXCALLS        4096
#endif


/*
** maximum depth for C calls (unsigned short): Not too big, or may
** overflow the C stack...
*/

#ifndef LUA_MAXCCALLS
#define LUA_MAXCCALLS        200
#endif


/* maximum size for the C stack */
#ifndef LUA_MAXCSTACK
#define LUA_MAXCSTACK        2048
#endif


/* maximum stack for a Lua function */
#define MAXSTACK	250


/* maximum number of variables declared in a function */
#ifndef MAXVARS
#define MAXVARS 200           /* arbitrary limit (<MAXSTACK) */
#endif


/* maximum number of upvalues per function */
#ifndef MAXUPVALUES
#define MAXUPVALUES	32
#endif


/* maximum number of parameters in a function */
#ifndef MAXPARAMS
#define MAXPARAMS 100           /* arbitrary limit (<MAXLOCALS) */
#endif


/* minimum size for the string table (must be power of 2) */
#ifndef MINSTRTABSIZE
#define MINSTRTABSIZE	32
#endif


/* minimum size for string buffer */
#ifndef LUA_MINBUFFER
#define LUA_MINBUFFER	32
#endif


/*
** maximum number of syntactical nested non-terminals: Not too big,
** or may overflow the C stack...
*/
#ifndef LUA_MAXPARSERLEVEL
#define LUA_MAXPARSERLEVEL	200
#endif


#endif
