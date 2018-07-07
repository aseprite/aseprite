// Aseprite
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

namespace app {
namespace cmd {

using namespace doc;

SetCelOpacity::SetCelOpacity(Cel* cel, int opacity)
  : WithCel(cel)
  , m_oldOpacity(cel->opacity())
  , m_newOpacity(opacity)
{
}

void SetCelOpacity::onExecute()
{
  cel()->setOpacity(m_newOpacity);
  cel()->data()->incrementVersion();
}

void SetCelOpacity::onUndo()
{
  cel()->setOpacity(m_oldOpacity);
  cel()->data()->incrementVersion();
}

void SetCelOpacity::onFireNotifications()
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(cel->sprite());
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onCelOpacityChange, ev);
}

} // namespace cmd
} // namespace app
