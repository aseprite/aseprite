/* ASEPRITE
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

#include "app/undoers/open_group.h"

#include "app/undoers/close_group.h"
#include "raster/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace raster;
using namespace undo;

OpenGroup::OpenGroup(ObjectsContainer* objects,
                     const char* label,
                     Modification modification,
                     Sprite* sprite,
                     const SpritePosition& pos)
  : m_label(label)
  , m_modification(modification)
  , m_spriteId(objects->addObject(sprite))
  , m_spritePosition(pos)
{
}

void OpenGroup::dispose()
{
  delete this;
}

void OpenGroup::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);

  redoers->pushUndoer(new CloseGroup(objects, m_label, m_modification, sprite,
                                     m_spritePosition));
}

} // namespace undoers
} // namespace app
