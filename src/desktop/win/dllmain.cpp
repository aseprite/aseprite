// Desktop Integration
// Copyright (C) 2017-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/string.h"
#include "base/win/registry.h"
#include "base/win/win32_exception.h"
#include "desktop/win/class_factory.h"
#include "desktop/win/thumbnail_handler.h"

#include <new>

// This CLSID is defined by Windows for IThumbnailProvider implementations.
#define THUMBNAILHANDLER_SHELL_EXTENSION_CLSID "{E357FCCD-A995-4576-B01F-234630154E96}"

// This CLSID is defined by us, and can be used to create class factory for ThumbnailHandler
// instances.
#define THUMBNAILHANDLER_CLSID_STRING "{A5E9417E-6E7A-4B2D-85A4-84E114D7A960}"
#define THUMBNAILHANDLER_NAME_STRING  "Aseprite Thumbnail Handler"

using namespace desktop;

namespace {

namespace Global {
long refCount = 0;
HINSTANCE hInstance = nullptr;
CLSID clsidThumbnailHandler;
} // namespace Global

template<class T>
class ClassFactoryDelegateImpl : public ClassFactoryDelegate {
public:
  HRESULT lockServer(const bool lock) override
  {
    if (lock)
      InterlockedIncrement(&Global::refCount);
    else
      InterlockedDecrement(&Global::refCount);
    return S_OK;
  }
  HRESULT createInstance(REFIID riid, void** ppvObject) override
  {
    return T::CreateInstance(riid, ppvObject);
  }
};

} // anonymous namespace

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, void*)
{
  if (dwReason == DLL_PROCESS_ATTACH) {
    CLSIDFromString(base::from_utf8(THUMBNAILHANDLER_CLSID_STRING).c_str(),
                    &Global::clsidThumbnailHandler);

    Global::hInstance = hInstance;
    DisableThreadLibraryCalls(hInstance);
  }
  return TRUE;
}

STDAPI DllCanUnloadNow()
{
  return (Global::refCount == 0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv)
{
  HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

  if (IsEqualCLSID(clsid, Global::clsidThumbnailHandler)) {
    ClassFactoryDelegate* delegate = new (std::nothrow) ClassFactoryDelegateImpl<ThumbnailHandler>;
    if (!delegate)
      return E_OUTOFMEMORY;

    IClassFactory* classFactory = new (std::nothrow) ClassFactory(delegate);
    if (!classFactory) {
      delete delegate;
      return E_OUTOFMEMORY;
    }

    hr = classFactory->QueryInterface(riid, ppv);
    classFactory->Release();
  }

  return hr;
}

STDAPI DllRegisterServer()
{
  WCHAR dllPath[MAX_PATH];
  if (!GetModuleFileNameW(Global::hInstance, dllPath, sizeof(dllPath) / sizeof(dllPath[0])))
    return HRESULT_FROM_WIN32(GetLastError());

  auto hkcu = base::hkey::current_user();
  auto k = hkcu.create("Software\\Classes\\CLSID\\" THUMBNAILHANDLER_CLSID_STRING);
  k.string("", THUMBNAILHANDLER_NAME_STRING);
  k = k.create("InProcServer32");
  k.string("", base::to_utf8(dllPath));
  k.string("ThreadingModel", "Apartment");

  k = hkcu.create("Software\\Classes\\AsepriteFile");
  k.create("ShellEx\\" THUMBNAILHANDLER_SHELL_EXTENSION_CLSID)
    .string("", THUMBNAILHANDLER_CLSID_STRING);

  // TypeOverlay = empty string = don't show the app icon overlay in the thumbnail
  k.string("TypeOverlay", "");

  // Treatment = 2 = photo border
  k.dword("Treatment", 2);

  return S_OK;
}

STDAPI DllUnregisterServer()
{
  HRESULT hr = S_OK;
  auto hkcu = base::hkey::current_user();
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
