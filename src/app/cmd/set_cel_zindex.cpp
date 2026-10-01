// Aseprite
// Copyright (C) 2023-present  Igara Studio S.A.
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

namespace app { namespace cmd {

using namespace doc;

SetCelZIndex::SetCelZIndex(Cel* cel, const int zindex) : WithCel(cel), m_value(zindex)
{
}

void SetCelZIndex::onExecute(Context* ctx)
{
  swap();
}

void SetCelZIndex::onUndo(Context* ctx)
{
  swap();
}

void SetCelZIndex::onFireNotifications(Context* ctx)
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onCelZIndexChange, ev);
}

void SetCelZIndex::onSerialize(CmdSerial& s)
{
  Cmd::onSerialize(s);
  serializeCelId(s);
  s(m_value);
}

void SetCelZIndex::swap()
{
  Cel* cel = this->cel();
  auto current = cel->zIndex();
  std::swap(current, m_value);
  cel->setZIndex(current);
  cel->incrementVersion();
}

}} // namespace app::cmd
