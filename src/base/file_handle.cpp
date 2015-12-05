// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/file_handle.h"

#include "base/string.h"

#include <stdexcept>

#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
#endif

#include <sys/stat.h>
#include <fcntl.h>

#ifndef O_BINARY
#define O_BINARY  0
#define O_TEXT    0
#endif

using namespace std;

namespace base {

FILE* open_file_raw(const string& filename, const string& mode)
{
#ifdef _WIN32
  return _wfopen(from_utf8(filename).c_str(),
                 from_utf8(mode).c_str());
#else
  return fopen(filename.c_str(), mode.c_str());
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

int open_file_descriptor_with_exception(const string& filename, const string& mode)
{
  int flags = 0;
  if (mode.find('r') != string::npos) flags |= O_RDONLY;
  if (mode.find('w') != string::npos) flags |= O_RDWR | O_CREAT | O_TRUNC;
  if (mode.find('b') != string::npos) flags |= O_BINARY;

  int fd;
#ifdef _WIN32
  fd = _wopen(from_utf8(filename).c_str(), flags, _S_IREAD | _S_IWRITE);
#else
  fd = open(filename.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
#endif

  if (fd == -1)
    throw runtime_error("Cannot open " + filename);

  return fd;
}

}
