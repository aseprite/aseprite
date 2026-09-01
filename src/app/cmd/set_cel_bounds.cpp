// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_cel_bounds.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"

namespace app { namespace cmd {

using namespace doc;

SetCelBoundsF::SetCelBoundsF(Cel* cel, const gfx::RectF& bounds)
  : WithCel(cel)
  , m_oldBounds(cel->boundsF())
  , m_newBounds(bounds)
{
}

void SetCelBoundsF::onExecute()
{
  setBounds(m_newBounds);
}

void SetCelBoundsF::onUndo()
{
  setBounds(m_oldBounds);
}

void SetCelBoundsF::setBounds(const gfx::RectF& newBounds)
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);

  doc->notify_observers<DocEvent&>(&DocObserver::onBeforeCelPositionChange, ev);

  cel->setBoundsF(newBounds);
  cel->incrementVersion();

  doc->notify_observers<DocEvent&>(&DocObserver::onAfterCelPositionChange, ev);
}

}} // namespace app::cmd
