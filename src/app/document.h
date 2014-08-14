/* Aseprite
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

#ifndef APP_DOCUMENT_H_INCLUDED
#define APP_DOCUMENT_H_INCLUDED
#pragma once

#include "base/override.h"
#include "base/disable_copying.h"
#include "base/observable.h"
#include "base/shared_ptr.h"
#include "base/unique_ptr.h"
#include "doc/document.h"
#include "gfx/transformation.h"
#include "raster/frame_number.h"
#include "raster/pixel_format.h"

#include <string>

namespace base {
  class mutex;
}

namespace raster {
  class Cel;
  class Image;
  class Layer;
  class Mask;
  class Sprite;
}

namespace gfx {
  class Region;
}

namespace undo {
  class UndoersCollector;
}

namespace app {
  class DocumentApi;
  class DocumentUndo;
  class FormatOptions;
  struct BoundSeg;

  using namespace raster;

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
    DocumentApi getApi(undo::UndoersCollector* undoers = NULL);

    //////////////////////////////////////////////////////////////////////
    // Main properties

    const DocumentUndo* getUndo() const { return m_undo; }
    DocumentUndo* getUndo() { return m_undo; }

    //////////////////////////////////////////////////////////////////////
    // Notifications

    void notifyGeneralUpdate();
    void notifySpritePixelsModified(Sprite* sprite, const gfx::Region& region);
    void notifyLayerMergedDown(Layer* srcLayer, Layer* targetLayer);
    void notifyCelMoved(Layer* fromLayer, FrameNumber fromFrame, Layer* toLayer, FrameNumber toFrame);
    void notifyCelCopied(Layer* fromLayer, FrameNumber fromFrame, Layer* toLayer, FrameNumber toFrame);

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

    //////////////////////////////////////////////////////////////////////
    // Loaded options from file

    void setFormatOptions(const SharedPtr<FormatOptions>& format_options);
    SharedPtr<FormatOptions> getFormatOptions() { return m_format_options; }

    //////////////////////////////////////////////////////////////////////
    // Boundaries

    int getBoundariesSegmentsCount() const;
    const BoundSeg* getBoundariesSegments() const;

    void generateMaskBoundaries(Mask* mask = NULL);

    //////////////////////////////////////////////////////////////////////
    // Extra Cel (it is used to draw pen preview, pixels in movement, etc.)

    void prepareExtraCel(int x, int y, int w, int h, int opacity);
    void destroyExtraCel();
    Cel* getExtraCel() const;
    Image* getExtraCelImage() const;

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

    gfx::Transformation getTransformation() const;
    void setTransformation(const gfx::Transformation& transform);
    void resetTransformation();

    //////////////////////////////////////////////////////////////////////
    // Copying

    void copyLayerContent(const Layer* sourceLayer, Document* destDoc, Layer* destLayer) const;
    Document* duplicate(DuplicateType type) const;

    //////////////////////////////////////////////////////////////////////
    // Multi-threading ("sprite wrappers" use this)

    // Locks the sprite to read or write on it, returning true if the
    // sprite can be accessed in the desired mode.
    bool lock(LockType lockType);

    // If you've locked the sprite to read, using this method you can
    // raise your access level to write it.
    bool lockToWrite();

    // If you've locked the sprite to write, using this method you can
    // your access level to only read it.
    void unlockToRead();

    void unlock();

  protected:
    virtual void onContextChanged() OVERRIDE;

  private:
    // Undo and redo information about the document.
    base::UniquePtr<DocumentUndo> m_undo;

    // True if this sprite is associated to a file in the file-system.
    bool m_associated_to_file;

    // Selected mask region boundaries
    struct {
      int nseg;
      BoundSeg* seg;
    } m_bound;

    // Mutex to modify the 'locked' flag.
    base::mutex* m_mutex;

    // True if some thread is writing the sprite.
    bool m_write_lock;

    // Greater than zero when one or more threads are reading the sprite.
    int m_read_locks;

    // Data to save the file in the same format that it was loaded
    SharedPtr<FormatOptions> m_format_options;

    // Extra cel used to draw extra stuff (e.g. editor's pen preview, pixels in movement, etc.)
    Cel* m_extraCel;

    // Image of the extra cel.
    Image* m_extraImage;

    // Current mask.
    base::UniquePtr<Mask> m_mask;
    bool m_maskVisible;

    // Current transformation.
    gfx::Transformation m_transformation;

    DISABLE_COPYING(Document);
  };

} // namespace app

#endif
