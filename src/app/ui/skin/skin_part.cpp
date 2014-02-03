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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_part.h"

#include <allegro.h>

namespace app {
namespace skin {

SkinPart::SkinPart()
{
}

SkinPart::~SkinPart()
{
  clear();
}

void SkinPart::clear()
{
  for (Bitmaps::iterator it = m_bitmaps.begin(), end = m_bitmaps.end();
       it != end; ++it) {
    ASSERT(*it != NULL);

    destroy_bitmap(*it);
    *it = NULL;
  }
}

void SkinPart::setBitmap(size_t index, BITMAP* bitmap)
{
  if (index >= m_bitmaps.size())
    m_bitmaps.resize(index+1, NULL);

  m_bitmaps[index] = bitmap;
}

} // namespace skin
} // namespace app
