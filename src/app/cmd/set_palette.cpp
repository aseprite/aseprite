/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#include "app/cmd/set_palette.h"

#include "base/serialization.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

SetPalette::SetPalette(Sprite* sprite, frame_t frame, Palette* newPalette)
  : WithSprite(sprite)
  , m_frame(frame)
{
  Palette* curPalette = sprite->palette(frame);

  // Check differences between current sprite palette and the new one
  m_from = m_to = -1;
  curPalette->countDiff(newPalette, &m_from, &m_to);

  if (m_from >= 0 && m_to >= m_from) {
    size_t ncolors = m_to-m_from+1;
    m_oldColors.resize(ncolors);
    m_newColors.resize(ncolors);

    for (size_t i=0; i<ncolors; ++i) {
      m_oldColors[i] = curPalette->getEntry(m_from+i);
      m_newColors[i] = newPalette->getEntry(m_from+i);
    }
  }
}

void SetPalette::onExecute()
{
  Sprite* sprite = this->sprite();
  Palette* palette = sprite->palette(m_frame);

  for (size_t i=0; i<m_newColors.size(); ++i)
    palette->setEntry(m_from+i, m_newColors[i]);
}

void SetPalette::onUndo()
{
  Sprite* sprite = this->sprite();
  Palette* palette = sprite->palette(m_frame);

  for (size_t i=0; i<m_oldColors.size(); ++i)
    palette->setEntry(m_from+i, m_oldColors[i]);
}

} // namespace cmd
} // namespace app
