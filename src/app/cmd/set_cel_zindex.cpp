// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_zindex.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"

namespace app {
namespace cmd {

using namespace doc;

SetCelZIndex::SetCelZIndex(Cel* cel, int zindex)
  : WithCel(cel)
  , m_oldZIndex(cel->zIndex())
  , m_newZIndex(zindex)
{
}

void SetCelZIndex::onExecute()
{
  cel()->setZIndex(m_newZIndex);
  cel()->incrementVersion();
}

void SetCelZIndex::onUndo()
{
  cel()->setZIndex(m_oldZIndex);
  cel()->incrementVersion();
}

void SetCelZIndex::onFireNotifications()
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onCelZIndexChange, ev);
}

} // namespace cmd
} // namespace app
