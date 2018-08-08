// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#define APP_UI_EDITOR_PIXELS_MOVEMENT_H_INCLUDED
#pragma once

#include "app/context_access.h"
#include "app/extra_cel.h"
#include "app/site.h"
#include "app/transaction.h"
#include "app/ui/editor/handle_type.h"
#include "base/shared_ptr.h"
#include "doc/algorithm/flip_type.h"
#include "gfx/size.h"
#include "obs/connection.h"

namespace doc {
  class Image;
  class Mask;
  class Sprite;
}

namespace app {
  class Doc;

  namespace cmd {
    class SetMask;
  }

  // Helper class to move pixels interactively and control undo history
  // correctly.  The extra cel of the sprite is temporally used to show
  // feedback, drag, and drop the specified image in the constructor
  // (which generally would be the selected region or the clipboard
  // content).
  class PixelsMovement {
  public:
    enum MoveModifier {
      NormalMovement = 1,
      SnapToGridMovement = 2,
      AngleSnapMovement = 4,
      MaintainAspectRatioMovement = 8,
      LockAxisMovement = 16,
      ScaleFromPivot = 32,
    };

    enum CommitChangesOption {
      DontCommitChanges,
      CommitChanges,
    };

    enum KeepMaskOption {
      DontKeepMask,
      KeepMask,
    };

    PixelsMovement(Context* context,
                   Site site,
                   const Image* moveThis,
                   const Mask* mask,
                   const char* operationName);
    ~PixelsMovement();

    HandleType handle() const { return m_handle; }

    void trim();
    void cutMask();
    void copyMask();
    void catchImage(const gfx::Point& pos, HandleType handle);
    void catchImageAgain(const gfx::Point& pos, HandleType handle);

    // Moves the image to the new position (relative to the start
    // position given in the ctor).
    void moveImage(const gfx::Point& pos, MoveModifier moveModifier);

    // Returns a copy of the current image being dragged with the
    // current transformation.
    void getDraggedImageCopy(std::unique_ptr<Image>& outputImage,
                             std::unique_ptr<Mask>& outputMask);

    // Copies the image being dragged in the current position.
    void stampImage();

    void dropImageTemporarily();
    void dropImage();
    void discardImage(const CommitChangesOption commit = CommitChanges,
                      const KeepMaskOption keepMask = DontKeepMask);
    bool isDragging() const;

    gfx::Rect getImageBounds();
    gfx::Size getInitialImageSize() const;

    void setMaskColor(bool opaque, color_t mask_color);

    // Flips the image and mask in the given direction in "flipType".
    // Flip Horizontally/Vertically commands are replaced calling this
    // function, so they work more as the user would expect (flip the
    // current selection instead of dropping and flipping it).
    void flipImage(doc::algorithm::FlipType flipType);

    // Rotates the image and the mask the given angle. It's used to
    // simulate RotateCommand when we're inside MovingPixelsState.
    void rotate(double angle);

    const Transformation& getTransformation() const { return m_currentData; }

  private:
    void onPivotChange();
    void onRotationAlgorithmChange();
    void redrawExtraImage();
    void redrawCurrentMask();
    void drawImage(doc::Image* dst, const gfx::Point& pos, bool renderOriginalLayer);
    void drawMask(doc::Mask* dst, bool shrink);
    void drawParallelogram(doc::Image* dst, const doc::Image* src, const doc::Mask* mask,
      const Transformation::Corners& corners,
      const gfx::Point& leftTop);
    void updateDocumentMask();

    const ContextReader m_reader;
    Site m_site;
    Doc* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    Transaction m_transaction;
    cmd::SetMask* m_setMaskCmd;
    bool m_isDragging;
    bool m_adjustPivot;
    HandleType m_handle;
    Image* m_originalImage;
    gfx::Point m_catchPos;
    Transformation m_initialData;
    Transformation m_currentData;
    Mask* m_initialMask;
    Mask* m_currentMask;
    bool m_opaque;
    color_t m_maskColor;
    obs::scoped_connection m_pivotVisConn;
    obs::scoped_connection m_pivotPosConn;
    obs::scoped_connection m_rotAlgoConn;
    ExtraCelRef m_extraCel;
  };

  inline PixelsMovement::MoveModifier& operator|=(PixelsMovement::MoveModifier& a,
                                                  const PixelsMovement::MoveModifier& b) {
    a = static_cast<PixelsMovement::MoveModifier>(a | b);
    return a;
  }

  typedef base::SharedPtr<PixelsMovement> PixelsMovementPtr;

} // namespace app

#endif
