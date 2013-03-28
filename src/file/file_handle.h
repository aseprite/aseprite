/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef FILE_HANDLE_FORMAT_H_INCLUDED
#define FILE_HANDLE_FORMAT_H_INCLUDED

#include "base/exception.h"
#include "base/unique_ptr.h"

#include <cstdio>
#include <string>

class FileHandle
{
public:
  FileHandle(const char* fileName, const char* mode)
    : m_handle(std::fopen(fileName, mode), std::fclose)
  {
    if (!m_handle)
      throw base::Exception(std::string("Cannot open ") + fileName);
  }

  operator std::FILE*() { return m_handle; }

private:
  UniquePtr<std::FILE, int(*)(std::FILE*)> m_handle;
};

#endif
