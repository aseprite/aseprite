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

#include "app/undoers/remap_palette.h"

#include "app/undoers/add_cel.h"
#include "app/undoers/object_io.h"
#include "doc/sprite.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace doc;
using namespace undo;

RemapPalette::RemapPalette(ObjectsContainer* objects, Sprite* sprite,
                           FrameNumber frameFrom, FrameNumber frameTo,
                           const std::vector<uint8_t>& mapping)
  : m_spriteId(objects->addObject(sprite))
  , m_frameFrom(frameFrom)
  , m_frameTo(frameTo)
  , m_mapping(mapping)
{
  ASSERT(mapping.size() == 256 && "Mapping tables must have 256 entries");
}

void RemapPalette::dispose()
{
  delete this;
}

void RemapPalette::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Sprite* sprite = objects->getObjectT<Sprite>(m_spriteId);

  // Inverse mapping
  std::vector<uint8_t> inverse_mapping(m_mapping.size());
  for (size_t c=0; c<m_mapping.size(); ++c)
    inverse_mapping[m_mapping[c]] = c;

  // Push an RemapPalette as redoer
  redoers->pushUndoer(new RemapPalette(objects, sprite,
                                       m_frameFrom,
                                       m_frameTo, inverse_mapping));

  // Remap in inverse order
  sprite->remapImages(m_frameFrom, m_frameTo, inverse_mapping);
}

} // namespace undoers
} // namespace app
