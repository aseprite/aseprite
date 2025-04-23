// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd_transaction.h"

#include "app/app.h"
#include "app/context.h"
#include "app/site.h"

namespace app {

CmdTransaction::CmdTransaction(const std::string& label, bool changeSavedState)
  : m_ranges(nullptr)
  , m_label(label)
  , m_changeSavedState(changeSavedState)
{
}

CmdTransaction* CmdTransaction::moveToEmptyCopy()
{
  CmdTransaction* copy = new CmdTransaction(m_label, m_changeSavedState);
  copy->m_spritePositionBefore = m_spritePositionBefore;
  copy->m_spritePositionAfter = m_spritePositionAfter;
  if (m_ranges) {
    copy->m_ranges.reset(new Ranges);
    copy->m_ranges->m_before = std::move(m_ranges->m_before);
    copy->m_ranges->m_after = std::move(m_ranges->m_after);
  }
  return copy;
}

void CmdTransaction::setNewDocRange(const view::RealRange& range)
{
  if (m_ranges)
    range.write(m_ranges->m_after);
}

void CmdTransaction::updateSpritePositionAfter()
{
  m_spritePositionAfter = calcSpritePosition();

  // We cannot capture m_ranges->m_after from the Timeline here
  // because the document range in the Timeline is updated after the
  // commit/command (on Timeline::onAfterCommandExecution).
  //
  // So m_ranges->m_after is captured explicitly in
  // setNewDocRange().
}

std::istream* CmdTransaction::documentRangeBeforeExecute() const
{
  if (m_ranges && m_ranges->m_before.tellp() > 0) {
    m_ranges->m_before.seekg(0);
    return &m_ranges->m_before;
  }
  else
    return nullptr;
}

std::istream* CmdTransaction::documentRangeAfterExecute() const
{
  if (m_ranges && m_ranges->m_after.tellp() > 0) {
    m_ranges->m_after.seekg(0);
    return &m_ranges->m_after;
  }
  else
    return nullptr;
}

void CmdTransaction::onExecute()
{
  // Save the current site and doc range
  m_spritePositionBefore = calcSpritePosition();
  if (isDocRangeEnabled()) {
    m_ranges.reset(new Ranges);
    calcDocRange().write(m_ranges->m_before);
  }

  // Execute the sequence of "cmds"
  CmdSequence::onExecute();
}

void CmdTransaction::onUndo()
{
  CmdSequence::onUndo();
}

void CmdTransaction::onRedo()
{
  CmdSequence::onRedo();
}

std::string CmdTransaction::onLabel() const
{
  return m_label;
}

size_t CmdTransaction::onMemSize() const
{
  size_t size = CmdSequence::onMemSize();
  if (m_ranges) {
    size += (m_ranges->m_before.tellp() + m_ranges->m_after.tellp());
  }
  return size;
}

SpritePosition CmdTransaction::calcSpritePosition() const
{
  // This check was added to allow executing transactions on documents that are
  // not part of any context. For instance, when dragging and dropping a
  // document on the timeline, the dragged document doesn't have any context (
  // it is not associated with any editor).
  if (!context())
    return SpritePosition();
  Site site = context()->activeSite();
  return SpritePosition(site.layer(), site.frame());
}

bool CmdTransaction::isDocRangeEnabled() const
{
  return (context() ? context()->range().enabled() : false);
}

view::RealRange CmdTransaction::calcDocRange() const
{
  return (context() ? context()->range() : view::RealRange());
}

} // namespace app
