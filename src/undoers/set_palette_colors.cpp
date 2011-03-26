/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "undoers/set_palette_colors.h"

#include "base/serialization.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "undo/objects_container.h"
#include "undo/undo_exception.h"
#include "undo/undoers_collector.h"

using namespace undo;
using namespace undoers;
using namespace base::serialization::little_endian;

SetPaletteColors::SetPaletteColors(ObjectsContainer* objects, Sprite* sprite, Palette* palette, int from, int to)
  : m_spriteId(objects->addObject(sprite))
  , m_frame(sprite->getCurrentFrame())
  , m_from(from)
  , m_to(to)
{
  // Write (to-from+1) palette color entries
  for (int i=from; i<=to; ++i)
    write32(m_stream, palette->getEntry(i));
}

void SetPaletteColors::dispose()
{
  delete this;
}

void SetPaletteColors::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);
  Palette* palette = sprite->getPalette(m_frame);

  if (palette == NULL)
    throw UndoException("Palette not found when restoring colors");

  // Push another SetPaletteColors as redoer
  redoers->pushUndoer(new SetPaletteColors(objects, sprite, palette, m_from, m_to));

  // Read palette color entries
  for (int i=m_from; i<=m_to; ++i) {
    uint32_t color = read32(m_stream);
    palette->setEntry(i, color);
  }
}
