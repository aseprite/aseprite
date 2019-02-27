// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/reselect_mask.h"

#include "app/cmd/set_mask.h"
#include "app/doc.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

ReselectMask::ReselectMask(Doc* doc)
  : WithDocument(doc)
{
}

void ReselectMask::onExecute()
{
  Doc* doc = document();

  if (m_oldMask) {
    doc->setMask(m_oldMask.get());
    m_oldMask.reset();
  }

  doc->setMaskVisible(true);
  doc->notifySelectionChanged();
}

void ReselectMask::onUndo()
{
  Doc* doc = document();

  m_oldMask.reset(doc->isMaskVisible() ? new Mask(*doc->mask()): nullptr);

  doc->setMaskVisible(false);
  doc->notifySelectionChanged();
}

size_t ReselectMask::onMemSize() const
{
  return sizeof(*this) + (m_oldMask ? m_oldMask->getMemSize(): 0);
}

} // namespace cmd
} // namespace app
