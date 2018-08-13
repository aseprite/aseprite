// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/deselect_mask.h"

#include "app/cmd/set_mask.h"
#include "app/doc.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

DeselectMask::DeselectMask(Doc* doc)
  : WithDocument(doc)
{
}

void DeselectMask::onExecute()
{
  Doc* doc = document();
  m_oldMask.reset(doc->isMaskVisible() ? new Mask(*doc->mask()): nullptr);
  doc->setMaskVisible(false);
}

void DeselectMask::onUndo()
{
  Doc* doc = document();

  doc->setMask(m_oldMask.get());
  doc->setMaskVisible(true);

  m_oldMask.reset();
}

size_t DeselectMask::onMemSize() const
{
  return sizeof(*this) + (m_oldMask ? m_oldMask->getMemSize(): 0);
}

} // namespace cmd
} // namespace app
