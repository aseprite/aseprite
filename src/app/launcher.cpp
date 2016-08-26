// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/launcher.h"

#include "base/exception.h"
#include "base/launcher.h"
#include "ui/alert.h"

namespace app {
namespace launcher {

void open_url(const std::string& url)
{
  open_file(url);
}

void open_file(const std::string& file)
{
  if (!base::launcher::open_file(file))
    ui::Alert::show("Problem<<Cannot open file:<<%s||&Close", file.c_str());
}

void open_folder(const std::string& file)
{
  if (!base::launcher::open_folder(file))
    ui::Alert::show("Problem<<Cannot open folder:<<%s||&Close", file.c_str());
}

} // namespace launcher
} // namespace app
