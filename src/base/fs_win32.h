// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#include <windows.h>
#include <stdexcept>

#include "base/string.h"

namespace base {

bool file_exists(const string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  // GetFileAttributes returns INVALID_FILE_ATTRIBUTES in case of
  // fail.
  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool directory_exists(const string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
}

void make_directory(const string& path)
{
  BOOL result = ::CreateDirectory(from_utf8(path).c_str(), NULL);
  if (result == 0) {
    // TODO add GetLastError() value into the exception
    throw std::runtime_error("Error creating directory");
  }
}

void remove_directory(const string& path)
{
  BOOL result = ::RemoveDirectory(from_utf8(path).c_str());
  if (result == 0) {
    // TODO add GetLastError() value into the exception
    throw std::runtime_error("Error removing directory");
  }
}

string get_app_path()
{
  TCHAR buffer[MAX_PATH+1];
  if (::GetModuleFileName(NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
    return to_utf8(buffer);
  else
    return "";
}

string get_temp_path()
{
  TCHAR buffer[MAX_PATH+1];
  DWORD result = GetTempPath(sizeof(buffer)/sizeof(TCHAR), buffer);
  return to_utf8(buffer);
}

}
