// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#define APP_UI_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#pragma once

#include "app/context_access.h"
#include "app/settings/settings_observers.h"
#include "app/transaction.h"
#include "app/ui/editor/handle_type.h"
#include "base/shared_ptr.h"
#include "doc/algorithm/flip_type.h"
#include "doc/site.h"
#include "gfx/size.h"

namespace doc {
  class Image;
  class Sprite;
}

namespace app {
  class Document;

  namespace cmd {
    class SetMask;
  }

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
      Site site,
      const Image* moveThis,
      const gfx::Point& initialPos, int opacity,
      const char* operationName);
    ~PixelsMovement();

    void cutMask();
    void copyMask();
    void catchImage(const gfx::Point& pos, HandleType handle);
    void catchImageAgain(const gfx::Point& pos, HandleType handle);

    // Creates a mask for the given image. Useful when the user paste a
    // image from the clipboard.
    void maskImage(const Image* image);

    // Moves the image to the new position (relative to the start
    // position given in the ctor).
    void moveImage(const gfx::Point& pos, MoveModifier moveModifier);

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
    void drawImage(doc::Image* dst, const gfx::Point& pos);
    void drawParallelogram(doc::Image* dst, doc::Image* src,
      const gfx::Transformation::Corners& corners,
      const gfx::Point& leftTop);
    void updateDocumentMask();

    const ContextReader m_reader;
    Site m_site;
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    Transaction m_transaction;
    cmd::SetMask* m_setMaskCmd;
    bool m_isDragging;
    bool m_adjustPivot;
    HandleType m_handle;
    Image* m_originalImage;
    gfx::Point m_catchPos;
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

  typedef base::SharedPtr<PixelsMovement> PixelsMovementPtr;

} // namespace app

#endif
