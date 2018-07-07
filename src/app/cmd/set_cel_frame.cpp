// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_frame.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

SetCelFrame::SetCelFrame(Cel* cel, frame_t frame)
  : WithCel(cel)
  , m_oldFrame(cel->frame())
  , m_newFrame(frame)
{
}

void SetCelFrame::onExecute()
{
  Cel* cel = this->cel();
  cel->layer()->moveCel(cel, m_newFrame);
  cel->incrementVersion();
}

void SetCelFrame::onUndo()
{
  Cel* cel = this->cel();
  cel->layer()->moveCel(cel, m_oldFrame);
  cel->incrementVersion();
}

void SetCelFrame::onFireNotifications()
{
  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(cel->layer()->sprite());
  ev.layer(cel->layer());
  ev.cel(cel);
  ev.frame(cel->frame());
  doc->notify_observers<DocEvent&>(&DocObserver::onCelFrameChanged, ev);
}

} // namespace cmd
} // namespace app
