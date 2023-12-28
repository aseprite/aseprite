#include "app_init.h"

#if LAF_WINDOWS
#include <windows.h>

namespace app {

// Successful calls to CoInitialize() (S_OK or S_FALSE) must match
// the calls to CoUninitialize().
// From: https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-couninitialize#remarks
CoInit::CoInit()
{
  hr = CoInitialize(nullptr);
}

CoInit::~CoInit()
{
  if (hr == S_OK || hr == S_FALSE)
    CoUninitialize();
}

}  // namespace app

#endif  // LAF_WINDOWS

