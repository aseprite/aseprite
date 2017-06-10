// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/res/palettes_loader_delegate.h"

#include "app/app.h"
#include "app/extensions.h"
#include "app/file/palette_file.h"
#include "app/file_system.h"
#include "app/res/palette_resource.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/scoped_value.h"
#include "doc/palette.h"

namespace app {

std::string PalettesLoaderDelegate::resourcesLocation() const
{
  std::string path;
  ResourceFinder rf;
  rf.includeDataDir("palettes");
  while (rf.next()) {
    if (base::is_directory(rf.filename())) {
      path = rf.filename();
      break;
    }
  }
  return base::fix_path_separators(path);
}

const std::map<std::string, std::string>& PalettesLoaderDelegate::extensionResources() const
{
  return App::instance()->extensions().palettes();
}

Resource* PalettesLoaderDelegate::loadResource(const std::string& id,
                                               const std::string& path)
{
  doc::Palette* palette = load_palette(path.c_str());
  if (palette)
    return new PaletteResource(id, palette);
  else
    return nullptr;
}

} // namespace app
