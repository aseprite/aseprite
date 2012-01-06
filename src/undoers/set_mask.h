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

#ifndef UNDOERS_SET_MASK_H_INCLUDED
#define UNDOERS_SET_MASK_H_INCLUDED

#include "undo/object_id.h"
#include "undoers/undoer_base.h"

#include <sstream>

class Document;

namespace undoers {

class SetMask : public UndoerBase
{
public:
  SetMask(undo::ObjectsContainer* objects, Document* document);

  void dispose() OVERRIDE;
  int getMemSize() const OVERRIDE { return sizeof(*this) + getStreamSize(); }
  void revert(undo::ObjectsContainer* objects, undo::UndoersCollector* redoers) OVERRIDE;

private:
  size_t getStreamSize() const {
    return const_cast<std::stringstream*>(&m_stream)->tellp();
  }

  undo::ObjectId m_documentId;
  bool m_isMaskVisible;
  std::stringstream m_stream;
};

} // namespace undoers

#endif  // UNDOERS_SET_MASK_H_INCLUDED
