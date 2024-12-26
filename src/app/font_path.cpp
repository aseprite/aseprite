// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/font_path.h"

#include "base/fs.h"

namespace app {

std::string find_font(const std::string& firstDir, const std::string& filename)
{
  std::string fn = base::join_path(firstDir, filename);
  if (base::is_file(fn))
    return fn;

  base::paths fontDirs;
  get_font_dirs(fontDirs);

  for (const std::string& dir : fontDirs) {
    fn = base::join_path(dir, filename);
    if (base::is_file(fn))
      return fn;
  }

  return std::string();
}

} // namespace app
