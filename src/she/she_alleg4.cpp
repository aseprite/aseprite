// SHE library
// Copyright (C) 2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she.h"

#include <allegro.h>
#include "loadpng.h"

#include <cassert>

namespace she {

class Alleg4System : public System {
public:
  Alleg4System() {
    allegro_init();
    set_uformat(U_ASCII);
    install_timer();

    // Register PNG as a supported bitmap type
    register_bitmap_file_type("png", load_png, save_png);
  }

  ~Alleg4System() {
    remove_timer();
    allegro_exit();
  }

  void dispose() {
    delete this;
  }

};

System* CreateSystem() {
  return new Alleg4System();
}
  
}

#ifdef main
int main(int argc, char* argv[]) {
#undef main
  extern int main(int argc, char* argv[]);
  return main(argc, argv);
}

END_OF_MAIN();
#endif
