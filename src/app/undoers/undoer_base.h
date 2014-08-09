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

#ifndef APP_UNDOERS_UNDOER_BASE_H_INCLUDED
#define APP_UNDOERS_UNDOER_BASE_H_INCLUDED
#pragma once

#include "base/override.h"
#include "undo/undoer.h"

namespace app {
  namespace undoers {

    // Helper class to make new Undoers, derive from here and implement
    // revert(), getMemSize(), and dispose() methods only.
    class UndoerBase : public undo::Undoer {
    public:
      undo::Modification getModification() const OVERRIDE { return undo::DoesntModifyDocument; }
      bool isOpenGroup() const OVERRIDE { return false; }
      bool isCloseGroup() const OVERRIDE { return false; }
    };

  } // namespace undoers
} // namespace app

#endif  // UNDOERS_UNDOER_BASE_H_INCLUDED
