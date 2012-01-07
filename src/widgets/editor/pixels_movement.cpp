/* ASEPRITE
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

#include "config.h"

#include "widgets/editor/pixels_movement.h"

#include "app.h"
#include "document.h"
#include "la/vector2d.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/rotate.h"
#include "raster/sprite.h"
#include "ui_context.h"
#include "util/expand_cel_canvas.h"

template<typename T>
static inline const la::Vector2d<double> point2Vector(const gfx::PointT<T>& pt) {
  return la::Vector2d<double>(pt.x, pt.y);
}

PixelsMovement::PixelsMovement(Document* document, Sprite* sprite, const Image* moveThis, int initialX, int initialY, int opacity,
                               const char* operationName)
  : m_documentReader(document)
  , m_sprite(sprite)
  , m_undoTransaction(document, operationName)
  , m_firstDrop(true)
  , m_isDragging(false)
  , m_adjustPivot(false)
  , m_handle(NoHandle)
  , m_originalImage(image_new_copy(moveThis))
{
  m_initialData = gfx::Transformation(gfx::Rect(initialX, initialY, moveThis->w, moveThis->h));
  m_currentData = m_initialData;

  DocumentWriter documentWriter(m_documentReader);
  documentWriter->prepareExtraCel(0, 0, m_sprite->getWidth(), m_sprite->getHeight(), opacity);

  Image* extraImage = documentWriter->getExtraCelImage();
  image_clear(extraImage, extraImage->mask_color);
  image_copy(extraImage, moveThis, initialX, initialY);

  m_initialMask = new Mask(*documentWriter->getMask());
  m_currentMask = new Mask(*documentWriter->getMask());
}

PixelsMovement::~PixelsMovement()
{
  delete m_originalImage;
  delete m_initialMask;
  delete m_currentMask;
}

void PixelsMovement::cutMask()
{
  {
    DocumentWriter documentWriter(m_documentReader);
    m_undoTransaction.clearMask(app_get_color_to_clear_layer(m_sprite->getCurrentLayer()));
  }

  copyMask();
}

void PixelsMovement::copyMask()
{
  // Hide the mask (do not deselect it, it will be moved them using m_undoTransaction.setMaskPosition)
  Mask emptyMask;
  {
    DocumentWriter documentWriter(m_documentReader);
    documentWriter->generateMaskBoundaries(&emptyMask);
  }

  update_screen_for_document(m_documentReader);
}

void PixelsMovement::catchImage(int x, int y, HandleType handle)
{
  ASSERT(handle != NoHandle);

  m_catchX = x;
  m_catchY = y;
  m_isDragging = true;
  m_handle = handle;
}

void PixelsMovement::catchImageAgain(int x, int y, HandleType handle)
{
  // Create a new UndoTransaction to move the pixels to other position
  m_initialData = m_currentData;
  m_isDragging = true;

  m_catchX = x;
  m_catchY = y;

  m_handle = handle;

  // Hide the mask (do not deselect it, it will be moved them using
  // m_undoTransaction.setMaskPosition)
  Mask emptyMask;
  {
    DocumentWriter documentWriter(m_documentReader);
    documentWriter->generateMaskBoundaries(&emptyMask);
  }

  update_screen_for_document(m_documentReader);
}

void PixelsMovement::maskImage(const Image* image, int x, int y)
{
  mask_replace(m_currentMask, x, y, image->w, image->h);

  m_currentMask->freeze();
  image_clear(m_currentMask->bitmap, 0);
  for (int v=0; v<image->h; ++v) {
    for (int u=0; u<image->w; ++u) {
      int bit = (image->getpixel(u, v) != image->mask_color ? 1: 0);
      m_currentMask->bitmap->putpixel(u, v, bit);
    }
  }
  m_currentMask->unfreeze();

  mask_copy(m_initialMask, m_currentMask);

  DocumentWriter documentWriter(m_documentReader);

  m_undoTransaction.copyToCurrentMask(m_currentMask);

  documentWriter->setMask(m_currentMask);
  documentWriter->generateMaskBoundaries(m_currentMask);

  update_screen_for_document(m_documentReader);
}

gfx::Rect PixelsMovement::moveImage(int x, int y, MoveModifier moveModifier)
{
  DocumentWriter documentWriter(m_documentReader);
  Image* image = documentWriter->getExtraCelImage();
  Cel* cel = documentWriter->getExtraCel();
  int x1, y1, x2, y2;

  x1 = m_initialData.bounds().x;
  y1 = m_initialData.bounds().y;
  x2 = m_initialData.bounds().x + m_initialData.bounds().w;
  y2 = m_initialData.bounds().y + m_initialData.bounds().h;

  bool updateBounds = false;
  int dx, dy;

  dx = ((x - m_catchX) *  cos(m_currentData.angle()) +
        (y - m_catchY) * -sin(m_currentData.angle()));
  dy = ((x - m_catchX) *  sin(m_currentData.angle()) +
        (y - m_catchY) *  cos(m_currentData.angle()));

  switch (m_handle) {

    case MoveHandle:
      x1 += dx;
      y1 += dy;
      x2 += dx;
      y2 += dy;
      updateBounds = true;

      if ((moveModifier & SnapToGridMovement) == SnapToGridMovement) {
        // Snap the x1,y1 point to the grid.
        gfx::Point gridOffset(x1, y1);
        UIContext::instance()->getSettings()->snapToGrid(gridOffset, NormalSnap);

        // Now we calculate the difference from x1,y1 point and we can
        // use it to adjust all coordinates (x1, y1, x2, y2).
        gridOffset -= gfx::Point(x1, y1);

        x1 += gridOffset.x;
        y1 += gridOffset.y;
        x2 += gridOffset.x;
        y2 += gridOffset.y;
      }
      break;

    case ScaleNWHandle:
      x1 = MIN(x1+dx, x2-1);
      y1 = MIN(y1+dy, y2-1);

      if ((moveModifier & MaintainAspectRatioMovement) == MaintainAspectRatioMovement) {
        if (1000 * (x2-x1) / getInitialImageSize().w >
            1000 * (y2-y1) / getInitialImageSize().h) {
          y1 = y2 - getInitialImageSize().h * (x2-x1) / getInitialImageSize().w;
        }
        else {
          x1 = x2 - getInitialImageSize().w * (y2-y1) / getInitialImageSize().h;
        }
      }

      updateBounds = true;
      break;

    case ScaleNHandle:
      y1 = MIN(y1+dy, y2-1);
      updateBounds = true;
      break;

    case ScaleNEHandle:
      x2 = MAX(x2+dx, x1+1);
      y1 = MIN(y1+dy, y2-1);

      if ((moveModifier & MaintainAspectRatioMovement) == MaintainAspectRatioMovement) {
        if (1000 * (x2-x1) / getInitialImageSize().w >
            1000 * (y2-y1) / getInitialImageSize().h) {
          y1 = y2 - getInitialImageSize().h * (x2-x1) / getInitialImageSize().w;
        }
        else {
          x2 = x1 + getInitialImageSize().w * (y2-y1) / getInitialImageSize().h;
        }
      }

      updateBounds = true;
      break;

    case ScaleWHandle:
      x1 = MIN(x1+dx, x2-1);
      updateBounds = true;
      break;

    case ScaleEHandle:
      x2 = MAX(x2+dx, x1+1);
      updateBounds = true;
      break;

    case ScaleSWHandle:
      x1 = MIN(x1+dx, x2-1);
      y2 = MAX(y2+dy, y1+1);

      if ((moveModifier & MaintainAspectRatioMovement) == MaintainAspectRatioMovement) {
        if (1000 * (x2-x1) / getInitialImageSize().w >
            1000 * (y2-y1) / getInitialImageSize().h) {
          y2 = y1 + getInitialImageSize().h * (x2-x1) / getInitialImageSize().w;
        }
        else {
          x1 = x2 - getInitialImageSize().w * (y2-y1) / getInitialImageSize().h;
        }
      }

      updateBounds = true;
      break;

    case ScaleSHandle:
      y2 = MAX(y2+dy, y1+1);
      updateBounds = true;
      break;

    case ScaleSEHandle:
      x2 = MAX(x2+dx, x1+1);
      y2 = MAX(y2+dy, y1+1);

      if ((moveModifier & MaintainAspectRatioMovement) == MaintainAspectRatioMovement) {
        if (1000 * (x2-x1) / getInitialImageSize().w >
            1000 * (y2-y1) / getInitialImageSize().h) {
          y2 = y1 + getInitialImageSize().h * (x2-x1) / getInitialImageSize().w;
        }
        else {
          x2 = x1 + getInitialImageSize().w * (y2-y1) / getInitialImageSize().h;
        }
      }

      updateBounds = true;
      break;

    case RotateNWHandle:
    case RotateNHandle:
    case RotateNEHandle:
    case RotateWHandle:
    case RotateEHandle:
    case RotateSWHandle:
    case RotateSHandle:
    case RotateSEHandle:
      {
        gfx::Point abs_initial_pivot = m_initialData.pivot();
        gfx::Point abs_pivot = m_currentData.pivot();

        double newAngle =
          m_initialData.angle()
          + atan2((double)(-y + abs_pivot.y),
                  (double)(+x - abs_pivot.x))
          - atan2((double)(-m_catchY + abs_initial_pivot.y),
                  (double)(+m_catchX - abs_initial_pivot.x));

        // Put the angle in -180 to 180 range.
        while (newAngle < -PI) newAngle += 2*PI;
        while (newAngle > PI) newAngle -= 2*PI;

        // Is the "angle snap" is activated, we've to snap the angle
        // to common (pixel art) angles.
        if ((moveModifier & AngleSnapMovement) == AngleSnapMovement) {
          // TODO make this configurable
          static const double keyAngles[] = {
            0.0, 26.565, 45.0, 63.435, 90.0, 116.565, 135.0, 153.435, -180.0,
            180.0, -153.435, -135.0, -116, -90.0, -63.435, -45.0, -26.565
          };

          double newAngleDegrees = 180.0 * newAngle / PI;

          int closest = 0;
          int last = sizeof(keyAngles) / sizeof(keyAngles[0]) - 1;
          for (int i=0; i<=last; ++i) {
            if (std::fabs(newAngleDegrees-keyAngles[closest]) >
                std::fabs(newAngleDegrees-keyAngles[i]))
              closest = i;
          }

          newAngle = PI * keyAngles[closest] / 180.0;
        }

        m_currentData.angle(newAngle);
      }
      break;

    case PivotHandle:
      {
        // Calculate the new position of the pivot
        gfx::Point newPivot(m_initialData.pivot().x + (x - m_catchX),
                            m_initialData.pivot().y + (y - m_catchY));

        m_currentData = m_initialData;
        m_currentData.displacePivotTo(newPivot);
      }
      break;
  }

  if (updateBounds) {
    m_currentData.bounds(gfx::Rect(x1, y1, x2 - x1, y2 - y1));
    m_adjustPivot = true;
  }

  gfx::Transformation::Corners corners;
  m_currentData.transformBox(corners);

  // Transform the extra-cel which is the chunk of pixels that the user is moving.
  image_clear(documentWriter->getExtraCelImage(), 0);
  image_parallelogram(documentWriter->getExtraCelImage(), m_originalImage,
                      corners.leftTop().x, corners.leftTop().y,
                      corners.rightTop().x, corners.rightTop().y,
                      corners.rightBottom().x, corners.rightBottom().y,
                      corners.leftBottom().x, corners.leftBottom().y);

  // Transform mask
  mask_replace(m_currentMask, 0, 0, m_sprite->getWidth(), m_sprite->getHeight());
  m_currentMask->freeze();
  image_clear(m_currentMask->bitmap, 0);
  image_parallelogram(m_currentMask->bitmap, m_initialMask->bitmap,
                      corners.leftTop().x, corners.leftTop().y,
                      corners.rightTop().x, corners.rightTop().y,
                      corners.rightBottom().x, corners.rightBottom().y,
                      corners.leftBottom().x, corners.leftBottom().y);
  m_currentMask->unfreeze();

  if (m_firstDrop)
    m_undoTransaction.copyToCurrentMask(m_currentMask);
  else
    documentWriter->setMask(m_currentMask);

  documentWriter->setTransformation(m_currentData);

  return gfx::Rect(0, 0, m_sprite->getWidth(), m_sprite->getHeight());
}

void PixelsMovement::dropImageTemporarily()
{
  m_isDragging = false;

  {
    DocumentWriter documentWriter(m_documentReader);

    // TODO Add undo information so the user can undo each transformation step.

    // Displace the pivot to the new location:
    if (m_adjustPivot) {
      m_adjustPivot = false;

      // Get the a factor for the X/Y position of the initial pivot
      // position inside the initial non-rotated bounds.
      gfx::PointT<double> pivotPosFactor(m_initialData.pivot() - m_initialData.bounds().getOrigin());
      pivotPosFactor.x /= m_initialData.bounds().w;
      pivotPosFactor.y /= m_initialData.bounds().h;

      // Get the current transformed bounds.
      gfx::Transformation::Corners corners;
      m_currentData.transformBox(corners);

      // The new pivot will be located from the rotated left-top
      // corner a distance equal to the transformed bounds's
      // width/height multiplied with the previously calculated X/Y
      // factor.
      la::Vector2d<double> newPivot(corners.leftTop().x,
                                    corners.leftTop().y);
      newPivot += pivotPosFactor.x * point2Vector(corners.rightTop() - corners.leftTop());
      newPivot += pivotPosFactor.y * point2Vector(corners.leftBottom() - corners.leftTop());

      m_currentData.displacePivotTo(gfx::Point(newPivot.x, newPivot.y));
    }

    documentWriter->generateMaskBoundaries(m_currentMask);
  }

  update_screen_for_document(m_documentReader);
}

void PixelsMovement::dropImage()
{
  m_isDragging = false;

  const Cel* cel = m_documentReader->getExtraCel();
  const Image* image = m_documentReader->getExtraCelImage();

  {
    DocumentWriter documentWriter(m_documentReader);
    {
      // Expand the canvas to paste the image in the fully visible
      // portion of sprite.
      ExpandCelCanvas expandCelCanvas(documentWriter, m_sprite,
                                      m_sprite->getCurrentLayer(), TILED_NONE);

      image_merge(expandCelCanvas.getDestCanvas(), image,
                  -expandCelCanvas.getCel()->getX(),
                  -expandCelCanvas.getCel()->getY(),
                  cel->getOpacity(), BLEND_MODE_NORMAL);

      expandCelCanvas.commit();
    }
    m_undoTransaction.commit();

    documentWriter->destroyExtraCel();
  }
}

bool PixelsMovement::isDragging() const
{
  return m_isDragging;
}

gfx::Rect PixelsMovement::getImageBounds()
{
  const Cel* cel = m_documentReader->getExtraCel();
  const Image* image = m_documentReader->getExtraCelImage();

  ASSERT(cel != NULL);
  ASSERT(image != NULL);

  return gfx::Rect(cel->getX(), cel->getY(), image->w, image->h);
}

gfx::Size PixelsMovement::getInitialImageSize() const
{
  return m_initialData.bounds().getSize();
}

void PixelsMovement::setMaskColor(uint32_t mask_color)
{
  {
    DocumentWriter documentWriter(m_documentReader);
    Image* image = documentWriter->getExtraCelImage();

    ASSERT(image != NULL);

    image->mask_color = mask_color;
  }

  update_screen_for_document(m_documentReader);
}
