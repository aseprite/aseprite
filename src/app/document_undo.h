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

#ifndef APP_DOCUMENT_UNDO_H_INCLUDED
#define APP_DOCUMENT_UNDO_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "raster/sprite_position.h"
#include "undo/undo_history.h"

namespace undo {
  class ObjectsContainer;
  class Undoer;
}

namespace app {
  namespace undoers {
    class CloseGroup;
  }

  using namespace raster;

  class DocumentUndo : public undo::UndoHistoryDelegate {
  public:
    DocumentUndo();

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool state) { m_enabled = state; }

    bool canUndo() const;
    bool canRedo() const;

    void doUndo();
    void doRedo();

    void clearRedo();

    bool isSavedState() const;
    void markSavedState();

    // UndoHistoryDelegate implementation.
    undo::ObjectsContainer* getObjects() const OVERRIDE { return m_objects; }
    size_t getUndoSizeLimit() const OVERRIDE;

    void pushUndoer(undo::Undoer* undoer);

    bool implantUndoerInLastGroup(undo::Undoer* undoer);

    const char* getNextUndoLabel() const;
    const char* getNextRedoLabel() const;

    SpritePosition getNextUndoSpritePosition() const;
    SpritePosition getNextRedoSpritePosition() const;

    undo::UndoersCollector* getDefaultUndoersCollector() {
      return m_undoHistory;
    }

  private:
    undoers::CloseGroup* getNextUndoGroup() const;
    undoers::CloseGroup* getNextRedoGroup() const;

    // Collection of objects used by UndoHistory to reference deleted
    // objects that are re-created by an Undoer. The container keeps an
    // ID that is saved in the serialization process, and loaded in the
    // deserialization process. The ID can be used by different undoers
    // to keep references to deleted objects.
    base::UniquePtr<undo::ObjectsContainer> m_objects;

    // Stack of undoers to undo operations.
    base::UniquePtr<undo::UndoHistory> m_undoHistory;

    bool m_enabled;

    DISABLE_COPYING(DocumentUndo);
  };

} // namespace app

#endif
