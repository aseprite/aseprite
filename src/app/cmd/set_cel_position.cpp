// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_position.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"

namespace app {
namespace cmd {

using namespace doc;

SetCelPosition::SetCelPosition(Cel* cel, int x, int y)
  : WithCel(cel)
  , m_oldX(cel->x())
  , m_oldY(cel->y())
  , m_newX(x)
  , m_newY(y)
{
}

void SetCelPosition::onExecute()
{
  cel()->data()->setPosition(gfx::Point(m_newX, m_newY));
  cel()->data()->incrementVersion();
}

void SetCelPosition::onUndo()
{
  cel()->data()->setPosition(gfx::Point(m_oldX, m_oldY));
  cel()->data()->incrementVersion();
}

void SetCelPosition::onFireNotifications()
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onCelPositionChanged, ev);
}

} // namespace cmd
} // namespace app
