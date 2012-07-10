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

#include "undoers/open_group.h"

#include "undo/objects_container.h"
#include "undo/undoers_collector.h"
#include "undoers/close_group.h"

using namespace undo;
using namespace undoers;

OpenGroup::OpenGroup(undo::ObjectsContainer* objects, const char* label, undo::Modification modification, Layer* layer, FrameNumber frame)
  : m_label(label)
  , m_modification(modification)
  , m_activeLayerId(objects->addObject(layer))
  , m_activeFrame(frame)
{
}

void OpenGroup::dispose()
{
  delete this;
}

void OpenGroup::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Layer* layer = objects->getObjectT<Layer>(m_activeLayerId);

  redoers->pushUndoer(new CloseGroup(objects, m_label, m_modification, layer, m_activeFrame));
}
