/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef CONTEXT_FLAGS_H_INCLUDED
#define CONTEXT_FLAGS_H_INCLUDED

class Context;

class ContextFlags
{
public:
  enum {
    HasActiveDocument           = 1 << 0,
    HasActiveSprite             = 1 << 1,
    HasVisibleMask              = 1 << 2,
    HasActiveLayer              = 1 << 3,
    HasActiveCel                = 1 << 4,
    HasActiveImage              = 1 << 5,
    HasBackgroundLayer          = 1 << 6,
    ActiveDocumentIsReadable    = 1 << 7,
    ActiveDocumentIsWritable    = 1 << 8,
    ActiveLayerIsImage          = 1 << 9,
    ActiveLayerIsBackground     = 1 << 10,
    ActiveLayerIsReadable       = 1 << 11,
    ActiveLayerIsWritable       = 1 << 12,
  };

  ContextFlags();

  bool check(uint32_t flags) const { return (m_flags & flags) == flags; }
  void update(Context* context);

private:
  uint32_t m_flags;
};

#endif
