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

#include "app/undoers/set_sprite_transparent_color.h"

#include "raster/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace undo;

SetSpriteTransparentColor::SetSpriteTransparentColor(ObjectsContainer* objects, Sprite* sprite)
  : m_spriteId(objects->addObject(sprite))
  , m_maskColor(sprite->getTransparentColor())
{
}

void SetSpriteTransparentColor::dispose()
{
  delete this;
}

void SetSpriteTransparentColor::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);

  // Push another SetSpriteTransparentColor as redoer
  redoers->pushUndoer(new SetSpriteTransparentColor(objects, sprite));

  sprite->setTransparentColor(m_maskColor);
}

} // namespace undoers
} // namespace app
