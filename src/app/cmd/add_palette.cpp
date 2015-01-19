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

#include "app/cmd/add_palette.h"

#include "doc/sprite.h"
#include "doc/palette.h"
#include "doc/palette_io.h"

namespace app {
namespace cmd {

using namespace doc;

AddPalette::AddPalette(Sprite* sprite, Palette* pal)
  : WithSprite(sprite)
  , WithPalette(pal)
{
}

void AddPalette::onExecute()
{
  Sprite* sprite = this->sprite();
  Palette* palette = this->palette();

  sprite->setPalette(palette, true);
}

void AddPalette::onUndo()
{
  Sprite* sprite = this->sprite();
  Palette* pal = this->palette();

  write_palette(m_stream, pal);

  sprite->deletePalette(pal);
  delete pal;
}

void AddPalette::onRedo()
{
  Sprite* sprite = this->sprite();
  Palette* pal = read_palette(m_stream);

  sprite->setPalette(pal, true);

  m_stream.str(std::string());
  m_stream.clear();
}

} // namespace cmd
} // namespace app
