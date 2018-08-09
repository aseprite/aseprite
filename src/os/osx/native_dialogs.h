// LAF OS Library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_NATIVE_DIALOGS_H_INCLUDED
#define OS_OSX_NATIVE_DIALOGS_H_INCLUDED
#pragma once

#include "os/native_dialogs.h"

namespace os {

  class NativeDialogsOSX : public NativeDialogs {
  public:
    NativeDialogsOSX();
    FileDialog* createFileDialog() override;
  };

} // namespace os

#endif
