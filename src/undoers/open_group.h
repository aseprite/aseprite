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

#ifndef UNDOERS_OPEN_GROUP_H_INCLUDED
#define UNDOERS_OPEN_GROUP_H_INCLUDED

#include "base/compiler_specific.h"
#include "undo/undoer.h"

namespace undoers {

class OpenGroup : public undo::Undoer
{
public:
  void dispose() OVERRIDE;
  int getMemSize() const OVERRIDE { return sizeof(*this); }
  bool isOpenGroup() const OVERRIDE { return true; }
  bool isCloseGroup() const OVERRIDE { return false; }
  void revert(undo::ObjectsContainer* objects, undo::UndoersCollector* redoers) OVERRIDE;
};

} // namespace undoers

#endif	// UNDOERS_OPEN_GROUP_H_INCLUDED
