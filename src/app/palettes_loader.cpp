/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/palettes_loader.h"

#include "app/file_system.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/scoped_value.h"
#include "raster/palette.h"

namespace app {

PalettesLoader::PalettesLoader()
  : m_done(false)
  , m_cancel(false)
  , m_thread(Bind<void>(&PalettesLoader::threadLoadPalettes, this))
{
  PRINTF("PalettesLoader::PalettesLoader()\n");
}

PalettesLoader::~PalettesLoader()
{
  m_thread.join();

  PRINTF("PalettesLoader::~PalettesLoader()\n");
}

void PalettesLoader::cancel()
{
  m_cancel = true;
}

bool PalettesLoader::done()
{
  return m_done;
}

bool PalettesLoader::next(base::UniquePtr<raster::Palette>& palette, std::string& name)
{
  Item item;
  if (m_queue.try_pop(item)) {
    palette.reset(item.palette);
    name = item.name;
    return true;
  }
  else
    return false;
}

// static
std::string PalettesLoader::palettesLocation()
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

void PalettesLoader::threadLoadPalettes()
{
  PRINTF("threadLoadPalettes()\n");

  base::ScopedValue<bool> scoped(m_done, false, true);

  std::string path = palettesLocation();
  PRINTF("Loading palettes from %s...\n", path.c_str());
  if (path.empty())
    return;

  FileSystemModule* fs = FileSystemModule::instance();
  LockFS lock(fs);

  IFileItem* item = fs->getFileItemFromPath(path);
  if (!item)
    return;

  FileItemList list = item->getChildren();
  for (FileItemList::iterator it=list.begin(), end=list.end();
       it != end; ++it) {
    if (m_cancel)
      break;

    raster::Palette* palette =
      raster::Palette::load((*it)->getFileName().c_str());
    
    if (palette) {
      m_queue.push(Item(palette, base::get_file_title((*it)->getFileName())));
    }
  }
}

} // namespace app
