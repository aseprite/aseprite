// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_opacity.h"

#include "app/document.h"
#include "doc/cel.h"
#include "doc/document_event.h"

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
  DocumentEvent ev(cel->document());
  ev.sprite(cel->sprite());
  ev.cel(cel);
  cel->document()->notify_observers<DocumentEvent&>(&DocumentObserver::onCelOpacityChange, ev);
}

} // namespace cmd
} // namespace app
