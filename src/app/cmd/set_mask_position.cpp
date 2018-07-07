// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_mask_position.h"

#include "app/doc.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

SetMaskPosition::SetMaskPosition(Doc* doc, const gfx::Point& pos)
  : WithDocument(doc)
  , m_oldPosition(doc->mask()->bounds().origin())
  , m_newPosition(pos)
{
}

void SetMaskPosition::onExecute()
{
  setMaskPosition(m_newPosition);
}

void SetMaskPosition::onUndo()
{
  setMaskPosition(m_oldPosition);
}

void SetMaskPosition::setMaskPosition(const gfx::Point& pos)
{
  Doc* doc = document();
  doc->mask()->setOrigin(pos.x, pos.y);
  doc->resetTransformation();
}

} // namespace cmd
} // namespace app
