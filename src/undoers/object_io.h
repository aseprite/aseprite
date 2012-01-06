/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#ifndef UNDOERS_OBJECT_IO_H_INCLUDED
#define UNDOERS_OBJECT_IO_H_INCLUDED

#include "base/serialization.h"
#include "base/unique_ptr.h"

namespace undo {
  class ObjectsContainer;
}

namespace undoers {

// read_object and write_object functions can be used to serialize an
// object into a stream, and restore it back into the memory with the
// same ID which were assigned in the ObjectsContainer previously.

// Serializes the given object into the stream identifying it with an
// ID from the ObjectsContainer. When the object is deserialized with
// read_object, the object is added to the container with the same ID.
template<class T, class Writer>
void write_object(undo::ObjectsContainer* objects, std::ostream& os, T* object, Writer& writer)
{
  using base::serialization::little_endian::write32;

  // Get an ID for the image.
  undo::ObjectId objectId = objects->addObject(object);

  write32(os, objectId);        // Write the ID
  writer(os, object);           // Write the object

  // Remove the object from the container (it will be
  // re-added by a undoers::read_object call).
  objects->removeObject(objectId);
}

// Deserializes the given object from the stream, adding the object
// into the ObjectsContainer with the same ID saved with write_object().
template<class T, class Reader>
T* read_object(undo::ObjectsContainer* objects, std::istream& is, Reader& reader)
{
  using base::serialization::little_endian::read32;

  undo::ObjectId objectId = read32(is);    // Read the ID
  UniquePtr<T> object(reader(is));         // Read the object

  // Re-insert the object in the container with the read ID.
  objects->insertObject(objectId, object);
  return object.release();
}

} // namespace undoers

#endif  // UNDOERS_OBJECT_IO_H_INCLUDED
