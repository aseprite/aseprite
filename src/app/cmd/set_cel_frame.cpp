/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_frame.h"

#include "doc/cel.h"
#include "doc/document.h"
#include "doc/document_event.h"
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
}

void SetCelFrame::onUndo()
{
  Cel* cel = this->cel();
  cel->layer()->moveCel(cel, m_oldFrame);
}

void SetCelFrame::onFireNotifications()
{
  Cel* cel = this->cel();
  doc::Document* doc = cel->sprite()->document();
  DocumentEvent ev(doc);
  ev.sprite(cel->layer()->sprite());
  ev.layer(cel->layer());
  ev.cel(cel);
  ev.frame(cel->frame());
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onCelFrameChanged, ev);
}

} // namespace cmd
} // namespace app
