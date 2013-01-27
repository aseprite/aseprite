/* ASEPRITE
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

#ifndef UNDOERS_IMAGE_AREA_H_INCLUDED
#define UNDOERS_IMAGE_AREA_H_INCLUDED

#include "undo/object_id.h"
#include "undoers/undoer_base.h"

#include <vector>

class Image;

namespace undoers {

class ImageArea : public UndoerBase
{
public:
  ImageArea(undo::ObjectsContainer* objects, Image* image, int x, int y, int w, int h);

  void dispose() OVERRIDE;
  size_t getMemSize() const OVERRIDE { return sizeof(*this) + m_data.size(); }
  void revert(undo::ObjectsContainer* objects, undo::UndoersCollector* redoers) OVERRIDE;

private:
  undo::ObjectId m_imageId;
  uint8_t m_format;
  uint16_t m_x, m_y, m_w, m_h;
  uint32_t m_lineSize;
  std::vector<uint8_t> m_data;
};

} // namespace undoers

#endif  // UNDOERS_IMAGE_AREA_H_INCLUDED
