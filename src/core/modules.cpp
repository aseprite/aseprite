/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "core/core.h"
#include "core/modules.h"
#include "effect/effect.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/rootmenu.h"

#define DEF_MODULE(name, reqs) \
  { #name, init_module_##name, exit_module_##name, (reqs), false }

typedef struct Module
{
  const char *name;
  int (*init)();
  void (*exit)();
  int reqs;
  bool installed;
} Module;

static Module module[] =
{
  /* This sorting is very important because last modules depend of
     first ones.  */

  DEF_MODULE(palette,		0),
  DEF_MODULE(effect,		0),
  DEF_MODULE(graphics,		REQUIRE_INTERFACE),
  DEF_MODULE(gui,		REQUIRE_INTERFACE),
  DEF_MODULE(rootmenu,		REQUIRE_INTERFACE),
  DEF_MODULE(editors,		REQUIRE_INTERFACE),
};

static int modules = sizeof(module) / sizeof(Module);

LegacyModules::LegacyModules(int requirements)
{
  for (int c=0; c<modules; c++)
    if ((module[c].reqs & requirements) == module[c].reqs) {
      PRINTF("Installing module: %s\n", module[c].name);

      if ((*module[c].init)() < 0)
	throw ase_exception("Error initializing module: %s",
			    static_cast<const char*>(module[c].name));

      module[c].installed = true;
    }
}

LegacyModules::~LegacyModules()
{
  for (int c=modules-1; c>=0; c--)
    if (module[c].installed) {
      PRINTF("Unstalling module: %s\n", module[c].name);
      (*module[c].exit)();
      module[c].installed = false;
    }
}
