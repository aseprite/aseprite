/* Aseprite
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

#ifndef APP_UI_SKIN_SKIN_PART_H_INCLUDED
#define APP_UI_SKIN_SKIN_PART_H_INCLUDED
#pragma once

#include <vector>
#include "base/shared_ptr.h"

struct BITMAP;

namespace app {
  namespace skin {

    class SkinPart {
    public:
      typedef std::vector<BITMAP*> Bitmaps;

      SkinPart();
      ~SkinPart();

      size_t size() const { return m_bitmaps.size(); }

      void clear();

      // It doesn't destroy the previous bitmap in the given "index".
      void setBitmap(size_t index, BITMAP* bitmap);

      BITMAP* getBitmap(size_t index) const {
        return (index < m_bitmaps.size() ? m_bitmaps[index]: NULL);
      }

    private:
      Bitmaps m_bitmaps;
    };

    typedef SharedPtr<SkinPart> SkinPartPtr;

  } // namespace skin
} // namespace app

#endif
