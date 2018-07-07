// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_bounds.h"

#include "app/document.h"
#include "doc/cel.h"
#include "doc/doc_event.h"

namespace app {
namespace cmd {

using namespace doc;

SetCelBoundsF::SetCelBoundsF(Cel* cel, const gfx::RectF& bounds)
  : WithCel(cel)
  , m_oldBounds(cel->boundsF())
  , m_newBounds(bounds)
{
}

void SetCelBoundsF::onExecute()
{
  cel()->setBoundsF(m_newBounds);
  cel()->incrementVersion();
}

void SetCelBoundsF::onUndo()
{
  cel()->setBoundsF(m_oldBounds);
  cel()->incrementVersion();
}

void SetCelBoundsF::onFireNotifications()
{
  Cel* cel = this->cel();
  DocEvent ev(cel->document());
  ev.sprite(cel->sprite());
  ev.cel(cel);
  cel->document()->notify_observers<DocEvent&>(&DocObserver::onCelPositionChanged, ev);
}

} // namespace cmd
} // namespace app
