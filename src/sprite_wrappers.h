/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef SPRITE_WRAPPERS_H_INCLUDED
#define SPRITE_WRAPPERS_H_INCLUDED

#include <list>
#include <exception>
#include "ase_exception.h"
#include "context.h"
#include "raster/sprite.h"

class LockedSpriteException : public ase_exception
{
public:
  LockedSpriteException() throw()
  : ase_exception("Cannot read/write the sprite.\n"
		  "The sprite is locked by a background task.\n"
		  "Try again later.") { }
};

//////////////////////////////////////////////////////////////////////

/**
 * Wraps a sprite.
 */
class SpriteWrapper
{
protected:
  Sprite* m_sprite;

public:
  SpriteWrapper() : m_sprite(NULL) { }
  SpriteWrapper(const SpriteWrapper& copy) : m_sprite(copy.m_sprite) { }
  explicit SpriteWrapper(Sprite* sprite) : m_sprite(sprite) { }
  ~SpriteWrapper() { }

  SpriteWrapper& operator=(const SpriteWrapper& copy) 
  {
    m_sprite = copy.m_sprite;
    return *this;
  }

  operator Sprite* () { return m_sprite; }
  operator const Sprite* () const { return m_sprite; }

  Sprite* operator->() 
  {
    ASSERT(m_sprite != NULL);
    return m_sprite;
  }

  const Sprite* operator->() const 
  {
    ASSERT(m_sprite != NULL);
    return m_sprite;
  }

};

/**
 * Class to view the sprite's state.
 */
class SpriteReader : public SpriteWrapper
{
public:

  SpriteReader()
  {
  }

  explicit SpriteReader(Sprite* sprite)
    : SpriteWrapper(sprite)
  {
    if (m_sprite && !m_sprite->lock(false))
      throw LockedSpriteException();
  }

  explicit SpriteReader(const SpriteReader& copy)
    : SpriteWrapper(copy)
  {
    if (m_sprite && !m_sprite->lock(false))
      throw LockedSpriteException();
  }

  SpriteReader& operator=(const SpriteReader& copy)
  {
    // unlock old sprite
    if (m_sprite)
      m_sprite->unlock();

    SpriteWrapper::operator=(copy);

    // relock the sprite
    if (m_sprite && !m_sprite->lock(false))
      throw LockedSpriteException();

    return *this;
  }

  ~SpriteReader()
  {
    // unlock the sprite
    if (m_sprite)
      m_sprite->unlock();
  }

};

/**
 * Class to modify the sprite's state.
 */
class SpriteWriter : public SpriteWrapper
{
  bool m_from_reader;
  bool m_locked;

  // Non-copyable
  SpriteWriter(const SpriteWriter&);
  SpriteWriter& operator=(const SpriteWriter&);

public:

  SpriteWriter()
    : m_from_reader(false)
    , m_locked(false)
  {
  }

  explicit SpriteWriter(Sprite* sprite)
    : SpriteWrapper(sprite)
    , m_from_reader(false)
    , m_locked(false)
  {
    if (m_sprite) {
      if (!m_sprite->lock(true))
	throw LockedSpriteException();

      m_locked = true;
    }
  }

  explicit SpriteWriter(const SpriteReader& sprite)
    : SpriteWrapper(sprite)
    , m_from_reader(true)
    , m_locked(false)
  {
    if (m_sprite) {
      if (!m_sprite->lockToWrite())
	throw LockedSpriteException();

      m_locked = true;
    }
  }

  ~SpriteWriter() 
  {
    unlock_writer();
  }

  SpriteWriter& operator=(const SpriteReader& copy)
  {
    unlock_writer();

    SpriteWrapper::operator=(copy);

    m_locked = false;
    if (m_sprite) {
      m_from_reader = true;

      if (!m_sprite->lockToWrite())
	throw LockedSpriteException();

      m_locked = true;
    }

    return *this;
  }

protected:

  void unlock_writer()
  {
    if (m_sprite && m_locked) {
      if (m_from_reader)
	m_sprite->unlockToRead();
      else
	m_sprite->unlock();
    }
  }

};

class CurrentSpriteReader : public SpriteReader
{
public:

  CurrentSpriteReader(Context* context)
    : SpriteReader(context->get_current_sprite())
  {
  }

  ~CurrentSpriteReader() 
  {
  }

};

class CurrentSpriteWriter : public SpriteWriter
{
  Context* m_context;

public:

  CurrentSpriteWriter(Context* context)
    : SpriteWriter(context->get_current_sprite())
    , m_context(context)
  {
  }

  ~CurrentSpriteWriter() 
  {
  }

  void destroy()
  {
    ASSERT(m_sprite != NULL);

    m_context->remove_sprite(m_sprite);
    unlock_writer();

    delete m_sprite;
    m_sprite = NULL;
  }

};

#endif
