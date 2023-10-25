// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cli/app_options.h"
#include "os/error.h"

#include <iostream>


// Aseprite entry point. (Called from "os" library.)
int app_main(int argc, char* argv[])
{
  try {
    app::AppOptions options(argc, const_cast<const char**>(argv));
    app::App app;
    const int code = app.initialize(options);

    app.run();

    // After starting the GUI, we'll always return 0, but in batch
    // mode we can return the error code.
    return (app.isGui() ? 0: code);
  }
  catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    os::error_message(e.what());
    return 1;
  }
}
