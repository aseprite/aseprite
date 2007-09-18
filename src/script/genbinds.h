/* Generated with bindings.gen */

#ifndef SCRIPT_GENBINDS_H
#define SCRIPT_GENBINDS_H

#include "script/script.h"

extern const luaL_reg bindings_routines[];

struct _bindings_constants {
  const char *name;
  double value;
};

extern struct _bindings_constants bindings_constants[];

void register_bindings (lua_State *L);

#endif /* SCRIPT_GENBINDS_H */
