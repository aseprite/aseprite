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

#include "config.h"

#include "document_undo.h"

#include "objects_container_impl.h"
#include "undo/undo_history.h"
#include "undoers/close_group.h"

#include <allegro/config.h>     // TODO remove this when get_config_int() is removed from here
#include <cassert>
#include <stdexcept>

DocumentUndo::DocumentUndo()
  : m_objects(new ObjectsContainerImpl)
  , m_undoHistory(new undo::UndoHistory(this))
  , m_enabled(true)
{
}

bool DocumentUndo::canUndo() const
{
  return m_undoHistory->canUndo();
}

bool DocumentUndo::canRedo() const
{
  return m_undoHistory->canRedo();
}

void DocumentUndo::doUndo()
{
  return m_undoHistory->doUndo();
}

void DocumentUndo::doRedo()
{
  return m_undoHistory->doRedo();
}

void DocumentUndo::clearRedo()
{
  return m_undoHistory->clearRedo();
}

bool DocumentUndo::isSavedState() const
{
  return m_undoHistory->isSavedState();
}

void DocumentUndo::markSavedState()
{
  return m_undoHistory->markSavedState();
}

void DocumentUndo::pushUndoer(undo::Undoer* undoer)
{
  return m_undoHistory->pushUndoer(undoer);
}

bool DocumentUndo::implantUndoerInLastGroup(undo::Undoer* undoer)
{
  return m_undoHistory->implantUndoerInLastGroup(undoer);
}

size_t DocumentUndo::getUndoSizeLimit() const
{
  return ((size_t)get_config_int("Options", "UndoSizeLimit", 8))*1024*1024;
}

const char* DocumentUndo::getNextUndoLabel() const
{
  return getNextUndoGroup()->getLabel();
}

const char* DocumentUndo::getNextRedoLabel() const
{
  return getNextRedoGroup()->getLabel();
}

SpritePosition DocumentUndo::getNextUndoSpritePosition() const
{
  return getNextUndoGroup()->getSpritePosition();
}

SpritePosition DocumentUndo::getNextRedoSpritePosition() const
{
  return getNextRedoGroup()->getSpritePosition();
}

undoers::CloseGroup* DocumentUndo::getNextUndoGroup() const
{
  undo::Undoer* undoer = m_undoHistory->getNextUndoer();
  if (!undoer)
    throw std::logic_error("There are some action without label");

  if (undoers::CloseGroup* closeGroup = dynamic_cast<undoers::CloseGroup*>(undoer))
    return closeGroup;
  else
    throw std::logic_error("There are some action without a CloseGroup");
}

undoers::CloseGroup* DocumentUndo::getNextRedoGroup() const
{
  undo::Undoer* undoer = m_undoHistory->getNextRedoer();
  if (!undoer)
    throw std::logic_error("There are some action without label");

  if (undoers::CloseGroup* closeGroup = dynamic_cast<undoers::CloseGroup*>(undoer))
    return closeGroup;
  else
    throw std::logic_error("There are some action without a CloseGroup");
}
