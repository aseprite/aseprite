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

#include "app/cmd/set_sprite_size.h"

#include "app/document.h"
#include "doc/document_event.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

SetSpriteSize::SetSpriteSize(Sprite* sprite, int newWidth, int newHeight)
  : WithSprite(sprite)
  , m_oldWidth(sprite->width())
  , m_oldHeight(sprite->height())
  , m_newWidth(newWidth)
  , m_newHeight(newHeight)
{
  ASSERT(m_newWidth > 0);
  ASSERT(m_newHeight > 0);
}

void SetSpriteSize::onExecute()
{
  sprite()->setSize(m_newWidth, m_newHeight);
}

void SetSpriteSize::onUndo()
{
  sprite()->setSize(m_oldWidth, m_oldHeight);
}

void SetSpriteSize::onFireNotifications()
{
  Sprite* sprite = this->sprite();
  DocumentEvent ev(sprite->document());
  ev.sprite(sprite);
  sprite->document()->notifyObservers<DocumentEvent&>(&DocumentObserver::onSpriteSizeChanged, ev);
}

} // namespace cmd
} // namespace app
