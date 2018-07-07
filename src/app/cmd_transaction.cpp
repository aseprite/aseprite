// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd_transaction.h"

#include "app/context.h"
#include "app/site.h"

#ifdef ENABLE_UI
#include "app/app.h"
#include "app/ui/timeline/timeline.h"
#endif

namespace app {

CmdTransaction::CmdTransaction(const std::string& label,
                               bool changeSavedState,
                               int* savedCounter)
  : m_ranges(nullptr)
  , m_label(label)
  , m_changeSavedState(changeSavedState)
  , m_savedCounter(savedCounter)
{
}

void CmdTransaction::setNewDocRange(const DocRange& range)
{
#ifdef ENABLE_UI
  if (m_ranges)
    range.write(m_ranges->m_after);
#endif
}

void CmdTransaction::commit()
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
  CmdSequence::onExecute();

  // The execution of CmdTransaction is called by Transaction at the
  // very beginning, just to save the current sprite position.
  m_spritePositionBefore = calcSpritePosition();
#ifdef ENABLE_UI
  if (isDocRangeEnabled()) {
    m_ranges.reset(new Ranges);
    calcDocRange().write(m_ranges->m_before);
  }
#endif

  if (m_changeSavedState)
    ++(*m_savedCounter);
}

void CmdTransaction::onUndo()
{
  CmdSequence::onUndo();

  if (m_changeSavedState)
    --(*m_savedCounter);
}

void CmdTransaction::onRedo()
{
  CmdSequence::onRedo();

  if (m_changeSavedState)
    ++(*m_savedCounter);
}

std::string CmdTransaction::onLabel() const
{
  return m_label;
}

size_t CmdTransaction::onMemSize() const
{
  size_t size = CmdSequence::onMemSize();
  if (m_ranges) {
    size += (m_ranges->m_before.tellp() +
             m_ranges->m_after.tellp());
  }
  return size;
}

SpritePosition CmdTransaction::calcSpritePosition() const
{
  Site site = context()->activeSite();
  return SpritePosition(site.layer(), site.frame());
}

bool CmdTransaction::isDocRangeEnabled() const
{
#ifdef ENABLE_UI
  if (App::instance()) {
    Timeline* timeline = App::instance()->timeline();
    if (timeline && timeline->range().enabled())
      return true;
  }
#endif
  return false;
}

DocRange CmdTransaction::calcDocRange() const
{
#ifdef ENABLE_UI
  // TODO We cannot use Context::activeSite() because it losts
  //      important information about the DocRange() (type and
  //      flags).
  if (App::instance()) {
    Timeline* timeline = App::instance()->timeline();
    if (timeline)
      return timeline->range();
  }
#endif
  return DocRange();
}

} // namespace app
