// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/file_handle.h"

#include "base/string.h"

#include <stdexcept>

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

namespace base {

FILE* open_file_raw(const string& filename, const string& mode)
{
#ifdef WIN32
  return _wfopen(from_utf8(filename).c_str(),
                 from_utf8(mode).c_str());
#else
  return fopen(filename.c_str(), mode);
#endif
}

FileHandle open_file(const string& filename, const string& mode)
{
  return FileHandle(open_file_raw(filename, mode), fclose);
}

FileHandle open_file_with_exception(const string& filename, const string& mode)
{
  FileHandle f(open_file_raw(filename, mode), fclose);
  if (!f)
    throw runtime_error("Cannot open " + filename);
  return f;
}

}
