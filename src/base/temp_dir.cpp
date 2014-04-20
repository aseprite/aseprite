// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/temp_dir.h"

#include "base/convert_to.h"
#include "base/fs.h"
#include "base/path.h"

#include <cstdlib>

namespace base {

TempDir::TempDir()
{
}

TempDir::TempDir(const string& appName)
{
  for (int i=(std::rand()%0xffff); ; ++i) {
    m_path = join_path(get_temp_path(),
                       appName + convert_to<string>(i));

    if (!is_directory(m_path)) {
      make_directory(m_path);
      break;
    }
  }
}

TempDir::~TempDir()
{
  remove();
}

void TempDir::remove()
{
  if (!m_path.empty()) {
    try {
      remove_directory(m_path);
    }
    catch (const std::exception&) {
      // Ignore exceptions if the directory cannot be removed.
    }
    m_path.clear();
  }
}

void TempDir::attach(const string& path)
{
  remove();

  ASSERT(is_directory(path));
  m_path = path;
}

}
