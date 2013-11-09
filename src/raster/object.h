/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef RASTER_OBJECT_H_INCLUDED
#define RASTER_OBJECT_H_INCLUDED

#include <list>

namespace raster {

  enum ObjectType {
    OBJECT_CEL,
    OBJECT_IMAGE,
    OBJECT_LAYER_IMAGE,
    OBJECT_LAYER_FOLDER,
    OBJECT_MASK,
    OBJECT_PALETTE,
    OBJECT_PATH,
    OBJECT_SPRITE,
    OBJECT_STOCK,
    OBJECT_RGBMAP,
  };

  class Cel;
  class Layer;

  typedef std::list<Cel*> CelList;
  typedef std::list<Cel*>::iterator CelIterator;
  typedef std::list<Cel*>::const_iterator CelConstIterator;

  typedef std::list<Layer*> LayerList;
  typedef std::list<Layer*>::iterator LayerIterator;
  typedef std::list<Layer*>::const_iterator LayerConstIterator;

  class Object {
  public:
    Object(ObjectType type);
    Object(const Object& object);
    virtual ~Object();

    ObjectType type() const { return m_type; }

    // Returns the approximate amount of memory (in bytes) which this
    // object use.
    virtual int getMemSize() const;

  private:
    ObjectType m_type;

    Object& operator=(const Object&);
  };

} // namespace raster

#endif
