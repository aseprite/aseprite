// Desktop Integration
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DESKTOP_WIN_HELPERS_H_INCLUDED
#define DESKTOP_WIN_HELPERS_H_INCLUDED

// This file is used from aseprite-thumbnailer and from aseprite itself.

#include "base/string.h"
#include "base/win/registry.h"
#include "base/win/win32_exception.h"

// This CLSID is defined by Windows for IThumbnailProvider implementations.
#define THUMBNAILHANDLER_SHELL_EXTENSION_CLSID "{E357FCCD-A995-4576-B01F-234630154E96}"

// This CLSID is defined by us, and can be used to create class factory for ThumbnailHandler
// instances.
#define THUMBNAILHANDLER_CLSID_STRING "{A5E9417E-6E7A-4B2D-85A4-84E114D7A960}"
#define THUMBNAILHANDLER_NAME_STRING  "Aseprite Thumbnail Handler"

// Flags for register_user_preferences() & get_user_preferences() functions
#define THUMBNAILER_FLAG_ENABLED 1
#define THUMBNAILER_FLAG_OVERLAY 2

namespace desktop { namespace win {

using hkey = base::hkey;

static HRESULT register_thumbnailer(const std::string& dllPath, const bool overlay)
{
  HRESULT hr = S_OK;
  try {
    hkey hkcu = base::hkey::current_user();
    hkey k = hkcu.create("Software\\Classes\\CLSID\\" THUMBNAILHANDLER_CLSID_STRING);
    k.string("", THUMBNAILHANDLER_NAME_STRING);
    k = k.create("InProcServer32");
    k.string("", dllPath);
    k.string("ThreadingModel", "Apartment");

    k = hkcu.create("Software\\Classes\\AsepriteFile");
    k.create("ShellEx\\" THUMBNAILHANDLER_SHELL_EXTENSION_CLSID)
      .string("", THUMBNAILHANDLER_CLSID_STRING);

    // TypeOverlay = empty string = don't show the app icon overlay in the thumbnail
    if (!overlay)
      k.string("TypeOverlay", ""); // Without overlay
    else if (k.exists("TypeOverlay"))
      k.delete_value("TypeOverlay"); // With overlay

    // Treatment = 2 = photo border
    k.dword("Treatment", 2);
  }
  catch (const base::Win32Exception& ex) {
    hr = HRESULT_FROM_WIN32(ex.errorCode());
  }
  return hr;
}

static HRESULT unregister_thumbnailer()
{
  HRESULT hr = S_OK;
  hkey hkcu = base::hkey::current_user();
  try {
    hkcu.delete_tree(
      "Software\\Classes\\AsepriteFile\\ShellEx\\" THUMBNAILHANDLER_SHELL_EXTENSION_CLSID);
  }
  catch (const base::Win32Exception& ex) {
    hr = HRESULT_FROM_WIN32(ex.errorCode());
  }
  try {
    hkcu.delete_tree("Software\\Classes\\CLSID\\" THUMBNAILHANDLER_CLSID_STRING);
  }
  catch (const base::Win32Exception& ex) {
    hr = HRESULT_FROM_WIN32(ex.errorCode());
  }
  return hr;
}

// Registers the preferences of the user so when we register the
// thumbnailer through the installer these preferences are not
// overwritten.
static void set_user_preferences(DWORD flags)
{
  try {
    hkey hkcu = base::hkey::current_user();
    hkey k = hkcu.create("Software\\Classes\\AsepriteFile");
    if (!k)
      return;

    k.dword("UserFlags", flags);
  }
  catch (const base::Win32Exception&) {
    // Do nothing
  }
}

static bool get_user_preferences(DWORD& flags)
{
  try {
    hkey hkcu = base::hkey::current_user();
    hkey k = hkcu.open("Software\\Classes\\AsepriteFile", hkey::read);
    if (!k || !k.exists("UserFlags"))
      return false;

    flags = k.dword("UserFlags");
    return true;
  }
  catch (const base::Win32Exception&) {
    // Do nothing
    return false;
  }
}

}} // namespace desktop::win

#endif // DESKTOP_WIN_HELPERS_H_INCLUDED
