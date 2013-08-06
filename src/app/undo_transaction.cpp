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

#include "app/undo_transaction.h"

#include "app/context_access.h"
#include "app/document.h"
#include "app/document_undo.h"
#include "app/undoers/close_group.h"
#include "app/undoers/open_group.h"
#include "raster/sprite.h"
#include "undo/undo_history.h"

namespace app {

UndoTransaction::UndoTransaction(Context* context, const char* label, undo::Modification modification)
  : m_context(context)
  , m_label(label)
  , m_modification(modification)
{
  ASSERT(label != NULL);

  DocumentLocation location = m_context->getActiveLocation();

  m_document = location.document();
  m_sprite = location.sprite();
  m_undo = m_document->getUndo();
  m_closed = false;
  m_committed = false;
  m_enabledFlag = m_undo->isEnabled();

  if (isEnabled()) {
    SpritePosition position(m_sprite->layerToIndex(location.layer()),
                            location.frame());
    m_undo->pushUndoer(new undoers::OpenGroup(getObjects(),
                                              m_label,
                                              m_modification,
                                              m_sprite,
                                              position));
  }
}

UndoTransaction::~UndoTransaction()
{
  try {
    if (isEnabled()) {
      // If it isn't committed, we have to rollback all changes.
      if (!m_committed && !m_closed)
        rollback();

      ASSERT(m_closed);
    }
  }
  catch (...) {
    // Just avoid throwing an exception in the dtor (just in case
    // rollback() failed).

    // TODO logging error
  }
}

void UndoTransaction::closeUndoGroup()
{
  ASSERT(!m_closed);

  if (isEnabled()) {
    DocumentLocation location = m_context->getActiveLocation();
    SpritePosition position(m_sprite->layerToIndex(location.layer()),
                            location.frame());

    // Close the undo information.
    m_undo->pushUndoer(new undoers::CloseGroup(getObjects(),
                                               m_label,
                                               m_modification,
                                               m_sprite,
                                               position));
    m_closed = true;
  }
}

void UndoTransaction::commit()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  if (isEnabled()) {
    closeUndoGroup();

    m_committed = true;
  }
}

void UndoTransaction::rollback()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  if (isEnabled()) {
    closeUndoGroup();

    // Undo the group of operations.
    m_undo->doUndo();

    // Clear the redo (sorry to the user, here we lost the old redo
    // information).
    m_undo->clearRedo();
  }
}

void UndoTransaction::pushUndoer(undo::Undoer* undoer)
{
  m_undo->pushUndoer(undoer);
}

undo::ObjectsContainer* UndoTransaction::getObjects() const
{
  return m_undo->getObjects();
}

} // namespace app
