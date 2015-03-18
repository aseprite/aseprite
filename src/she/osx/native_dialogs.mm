// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include "she/osx/native_dialogs.h"

namespace she {

NativeDialogsOSX::NativeDialogsOSX()
{
}

FileDialog* NativeDialogsOSX::createFileDialog()
{
  return nullptr;
}

} // namespace she
