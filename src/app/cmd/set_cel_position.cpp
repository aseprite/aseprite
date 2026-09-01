// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
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

namespace app { namespace cmd {

using namespace doc;

SetCelPosition::SetCelPosition(Cel* cel, const gfx::Point& newPosition)
  : WithCel(cel)
  , m_old(cel->position())
  , m_new(newPosition)
{
}

void SetCelPosition::onExecute()
{
  setPosition(m_new);
}

void SetCelPosition::onUndo()
{
  setPosition(m_old);
}

void SetCelPosition::setPosition(const gfx::Point& newPos)
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);

  doc->notify_observers<DocEvent&>(&DocObserver::onBeforeCelPositionChange, ev);

  cel->data()->setPosition(newPos);
  cel->data()->incrementVersion();

  doc->notify_observers<DocEvent&>(&DocObserver::onAfterCelPositionChange, ev);
}

}} // namespace app::cmd
