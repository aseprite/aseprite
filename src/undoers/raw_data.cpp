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

#include "undoers/raw_data.h"

#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

using namespace undo;
using namespace undoers;

RawData::RawData(ObjectsContainer* objects, void* object, void* fieldAddress, int fieldSize)
  : m_objectId(objects->addObject(object))
  , m_offset((uint32_t)(((uint8_t*)fieldAddress) - ((uint8_t*)object)))
  , m_data(fieldSize)
{
  memcpy(&m_data[0], fieldAddress, m_data.size());
}

void RawData::dispose()
{
  delete this;
}

void RawData::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  void* object = objects->getObject(m_objectId);
  void* fieldAddress = (void*)(((uint8_t*)object) + m_offset);

  // Save the current data
  redoers->pushUndoer(new RawData(objects, object, fieldAddress, m_data.size()));

  // Copy back the old data
  memcpy(fieldAddress, &m_data[0], m_data.size());
}
