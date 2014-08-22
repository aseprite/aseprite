/* Aseprite
 * Copyright (C) 2014  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/res/palettes_loader_delegate.h"

#include "app/file/palette_file.h"
#include "app/file_system.h"
#include "app/res/palette_resource.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/scoped_value.h"
#include "raster/palette.h"

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

Resource* PalettesLoaderDelegate::loadResource(const std::string& filename)
{
  raster::Palette* palette = load_palette(filename.c_str());
  if (!palette)
    return NULL;

  return new PaletteResource(palette, base::get_file_title(filename));
}

} // namespace app
