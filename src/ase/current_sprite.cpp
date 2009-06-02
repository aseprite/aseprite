/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "config.h"

#include "ase/context.h"
#include "ase/current_sprite.h"
#include "ase/ui_context.h"	// TODO remove this line
#include "raster/sprite.h"

CurrentSprite::CurrentSprite()
{
  m_context = UIContext::instance();
  m_sprite = m_context->get_current_sprite();
  if (m_sprite)
    m_writeable = m_sprite->lock();
}

CurrentSprite::CurrentSprite(Context* context)
{
  assert(context != NULL);

  m_context = context;
  m_sprite = m_context->get_current_sprite();
  if (m_sprite)
    m_writeable = m_sprite->lock();
}

CurrentSprite::~CurrentSprite()
{
  if (m_sprite)
    m_sprite->unlock();
}

void CurrentSprite::destroy()
{
  if (m_sprite) {
    m_context->remove_sprite(m_sprite);
    m_sprite->unlock();

    delete m_sprite;
    m_sprite = NULL;
    m_writeable = false;
  }
}
