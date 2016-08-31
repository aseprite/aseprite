// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/reselect_mask.h"

#include "app/cmd/set_mask.h"
#include "app/document.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

ReselectMask::ReselectMask(Document* doc)
  : WithDocument(doc)
{
}

void ReselectMask::onExecute()
{
  app::Document* doc = document();

  if (m_oldMask) {
    doc->setMask(m_oldMask);
    m_oldMask.reset();
  }

  doc->setMaskVisible(true);
}

void ReselectMask::onUndo()
{
  app::Document* doc = document();

  m_oldMask.reset(doc->isMaskVisible() ? new Mask(*doc->mask()): nullptr);

  doc->setMaskVisible(false);
}

size_t ReselectMask::onMemSize() const
{
  return sizeof(*this) + (m_oldMask ? m_oldMask->getMemSize(): 0);
}

} // namespace cmd
} // namespace app
