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

#include "config.h"

#include "app.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "undoable.h"
#include "widgets/editor/pixels_movement.h"

class PixelsMovementImpl
{
  SpriteWriter m_sprite_writer;
  Undoable m_undoable;
  int m_initial_x, m_initial_y;
  int m_catch_x, m_catch_y;
  bool m_firstDrop;
  bool m_isDragging;

public:
  PixelsMovementImpl(Sprite* sprite, const Image* moveThis, int initial_x, int initial_y, int opacity)
    : m_sprite_writer(sprite)
    , m_undoable(m_sprite_writer, "Pixels Movement")
    , m_initial_x(initial_x)
    , m_initial_y(initial_y)
    , m_firstDrop(true)
    , m_isDragging(false)
  {
    m_sprite_writer->prepareExtraCel(initial_x, initial_y, moveThis->w, moveThis->h, opacity);

    Image* extraImage = m_sprite_writer->getExtraCelImage();
    image_copy(extraImage, moveThis, 0, 0);
  }

  ~PixelsMovementImpl()
  {
  }

  void cutMask()
  {
    m_undoable.clearMask(app_get_color_to_clear_layer(m_sprite_writer->getCurrentLayer()));

    copyMask();
  }

  void copyMask()
  {
    // Hide the mask (do not deselect it, it will be moved them using m_undoable.setMaskPosition)
    Mask* empty_mask = new Mask();
    m_sprite_writer->generateMaskBoundaries(empty_mask);
    delete empty_mask;

    update_screen_for_sprite(m_sprite_writer);
  }

  void catchImage(int x, int y)
  {
    m_catch_x = x;
    m_catch_y = y;
    m_isDragging = true;
  }

  void catchImageAgain(int x, int y)
  {
    // Create a new Undoable to move the pixels to other position
    Cel* cel = m_sprite_writer->getExtraCel();
    m_initial_x = cel->x;
    m_initial_y = cel->y;
    m_isDragging = true;

    m_catch_x = x;
    m_catch_y = y;

    // Hide the mask (do not deselect it, it will be moved them using m_undoable.setMaskPosition)
    Mask* empty_mask = new Mask();
    m_sprite_writer->generateMaskBoundaries(empty_mask);
    delete empty_mask;

    update_screen_for_sprite(m_sprite_writer);
  }

  Rect moveImage(int x, int y)
  {
    Cel* cel = m_sprite_writer->getExtraCel();
    Image* image = m_sprite_writer->getExtraCelImage();
    int x1, y1, x2, y2;
    int u1, v1, u2, v2;

    x1 = cel->x;
    y1 = cel->y;
    x2 = cel->x + image->w;
    y2 = cel->y + image->h;

    int new_x = m_initial_x + x - m_catch_x;
    int new_y = m_initial_y + y - m_catch_y;

    // No movement
    if (cel->x == new_x && cel->y == new_y)
      return Rect();

    cel->x = new_x;
    cel->y = new_y;

    u1 = cel->x;
    v1 = cel->y;
    u2 = cel->x + image->w;
    v2 = cel->y + image->h;

    return Rect(MIN(x1, u1), MIN(y1, v1),
		MAX(x2, u2) - MIN(x1, u1) + 1,
		MAX(y2, v2) - MIN(y1, v1) + 1);
  }

  void dropImageTemporarily()
  {
    m_isDragging = false;

    Cel* cel = m_sprite_writer->getExtraCel();

    // Show the mask again in the new position
    if (m_firstDrop) {
      m_firstDrop = false;
      m_undoable.setMaskPosition(cel->x, cel->y);
    }
    else {
      m_sprite_writer->getMask()->x = cel->x;
      m_sprite_writer->getMask()->y = cel->y;
    }
    m_sprite_writer->generateMaskBoundaries();

    update_screen_for_sprite(m_sprite_writer);
  }

  void dropImage()
  {
    m_isDragging = false;

    Cel* cel = m_sprite_writer->getExtraCel();
    Image* image = m_sprite_writer->getExtraCelImage();

    m_undoable.pasteImage(image, cel->x, cel->y, cel->opacity);
    m_undoable.commit();
  }

  bool isDragging()
  {
    return m_isDragging;
  }

  Rect getImageBounds()
  {
    Cel* cel = m_sprite_writer->getExtraCel();
    Image* image = m_sprite_writer->getExtraCelImage();

    ASSERT(cel != NULL);
    ASSERT(image != NULL);

    return Rect(cel->x, cel->y, image->w, image->h);
  }

  void setMaskColor(ase_uint32 mask_color)
  {
    Image* image = m_sprite_writer->getExtraCelImage();

    ASSERT(image != NULL);

    image->mask_color = mask_color;

    update_screen_for_sprite(m_sprite_writer);
  }

};

//////////////////////////////////////////////////////////////////////
// PixelsMovement

PixelsMovement::PixelsMovement(Sprite* sprite, const Image* moveThis, int x, int y, int opacity)
{
  m_impl = new PixelsMovementImpl(sprite, moveThis, x, y, opacity);
}

PixelsMovement::~PixelsMovement()
{
  delete m_impl;
}

void PixelsMovement::cutMask()
{
  m_impl->cutMask();
}

void PixelsMovement::copyMask()
{
  m_impl->copyMask();
}

void PixelsMovement::catchImage(int x, int y)
{
  m_impl->catchImage(x, y);
}

void PixelsMovement::catchImageAgain(int x, int y)
{
  return m_impl->catchImageAgain(x, y);
}

Rect PixelsMovement::moveImage(int x, int y)
{
  return m_impl->moveImage(x, y);
}

void PixelsMovement::dropImageTemporarily()
{
  return m_impl->dropImageTemporarily();
}

void PixelsMovement::dropImage()
{
  m_impl->dropImage();
}

bool PixelsMovement::isDragging()
{
  return m_impl->isDragging();
}

Rect PixelsMovement::getImageBounds()
{
  return m_impl->getImageBounds();
}

void PixelsMovement::setMaskColor(ase_uint32 mask_color)
{
  m_impl->setMaskColor(mask_color);
}
