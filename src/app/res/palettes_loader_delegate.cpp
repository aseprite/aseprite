// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "ui/system.h"

namespace app {

PalettesLoaderDelegate::PalettesLoaderDelegate()
{
  // Necessary to load preferences in the UI-thread which will be used
  // in a FileOp executed in a background thread.
  m_config.fillFromPreferences();
}

void PalettesLoaderDelegate::getResourcesPaths(std::map<std::string, std::string>& idAndPath) const
{
  // Include extension palettes
  idAndPath = App::instance()->extensions().palettes();

  // Search old palettes too
  std::string path;
  ResourceFinder rf;
  rf.includeDataDir("palettes"); // data/palettes/ in all places
  rf.includeUserDir("palettes"); // palettes/ in user home
  while (rf.next()) {
    if (base::is_directory(rf.filename())) {
      path = rf.filename();
      path = base::fix_path_separators(path);
      for (const auto& fn : base::list_files(path)) {
        // Ignore the default palette that is inside the palettes/ dir
        // in the user home dir.
        if (fn == "default.ase" ||
            fn == "default.gpl")
          continue;

        std::string fullFn = base::join_path(path, fn);
        if (base::is_file(fullFn))
          idAndPath[base::get_file_title(fn)] = fullFn;
      }
    }
  }
}

Resource* PalettesLoaderDelegate::loadResource(const std::string& id,
                                               const std::string& path)
{
  doc::Palette* palette = load_palette(path.c_str(), &m_config);
  if (palette)
    return new PaletteResource(id, path, palette);
  else
    return nullptr;
}

} // namespace app
