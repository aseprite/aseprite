// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_DOCUMENT_H_INCLUDED
#define APP_DOCUMENT_H_INCLUDED
#pragma once

#include "app/extra_cel.h"
#include "app/file/format_options.h"
#include "app/transformation.h"
#include "base/disable_copying.h"
#include "base/mutex.h"
#include "base/observable.h"
#include "base/shared_ptr.h"
#include "base/unique_ptr.h"
#include "doc/blend_mode.h"
#include "doc/color.h"
#include "doc/document.h"
#include "doc/frame.h"
#include "doc/pixel_format.h"
#include "gfx/rect.h"

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
  class Document : public doc::Document {
  public:
    enum LockType {
      ReadLock,
      WriteLock
    };

    Document(Sprite* sprite);
    ~Document();

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
    // Copying

    void copyLayerContent(const Layer* sourceLayer, Document* destDoc, Layer* destLayer) const;
    Document* duplicate(DuplicateType type) const;

    //////////////////////////////////////////////////////////////////////
    // Multi-threading ("sprite wrappers" use this)

    // Locks the sprite to read or write on it, returning true if the
    // sprite can be accessed in the desired mode.
    bool lock(LockType lockType, int timeout);

    // If you've locked the sprite to read, using this method you can
    // raise your access level to write it.
    bool lockToWrite(int timeout);

    // If you've locked the sprite to write, using this method you can
    // your access level to only read it.
    void unlockToRead();

    void unlock();

  protected:
    virtual void onContextChanged() override;

  private:
    // Undo and redo information about the document.
    base::UniquePtr<DocumentUndo> m_undo;

    // True if this sprite is associated to a file in the file-system.
    bool m_associated_to_file;

    // Selected mask region boundaries
    base::UniquePtr<doc::MaskBoundaries> m_maskBoundaries;

    // Mutex to modify the 'locked' flag.
    base::mutex m_mutex;

    // True if some thread is writing the sprite.
    bool m_write_lock;

    // Greater than zero when one or more threads are reading the sprite.
    int m_read_locks;

    // Data to save the file in the same format that it was loaded
    base::SharedPtr<FormatOptions> m_format_options;

    // Extra cel used to draw extra stuff (e.g. editor's pen preview, pixels in movement, etc.)
    ExtraCelRef m_extraCel;

    // Current mask.
    base::UniquePtr<Mask> m_mask;
    bool m_maskVisible;

    // Current transformation.
    Transformation m_transformation;

    DISABLE_COPYING(Document);
  };

} // namespace app

#endif
