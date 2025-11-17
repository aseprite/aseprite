// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/win/file_associations.h"

#include "app/win/notify_shell.h"
#include "base/fs.h"
#include "base/win/registry.h"
#include "fmt/format.h"
#include "os/error.h"
#include "ver/info.h"

namespace app { namespace win {

using hkey = base::hkey;

static void configure_shell_open(hkey& k)
{
  hkey open = k.create("shell\\open");

  hkey command = open.create("command");
  command.string("", fmt::format("\"{}\"", base::get_app_path()));

  hkey ddeexec = open.create("ddeexec");
  ddeexec.string("", "[open(\"%1\")]");
  ddeexec.create("application").string("", "Aseprite");
  ddeexec.create("topic").string("", "system");
}

void associate_file_type_with_asepritefile_class(const std::string& extension)
{
  try {
    hkey hkcr = hkey::current_user().open("Software\\Classes", hkey::write);

    // HKEY_CLASSES_ROOT\.extension

    hkey k = hkcr.create(std::string(".") + extension);
    if (k.string("") != "AsepriteFile")
      k.string("", "AsepriteFile");

    // HKEY_CLASSES_ROOT\AsepriteFile

    k = hkcr.create("AsepriteFile");
    hkey icon = k.create("DefaultIcon");
    icon.string("", fmt::format("{},1", base::get_app_path()));

    configure_shell_open(k);

    notify_shell_about_association_change_regen_thumbnails();
  }
  catch (const std::exception& ex) {
    os::error_message(ex.what());
  }
}

void add_aseprite_to_open_with_file_type(const std::string& extension)
{
  try {
    hkey hkcr = hkey::current_user().open("Software\\Classes", hkey::write);

    // HKEY_CLASSES_ROOT\.extension
    hkey k = hkcr.create(fmt::format(".{}\\OpenWithProgids", extension));
    k.string("IgaraStudio.Aseprite", "");

    // HKEY_CLASSES_ROOT\IgaraStudio.Aseprite
    k = hkcr.create("IgaraStudio.Aseprite");
    k.create("Application").string("ApplicationName", "Aseprite (running instance)");
    configure_shell_open(k);

    notify_shell_about_association_change_regen_thumbnails();
  }
  catch (const std::exception& ex) {
    os::error_message(ex.what());
  }
}

}} // namespace app::win
