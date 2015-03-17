// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_NATIVE_DIALOGS_H_INCLUDED
#define SHE_OSX_NATIVE_DIALOGS_H_INCLUDED
#pragma once

#include "she/native_dialogs.h"

namespace she {

  class NativeDialogsOSX : public NativeDialogs {
  public:
    NativeDialogsOSX();
    FileDialog* createFileDialog() override;
  };

} // namespace she

#endif
