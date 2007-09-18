/*
** $Id: lundump.h,v 1.1 2004/03/09 02:46:00 dacap Exp $
** load pre-compiled Lua chunks
** See Copyright Notice in lua.h
*/

#ifndef lundump_h
#define lundump_h

#include "lobject.h"
#include "lzio.h"

/* load one chunk; from lundump.c */
Proto* luaU_undump (lua_State* L, ZIO* Z, Mbuffer* buff);

/* find byte order; from lundump.c */
int luaU_endianness (void);

/* dump one chunk; from ldump.c */
void luaU_dump (lua_State* L, const Proto* Main, lua_Chunkwriter w, void* data);

/* print one chunk; from print.c */
void luaU_print (const Proto* Main);

/* definitions for headers of binary files */
#define	LUA_SIGNATURE	"\033Lua"	/* binary files start with <esc>Lua */
#define	VERSION		0x50		/* last format change was in 5.0 */
#define	VERSION0	0x50		/* last major  change was in 5.0 */

/* a multiple of PI for testing native format */
/* multiplying by 1E8 gives non-trivial integer values */
#define	TEST_NUMBER	3.14159265358979323846E8

#endif
