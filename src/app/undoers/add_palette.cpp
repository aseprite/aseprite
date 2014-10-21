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

#include "app/undoers/add_palette.h"

#include "app/undoers/remove_palette.h"
#include "doc/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace undo;

AddPalette::AddPalette(ObjectsContainer* objects, Sprite* sprite, FrameNumber paletteFrame)
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
  Palette* palette = sprite->getPalette(FrameNumber(m_paletteFrame));

  redoers->pushUndoer(new RemovePalette(objects, sprite, FrameNumber(m_paletteFrame)));

  sprite->deletePalette(palette);
}

} // namespace undoers
} // namespace app
