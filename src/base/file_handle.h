// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_OPEN_FILE_H_INCLUDED
#define BASE_OPEN_FILE_H_INCLUDED

#include "base/shared_ptr.h"
#include "base/string.h"

#include <cstdio>

namespace base {
  
  typedef SharedPtr<FILE> FileHandle;

  FILE* open_file_raw(const string& filename, const string& mode);
  FileHandle open_file(const string& filename, const string& mode);
  FileHandle open_file_with_exception(const string& filename, const string& mode);
  int open_file_descriptor_with_exception(const string& filename, const string& mode);

}

#endif
