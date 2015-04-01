// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_OPEN_FILE_H_INCLUDED
#define BASE_OPEN_FILE_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"

#include <cstdio>
#include <string>

namespace base {

  typedef base::SharedPtr<FILE> FileHandle;

  FILE* open_file_raw(const std::string& filename, const std::string& mode);
  FileHandle open_file(const std::string& filename, const std::string& mode);
  FileHandle open_file_with_exception(const std::string& filename, const std::string& mode);
  int open_file_descriptor_with_exception(const std::string& filename, const std::string& mode);

}

#endif
