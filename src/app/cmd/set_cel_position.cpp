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

#include "app/cmd/set_cel_position.h"

#include "app/document.h"
#include "doc/cel.h"
#include "doc/document_event.h"

namespace app {
namespace cmd {

using namespace doc;

SetCelPosition::SetCelPosition(Cel* cel, int x, int y)
  : WithCel(cel)
  , m_oldX(cel->x())
  , m_oldY(cel->y())
  , m_newX(x)
  , m_newY(y)
{
}

void SetCelPosition::onExecute()
{
  cel()->setPosition(m_newX, m_newY);
}

void SetCelPosition::onUndo()
{
  cel()->setPosition(m_oldX, m_oldY);
}

void SetCelPosition::onFireNotifications()
{
  Cel* cel = this->cel();
  DocumentEvent ev(cel->document());
  ev.sprite(cel->sprite());
  ev.cel(cel);
  cel->document()->notifyObservers<DocumentEvent&>(&DocumentObserver::onCelPositionChanged, ev);
}

} // namespace cmd
} // namespace app
