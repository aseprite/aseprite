/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "undoers/add_palette.h"

#include "raster/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"
#include "undoers/remove_palette.h"

using namespace undo;
using namespace undoers;

AddPalette::AddPalette(ObjectsContainer* objects, Sprite* sprite, int paletteFrame)
  : m_spriteId(objects->addObject(sprite))
  , m_paletteFrame(paletteFrame)
{
}

void AddPalette::dispose()
{
  delete this;
}

void AddPalette::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);
  Palette* palette = sprite->getPalette(m_paletteFrame);

  redoers->pushUndoer(new RemovePalette(objects, sprite, m_paletteFrame));

  sprite->deletePalette(palette);
}
