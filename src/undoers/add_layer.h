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

#ifndef UNDOERS_ADD_LAYER_H_INCLUDED
#define UNDOERS_ADD_LAYER_H_INCLUDED

#include "undo/object_id.h"
#include "undoers/undoer_base.h"

class Layer;

namespace undoers {

class AddLayer : public UndoerBase
{
public:
  AddLayer(undo::ObjectsContainer* objects, Layer* folder, Layer* layer);

  void dispose() OVERRIDE;
  size_t getMemSize() const OVERRIDE { return sizeof(*this); }
  void revert(undo::ObjectsContainer* objects, undo::UndoersCollector* redoers) OVERRIDE;

private:
  undo::ObjectId m_folderId;
  undo::ObjectId m_layerId;
};

} // namespace undoers

#endif  // UNDOERS_ADD_LAYER_H_INCLUDED
