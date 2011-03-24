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

#include "config.h"

#include "app.h"
#include "document.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undo_transaction.h"
#include "widgets/editor/pixels_movement.h"

using namespace gfx;

class PixelsMovementImpl
{
  const DocumentReader m_documentReader;
  Sprite* m_sprite;
  UndoTransaction m_undoTransaction;
  int m_initial_x, m_initial_y;
  int m_catch_x, m_catch_y;
  bool m_firstDrop;
  bool m_isDragging;

public:
  PixelsMovementImpl(Document* document, Sprite* sprite, const Image* moveThis, int initial_x, int initial_y, int opacity)
    : m_documentReader(document)
    , m_sprite(sprite)
    , m_undoTransaction(document, "Pixels Movement")
    , m_initial_x(initial_x)
    , m_initial_y(initial_y)
    , m_firstDrop(true)
    , m_isDragging(false)
  {
    DocumentWriter documentWriter(m_documentReader);
    documentWriter->prepareExtraCel(initial_x, initial_y, moveThis->w, moveThis->h, opacity);

    Image* extraImage = documentWriter->getExtraCelImage();
    image_copy(extraImage, moveThis, 0, 0);
  }

  ~PixelsMovementImpl()
  {
  }

  void cutMask()
  {
    {
      DocumentWriter documentWriter(m_documentReader);
      m_undoTransaction.clearMask(app_get_color_to_clear_layer(m_sprite->getCurrentLayer()));
    }

    copyMask();
  }

  void copyMask()
  {
    // Hide the mask (do not deselect it, it will be moved them using m_undoTransaction.setMaskPosition)
    Mask emptyMask;
    {
      DocumentWriter documentWriter(m_documentReader);
      documentWriter->generateMaskBoundaries(&emptyMask);
    }

    update_screen_for_document(m_documentReader);
  }

  void catchImage(int x, int y)
  {
    m_catch_x = x;
    m_catch_y = y;
    m_isDragging = true;
  }

  void catchImageAgain(int x, int y)
  {
    // Create a new UndoTransaction to move the pixels to other position
    const Cel* cel = m_documentReader->getExtraCel();
    m_initial_x = cel->x;
    m_initial_y = cel->y;
    m_isDragging = true;

    m_catch_x = x;
    m_catch_y = y;

    // Hide the mask (do not deselect it, it will be moved them using m_undoTransaction.setMaskPosition)
    Mask emptyMask;
    {
      DocumentWriter documentWriter(m_documentReader);
      documentWriter->generateMaskBoundaries(&emptyMask);
    }

    update_screen_for_document(m_documentReader);
  }

  Rect moveImage(int x, int y)
  {
    DocumentWriter documentWriter(m_documentReader);
    Cel* cel = documentWriter->getExtraCel();
    Image* image = documentWriter->getExtraCelImage();
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

    const Cel* cel = m_documentReader->getExtraCel();

    {
      DocumentWriter documentWriter(m_documentReader);

      // Show the mask again in the new position
      if (m_firstDrop) {
	m_firstDrop = false;
	m_undoTransaction.setMaskPosition(cel->x, cel->y);
      }
      else {
	documentWriter->getMask()->x = cel->x;
	documentWriter->getMask()->y = cel->y;
      }
      documentWriter->generateMaskBoundaries();
    }

    update_screen_for_document(m_documentReader);
  }

  void dropImage()
  {
    m_isDragging = false;

    const Cel* cel = m_documentReader->getExtraCel();
    const Image* image = m_documentReader->getExtraCelImage();

    {
      DocumentWriter documentWriter(m_documentReader);
      m_undoTransaction.pasteImage(image, cel->x, cel->y, cel->opacity);
      m_undoTransaction.commit();
    }
  }

  bool isDragging()
  {
    return m_isDragging;
  }

  Rect getImageBounds()
  {
    const Cel* cel = m_documentReader->getExtraCel();
    const Image* image = m_documentReader->getExtraCelImage();

    ASSERT(cel != NULL);
    ASSERT(image != NULL);

    return Rect(cel->x, cel->y, image->w, image->h);
  }

  void setMaskColor(uint32_t mask_color)
  {
    {
      DocumentWriter documentWriter(m_documentReader);
      Image* image = documentWriter->getExtraCelImage();

      ASSERT(image != NULL);

      image->mask_color = mask_color;
    }

    update_screen_for_document(m_documentReader);
  }

};

//////////////////////////////////////////////////////////////////////
// PixelsMovement

PixelsMovement::PixelsMovement(Document* document, Sprite* sprite, const Image* moveThis, int x, int y, int opacity)
{
  m_impl = new PixelsMovementImpl(document, sprite, moveThis, x, y, opacity);
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

void PixelsMovement::setMaskColor(uint32_t mask_color)
{
  m_impl->setMaskColor(mask_color);
}
