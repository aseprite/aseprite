// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_last_point.h"

#include "app/document.h"

namespace app {
namespace cmd {

SetLastPoint::SetLastPoint(Document* doc, const gfx::Point& pos)
  : WithDocument(doc)
  , m_oldPoint(doc->lastDrawingPoint())
  , m_newPoint(pos)
{
}

void SetLastPoint::onExecute()
{
  setLastPoint(m_newPoint);
}

void SetLastPoint::onUndo()
{
  setLastPoint(m_oldPoint);
}

void SetLastPoint::setLastPoint(const gfx::Point& pos)
{
  Document* doc = document();
  doc->setLastDrawingPoint(pos);
}

} // namespace cmd
} // namespace app
