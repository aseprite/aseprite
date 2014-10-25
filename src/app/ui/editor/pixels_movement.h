/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_UI_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#define APP_UI_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#pragma once

#include "app/context_access.h"
#include "app/settings/settings_observers.h"
#include "app/ui/editor/handle_type.h"
#include "app/undo_transaction.h"
#include "base/shared_ptr.h"
#include "gfx/size.h"
#include "doc/algorithm/flip_type.h"

namespace doc {
  class Image;
  class Sprite;
}

namespace app {
  class Document;

  // Helper class to move pixels interactively and control undo history
  // correctly.  The extra cel of the sprite is temporally used to show
  // feedback, drag, and drop the specified image in the constructor
  // (which generally would be the selected region or the clipboard
  // content).
  class PixelsMovement : public SelectionSettingsObserver {
  public:
    enum MoveModifier {
      NormalMovement = 1,
      SnapToGridMovement = 2,
      AngleSnapMovement = 4,
      MaintainAspectRatioMovement = 8,
      LockAxisMovement = 16
    };

    // The "moveThis" image specifies the chunk of pixels to be moved.
    // The "x" and "y" parameters specify the initial position of the image.
    PixelsMovement(Context* context,
                   Document* document, Sprite* sprite, Layer* layer,
                   const Image* moveThis, int x, int y, int opacity,
                   const char* operationName);
    ~PixelsMovement();

    void cutMask();
    void copyMask();
    void catchImage(int x, int y, HandleType handle);
    void catchImageAgain(int x, int y, HandleType handle);

    // Creates a mask for the given image. Useful when the user paste a
    // image from the clipboard.
    void maskImage(const Image* image, int x, int y);

    // Moves the image to the new position (relative to the start
    // position given in the ctor).
    void moveImage(int x, int y, MoveModifier moveModifier);

    // Returns a copy of the current image being dragged with the
    // current transformation.
    Image* getDraggedImageCopy(gfx::Point& origin);

    // Copies the image being dragged in the current position.
    void stampImage();

    void dropImageTemporarily();
    void dropImage();
    void discardImage(bool commit = true);
    bool isDragging() const;

    gfx::Rect getImageBounds();
    gfx::Size getInitialImageSize() const;

    void setMaskColor(color_t mask_color);

    // Flips the image and mask in the given direction in "flipType".
    // Flip Horizontally/Vertically commands are replaced calling this
    // function, so they work more as the user would expect (flip the
    // current selection instead of dropping and flipping it).
    void flipImage(doc::algorithm::FlipType flipType);

    const gfx::Transformation& getTransformation() const { return m_currentData; }

  protected:
    void onSetRotationAlgorithm(RotationAlgorithm algorithm) override;

  private:
    void redrawExtraImage();
    void redrawCurrentMask();
    void drawImage(doc::Image* dst, const gfx::Point& pt);
    void drawParallelogram(doc::Image* dst, doc::Image* src,
      const gfx::Transformation::Corners& corners,
      const gfx::Point& leftTop);
    void updateDocumentMask();

    const ContextReader m_reader;
    Document* m_document;
    Sprite* m_sprite;
    UndoTransaction m_undoTransaction;
    bool m_firstDrop;
    bool m_isDragging;
    bool m_adjustPivot;
    HandleType m_handle;
    Image* m_originalImage;
    int m_catchX, m_catchY;
    gfx::Transformation m_initialData;
    gfx::Transformation m_currentData;
    Mask* m_initialMask;
    Mask* m_currentMask;
    color_t m_maskColor;
  };

  inline PixelsMovement::MoveModifier& operator|=(PixelsMovement::MoveModifier& a,
                                                  const PixelsMovement::MoveModifier& b) {
    a = static_cast<PixelsMovement::MoveModifier>(a | b);
    return a;
  }

  typedef SharedPtr<PixelsMovement> PixelsMovementPtr;
  
} // namespace app

#endif
