// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/win/thumbnails.h"

#include "app/win/notify_shell.h"
#include "base/fs.h"

// Helpers functions/constants to register/unregister the thumbnailer directly.
#include "desktop/win/helpers.h"

namespace app { namespace win {

using hkey = base::hkey;

const char* kAsepriteThumbnailerDllName = "aseprite-thumbnailer.dll";

std::string get_thumbnailer_dll()
{
  std::string dll_path = base::join_path(base::get_file_path(base::get_app_path()),
                                         kAsepriteThumbnailerDllName);
  if (!base::is_file(dll_path))
    return {}; // DLL doesn't exist
  return dll_path;
}

ThumbnailsOption get_thumbnail_options(const std::string& extension_name)
{
  ThumbnailsOption opts;

  try {
    hkey hkcr = hkey::classes_root();
    hkey k = hkcr.open(std::string(".") + extension_name, hkey::read);
    if (!k || k.string("") != "AsepriteFile")
      return opts;

    k = hkcr.open("AsepriteFile", hkey::read);
    if (!k)
      return opts;
    opts.overlay = (!k.exists("TypeOverlay"));

    hkey hkcu = hkey::current_user();
    k = hkcu.open(
      "Software\\Classes\\AsepriteFile\\ShellEx\\" THUMBNAILHANDLER_SHELL_EXTENSION_CLSID,
      hkey::read);
    if (!k)
      return opts;

    std::string clsid = k.string("");
    if (clsid != THUMBNAILHANDLER_CLSID_STRING)
      return opts;

    opts.enabled = true;
  }
  catch (const std::exception&) {
    // Ignore any error reading the registry
  }

  return opts;
}

bool set_thumbnail_options(const std::string& extension_name, const ThumbnailsOption& options)
{
  // Get current options and check if they are different from the ones
  // that the user wants to specify.
  ThumbnailsOption current = get_thumbnail_options(extension_name);
  if (current == options)
    return false;

  std::string dll_path = get_thumbnailer_dll();
  if (dll_path.empty()) // DLL doesn't exist
    return false;

  if (options.enabled)
    desktop::win::register_thumbnailer(dll_path, options.overlay);
  else
    desktop::win::unregister_thumbnailer();

  // Set user preferences for the future, so if the thumbnailer DLL is
  // registered through regsvr32, it knowns what the user prefers and
  // avoid overwritting their preferences.
  DWORD flags = 0;
  if (options.enabled) {
    flags |= THUMBNAILER_FLAG_ENABLED;
    if (options.overlay)
      flags |= THUMBNAILER_FLAG_OVERLAY;
  }
  desktop::win::set_user_preferences(flags);

  notify_shell_about_association_change_regen_thumbnails();
  return true;
}

}} // namespace app::win
