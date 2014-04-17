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

#ifndef APP_PALETTES_LOADER_H_INCLUDED
#define APP_PALETTES_LOADER_H_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "base/thread.h"
#include "base/unique_ptr.h"

namespace raster {
  class Palette;
}

namespace app {

  class PalettesLoader {
  public:
    PalettesLoader();
    ~PalettesLoader();

    void cancel();
    bool done();
    bool isDone() const { return m_done; }

    bool next(base::UniquePtr<raster::Palette>& palette, std::string& name);

    static std::string palettesLocation();

  private:
    void threadLoadPalettes();

    struct Item {
      raster::Palette* palette;
      std::string name;
      Item() : palette(NULL) {
      }
      Item(raster::Palette* palette, const std::string& name)
        : palette(palette), name(name) {
      }
    };

    typedef base::concurrent_queue<Item> Queue;

    bool m_done;
    bool m_cancel;
    Queue m_queue;
    base::thread m_thread;
  };

} // namespace app

#endif
