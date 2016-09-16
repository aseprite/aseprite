// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_position.h"

#include "app/document.h"
#include "doc/cel.h"
#include "doc/document_event.h"

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
  cel()->setPosition(m_newX, m_newY);
  cel()->incrementVersion();
}

void SetCelPosition::onUndo()
{
  cel()->setPosition(m_oldX, m_oldY);
  cel()->incrementVersion();
}

void SetCelPosition::onFireNotifications()
{
  Cel* cel = this->cel();
  DocumentEvent ev(cel->document());
  ev.sprite(cel->sprite());
  ev.cel(cel);
  cel->document()->notify_observers<DocumentEvent&>(&DocumentObserver::onCelPositionChanged, ev);
}

} // namespace cmd
} // namespace app
