// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_cel_opacity.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"

namespace app { namespace cmd {

using namespace doc;

SetCelOpacity::SetCelOpacity(Cel* cel, const int opacity) : WithCel(cel), m_value(opacity)
{
}

void SetCelOpacity::onExecute(Context* ctx)
{
  swap();
}

void SetCelOpacity::onUndo(Context* ctx)
{
  swap();
}

void SetCelOpacity::onSerialize(CmdSerial& s)
{
  Cmd::onSerialize(s);
  serializeCelId(s);
  s(m_value);
}

void SetCelOpacity::swap()
{
  Cel* cel = this->cel();

  auto current = cel->opacity();
  std::swap(current, m_value);
  cel->setOpacity(current);
  cel->incrementVersion();

  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onCelOpacityChange, ev);
}

}} // namespace app::cmd
