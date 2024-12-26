// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/launcher.h"

#include "app/i18n/strings.h"
#include "base/exception.h"
#include "base/launcher.h"
#include "ui/alert.h"

namespace app { namespace launcher {

void open_url(const std::string& url)
{
  open_file(url);
}

void open_file(const std::string& file)
{
  if (!base::launcher::open_file(file))
    ui::Alert::show(Strings::alerts_cannot_open_file(file));
}

void open_folder(const std::string& file)
{
  if (!base::launcher::open_folder(file))
    ui::Alert::show(Strings::alerts_cannot_open_folder(file));
}

}} // namespace app::launcher
