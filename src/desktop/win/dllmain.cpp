// Desktop Integration
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/string.h"
#include "base/win/registry.h"
#include "base/win/win32_exception.h"
#include "desktop/win/class_factory.h"
#include "desktop/win/thumbnail_handler.h"

#include "desktop/win/helpers.h"

#include <new>

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

  bool enabled = true;
  bool overlay = true; // Enable overlay by default

  // Read user preferences to avoid enabling thumbnails if the user
  // doesn't want  them.
  DWORD flags = 0;
  if (win::get_user_preferences(flags)) {
    enabled = (flags & THUMBNAILER_FLAG_ENABLED ? true : false);
    overlay = (flags & THUMBNAILER_FLAG_OVERLAY ? true : false);
  }

  if (enabled)
    return win::register_thumbnailer(base::to_utf8(dllPath), overlay);
  else
    return win::unregister_thumbnailer();
}

STDAPI DllUnregisterServer()
{
  return win::unregister_thumbnailer();
}
