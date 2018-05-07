// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/modules.h"

#include "app/modules/gui.h"
#include "app/modules/palettes.h"

namespace app {

struct Module {
  const char *name;
  int (*init)();
  void (*exit)();
  int reqs;
  bool installed;
};

static Module module[] =
{
#define DEF_MODULE(name, reqs) \
  { #name, init_module_##name, exit_module_##name, (reqs), false }

  // This sorting is very important because last modules depend of
  // first ones.

  DEF_MODULE(palette,           0),
#ifdef ENABLE_UI
  DEF_MODULE(gui,               REQUIRE_INTERFACE),
#endif
};

static int modules = sizeof(module) / sizeof(Module);

LegacyModules::LegacyModules(int requirements)
{
  for (int c=0; c<modules; c++)
    if ((module[c].reqs & requirements) == module[c].reqs) {
      LOG("MODS: Installing module: %s\n", module[c].name);

      if ((*module[c].init)() < 0)
        throw base::Exception("Error initializing module: %s",
                              static_cast<const char*>(module[c].name));

      module[c].installed = true;
    }
}

LegacyModules::~LegacyModules()
{
  for (int c=modules-1; c>=0; c--)
    if (module[c].installed) {
      LOG("MODS: Unstalling module: %s\n", module[c].name);
      (*module[c].exit)();
      module[c].installed = false;
    }
}

} // namespace app
