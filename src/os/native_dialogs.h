// LAF OS Library
// Copyright (C) 2015-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_NATIVE_DIALOGS_H_INCLUDED
#define OS_NATIVE_DIALOGS_H_INCLUDED
#pragma once

#include "base/paths.h"

#include <string>

namespace os {
  class Display;

  class FileDialog {
  public:
    enum class Type {
      OpenFile,
      OpenFiles,
      OpenFolder,
      SaveFile,
    };

    virtual ~FileDialog() { }
    virtual void dispose() = 0;
    virtual void setType(const Type type) = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void setDefaultExtension(const std::string& extension) = 0;
    virtual void addFilter(const std::string& extension, const std::string& description) = 0;
    virtual std::string fileName() = 0;
    virtual void getMultipleFileNames(base::paths& output) = 0;
    virtual void setFileName(const std::string& filename) = 0;
    virtual bool show(Display* parent) = 0;
  };

  class NativeDialogs {
  public:
    virtual ~NativeDialogs() { }
    virtual FileDialog* createFileDialog() = 0;
  };

} // namespace os

#endif
