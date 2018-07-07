// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_H_INCLUDED
#define APP_DOCUMENT_H_INCLUDED
#pragma once

#include "app/doc_observer.h"
#include "app/extra_cel.h"
#include "app/file/format_options.h"
#include "app/transformation.h"
#include "base/disable_copying.h"
#include "base/mutex.h"
#include "base/rw_lock.h"
#include "base/shared_ptr.h"
#include "base/unique_ptr.h"
#include "doc/blend_mode.h"
#include "doc/color.h"
#include "doc/document.h"
#include "doc/frame.h"
#include "doc/pixel_format.h"
#include "gfx/rect.h"
#include "obs/observable.h"

#include <string>

namespace doc {
  class Cel;
  class Layer;
  class Mask;
  class MaskBoundaries;
  class Sprite;
}

namespace gfx {
  class Region;
}

namespace app {

  class Context;
  class DocumentApi;
  class DocumentUndo;
  class Transaction;

  using namespace doc;

  enum DuplicateType {
    DuplicateExactCopy,
    DuplicateWithFlattenLayers,
  };

  // An application document. It is the class used to contain one file
  // opened and being edited by the user (a sprite).
  class Document : public doc::Document,
                   public base::RWLock,
                   public obs::observable<DocObserver> {
    enum Flags {
      kAssociatedToFile = 1, // This sprite is associated to a file in the file-system
      kMaskVisible      = 2, // The mask wasn't hidden by the user
      kInhibitBackup    = 4, // Inhibit the backup process
    };
  public:
    Document(Sprite* sprite);
    ~Document();

    Context* context() const { return m_ctx; }
    void setContext(Context* ctx);

    // Returns a high-level API: observable and undoable methods.
    DocumentApi getApi(Transaction& transaction);

    //////////////////////////////////////////////////////////////////////
    // Main properties

    const DocumentUndo* undoHistory() const { return m_undo; }
    DocumentUndo* undoHistory() { return m_undo; }

    color_t bgColor() const;
    color_t bgColor(Layer* layer) const;

    //////////////////////////////////////////////////////////////////////
    // Notifications

    void notifyGeneralUpdate();
    void notifySpritePixelsModified(Sprite* sprite, const gfx::Region& region, frame_t frame);
    void notifyExposeSpritePixels(Sprite* sprite, const gfx::Region& region);
    void notifyLayerMergedDown(Layer* srcLayer, Layer* targetLayer);
    void notifyCelMoved(Layer* fromLayer, frame_t fromFrame, Layer* toLayer, frame_t toFrame);
    void notifyCelCopied(Layer* fromLayer, frame_t fromFrame, Layer* toLayer, frame_t toFrame);
    void notifySelectionChanged();

    //////////////////////////////////////////////////////////////////////
    // File related properties

    bool isModified() const;
    bool isAssociatedToFile() const;
    void markAsSaved();

    // You can use this to indicate that we've destroyed (or we cannot
    // trust) the file associated with the document (e.g. when we
    // cancel a Save operation in the middle). So it's impossible to
    // back to the saved state using the UndoHistory.
    void impossibleToBackToSavedState();

    // Returns true if it does make sense to create a backup in this
    // document. For example, it doesn't make sense to create a backup
    // for an unmodified document.
    bool needsBackup() const;

    // Can be used to avoid creating a backup when the file is in a
    // unusual temporary state (e.g. when the file is resized to be
    // exported with other size)
    bool inhibitBackup() const;
    void setInhibitBackup(const bool inhibitBackup);

    //////////////////////////////////////////////////////////////////////
    // Loaded options from file

    void setFormatOptions(const base::SharedPtr<FormatOptions>& format_options);
    base::SharedPtr<FormatOptions> getFormatOptions() { return m_format_options; }

    //////////////////////////////////////////////////////////////////////
    // Boundaries

    void generateMaskBoundaries(const Mask* mask = nullptr);

    const MaskBoundaries* getMaskBoundaries() const {
     return m_maskBoundaries.get();
    }

    //////////////////////////////////////////////////////////////////////
    // Extra Cel (it is used to draw pen preview, pixels in movement, etc.)

    ExtraCelRef extraCel() const { return m_extraCel; }
    void setExtraCel(const ExtraCelRef& extraCel) { m_extraCel = extraCel; }

    //////////////////////////////////////////////////////////////////////
    // Mask

    // Returns the current mask, it can be empty. The mask could be not
    // empty but hidden to the user if the setMaskVisible(false) was
    // used called before.
    Mask* mask() const { return m_mask; }

    // Sets the current mask. The new mask will be visible by default,
    // so you don't need to call setMaskVisible(true).
    void setMask(const Mask* mask);

    // Returns true only when the mask is not empty, and was not yet
    // hidden using setMaskVisible (e.g. when the user "deselect the
    // mask").
    bool isMaskVisible() const;

    // Changes the visibility state of the mask (it is useful only if
    // the getMask() is not empty and the user can see that the mask is
    // being hidden and shown to him).
    void setMaskVisible(bool visible);

    //////////////////////////////////////////////////////////////////////
    // Transformation

    Transformation getTransformation() const;
    void setTransformation(const Transformation& transform);
    void resetTransformation();

    //////////////////////////////////////////////////////////////////////
    // Last point used to draw straight lines using freehand tools + Shift key
    // (EditorCustomizationDelegate::isStraightLineFromLastPoint() modifier)

    static gfx::Point NoLastDrawingPoint();
    gfx::Point lastDrawingPoint() const { return m_lastDrawingPoint; }
    void setLastDrawingPoint(const gfx::Point& pos) { m_lastDrawingPoint = pos; }

    //////////////////////////////////////////////////////////////////////
    // Copying

    void copyLayerContent(const Layer* sourceLayer, Document* destDoc, Layer* destLayer) const;
    Document* duplicate(DuplicateType type) const;

    void close();

  protected:
    void onFileNameChange() override;
    virtual void onContextChanged();

  private:
    void removeFromContext();

    Context* m_ctx;
    int m_flags;

    // Undo and redo information about the document.
    base::UniquePtr<DocumentUndo> m_undo;

    // Selected mask region boundaries
    base::UniquePtr<doc::MaskBoundaries> m_maskBoundaries;

    // Data to save the file in the same format that it was loaded
    base::SharedPtr<FormatOptions> m_format_options;

    // Extra cel used to draw extra stuff (e.g. editor's pen preview, pixels in movement, etc.)
    ExtraCelRef m_extraCel;

    // Current mask.
    base::UniquePtr<Mask> m_mask;

    // Current transformation.
    Transformation m_transformation;

    gfx::Point m_lastDrawingPoint;

    DISABLE_COPYING(Document);
  };

} // namespace app

#endif
