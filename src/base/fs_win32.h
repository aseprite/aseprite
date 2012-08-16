// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include <windows.h>
#include <stdexcept>

namespace base {

bool file_exists(const string& path)
{
  DWORD attr = ::GetFileAttributes(path.c_str());

  // GetFileAttributes returns INVALID_FILE_ATTRIBUTES in case of
  // fail.
  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool directory_exists(const string& path)
{
  DWORD attr = ::GetFileAttributes(path.c_str());

  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
}

void make_directory(const string& path)
{
  BOOL result = ::CreateDirectory(path.c_str(), NULL);
  if (result == 0) {
    // TODO add GetLastError() value into the exception
    throw std::runtime_error("Error creating directory");
  }
}

void remove_directory(const string& path)
{
  BOOL result = ::RemoveDirectory(path.c_str());
  if (result == 0) {
    // TODO add GetLastError() value into the exception
    throw std::runtime_error("Error removing directory");
  }
}

string get_temp_path()
{
  TCHAR buffer[MAX_PATH+1];
  DWORD result = GetTempPath(sizeof(buffer)/sizeof(TCHAR), buffer);
  return string(buffer);
}

}
