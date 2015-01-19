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

#include "app/cmd/set_transparent_color.h"

#include "app/document.h"
#include "doc/document_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetTransparentColor::SetTransparentColor(Sprite* sprite, color_t newMask)
  : WithSprite(sprite)
  , m_oldMaskColor(sprite->transparentColor())
  , m_newMaskColor(newMask)
{
}

void SetTransparentColor::onExecute()
{
  sprite()->setTransparentColor(m_newMaskColor);
}

void SetTransparentColor::onUndo()
{
  sprite()->setTransparentColor(m_oldMaskColor);
}

void SetTransparentColor::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  DocumentEvent ev(sprite->document());
  ev.sprite(sprite);
  sprite->document()->notifyObservers<DocumentEvent&>(&DocumentObserver::onSpriteTransparentColorChanged, ev);
}

} // namespace cmd
} // namespace app
