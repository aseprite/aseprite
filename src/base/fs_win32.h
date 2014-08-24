// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <stdexcept>
#include <windows.h>
#include <shlobj.h>
#include <sys/stat.h>

#include "base/path.h"
#include "base/string.h"
#include "base/win32_exception.h"

namespace base {

bool is_file(const std::string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  // GetFileAttributes returns INVALID_FILE_ATTRIBUTES in case of
  // fail.
  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool is_directory(const std::string& path)
{
  DWORD attr = ::GetFileAttributes(from_utf8(path).c_str());

  return ((attr != INVALID_FILE_ATTRIBUTES) &&
          ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
}

size_t file_size(const std::string& path)
{
  struct _stat sts;
  return (_wstat(from_utf8(path).c_str(), &sts) == 0) ? sts.st_size: 0;
}

void move_file(const std::string& src, const std::string& dst)
{
  BOOL result = ::MoveFile(from_utf8(src).c_str(), from_utf8(dst).c_str());
  if (result == 0)
    throw Win32Exception("Error moving file");
}

void delete_file(const std::string& path)
{
  BOOL result = ::DeleteFile(from_utf8(path).c_str());
  if (result == 0)
    throw Win32Exception("Error deleting file");
}

bool has_readonly_attr(const std::string& path)
{
  std::wstring fn = from_utf8(path);
  DWORD attr = ::GetFileAttributes(fn.c_str());
  return ((attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY);
}

void remove_readonly_attr(const std::string& path)
{
  std::wstring fn = from_utf8(path);
  DWORD attr = ::GetFileAttributes(fn.c_str());
  if ((attr & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY)
    ::SetFileAttributes(fn.c_str(), attr & ~FILE_ATTRIBUTE_READONLY);
}

void make_directory(const std::string& path)
{
  BOOL result = ::CreateDirectory(from_utf8(path).c_str(), NULL);
  if (result == 0)
    throw Win32Exception("Error creating directory");
}

void remove_directory(const std::string& path)
{
  BOOL result = ::RemoveDirectory(from_utf8(path).c_str());
  if (result == 0)
    throw Win32Exception("Error removing directory");
}

std::string get_app_path()
{
  TCHAR buffer[MAX_PATH+1];
  if (::GetModuleFileName(NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
    return to_utf8(buffer);
  else
    return "";
}

std::string get_temp_path()
{
  TCHAR buffer[MAX_PATH+1];
  DWORD result = ::GetTempPath(sizeof(buffer)/sizeof(TCHAR), buffer);
  return to_utf8(buffer);
}

}
