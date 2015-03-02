// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_NATIVE_DIALOGS_H_INCLUDED
#define SHE_NATIVE_DIALOGS_H_INCLUDED
#pragma once

#include "she/display_handle.h"

#include <string>

namespace she {

  class FileDialog {
  public:
    virtual ~FileDialog() { }
    virtual void dispose() = 0;
    virtual void toOpenFile() = 0; // Configure the dialog to open a file
    virtual void toSaveFile() = 0; // Configure the dialog to save a file
    virtual void setTitle(const std::string& title) = 0;
    virtual void setDefaultExtension(const std::string& extension) = 0;
    virtual void addFilter(const std::string& extension, const std::string& description) = 0;
    virtual std::string getFileName() = 0;
    virtual void setFileName(const std::string& filename) = 0;
    virtual bool show(DisplayHandle parent) = 0;
  };

  class NativeDialogs {
  public:
    virtual ~NativeDialogs() { }
    virtual FileDialog* createFileDialog() = 0;
  };

} // namespace she

#endif
