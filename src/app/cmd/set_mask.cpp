// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask.h"

#include "app/doc.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

SetMask::SetMask(Doc* doc, const Mask* newMask)
  : WithDocument(doc)
  , m_oldMask(doc->isMaskVisible() ? new Mask(*doc->mask()): nullptr)
  , m_newMask(newMask && !newMask->isEmpty() ? new Mask(*newMask): nullptr)
{
}

void SetMask::setNewMask(const Mask* newMask)
{
  m_newMask.reset(newMask ? new Mask(*newMask): nullptr);
  setMask(m_newMask.get());
}

void SetMask::onExecute()
{
  setMask(m_newMask.get());
}

void SetMask::onUndo()
{
  setMask(m_oldMask.get());
}

size_t SetMask::onMemSize() const
{
  return sizeof(*this) +
    (m_oldMask ? m_oldMask->getMemSize(): 0) +
    (m_newMask ? m_newMask->getMemSize(): 0);
}

void SetMask::setMask(const Mask* mask)
{
  Doc* doc = document();

  if (mask) {
    doc->setMask(mask);
    doc->setMaskVisible(!mask->isEmpty());
  }
  else {
    Mask empty;
    doc->setMask(&empty);
    doc->setMaskVisible(false);
  }

  doc->notifySelectionChanged();
}

} // namespace cmd
} // namespace app
