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

#include "undoers/remove_palette.h"

#include "raster/palette.h"
#include "raster/palette_io.h"
#include "raster/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"
#include "undoers/add_palette.h"
#include "undoers/object_io.h"

using namespace undo;
using namespace undoers;

RemovePalette::RemovePalette(ObjectsContainer* objects, Sprite* sprite, Palette* palette)
  : m_spriteId(objects->addObject(sprite))
{
  write_object(objects, m_stream, palette, raster::write_palette);
}

void RemovePalette::dispose()
{
  delete this;
}

void RemovePalette::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);
  UniquePtr<Palette> palette(read_object<Palette>(objects, m_stream, raster::read_palette));
  
  // Push an AddPalette as redoer
  redoers->pushUndoer(new AddPalette(objects, sprite, palette));

  sprite->setPalette(palette, true);

  // Here the palette is deleted by UniquePtr<>, it is needed becase
  // Sprite::setPalette() makes a copy of the given palette.
}
