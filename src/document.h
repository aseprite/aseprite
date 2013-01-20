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

#ifndef DOCUMENT_H_INCLUDED
#define DOCUMENT_H_INCLUDED

#include "base/disable_copying.h"
#include "base/shared_ptr.h"
#include "base/unique_ptr.h"
#include "document_id.h"
#include "gfx/transformation.h"
#include "observable.h"
#include "raster/pixel_format.h"

#include <string>

class Cel;
class DocumentObserver;
class DocumentUndo;
class FormatOptions;
class Image;
class Layer;
class Mask;
class Mutex;
class Sprite;
struct _BoundSeg;

namespace gfx { class Region; }

enum DuplicateType
{
  DuplicateExactCopy,
  DuplicateWithFlattenLayers,
};

// An application document. It is the class used to contain one file
// opened and being edited by the user (a sprite).
class Document : public Observable<DocumentObserver> {
public:

  enum LockType {
    ReadLock,
    WriteLock
  };

  // Creates a document with one sprite, with one transparent layer,
  // and one frame.
  static Document* createBasicDocument(PixelFormat format, int width, int height, int ncolors);

  Document(Sprite* sprite);
  ~Document();

  //////////////////////////////////////////////////////////////////////
  // Main properties

  DocumentId getId() const { return m_id; }
  void setId(DocumentId id) { m_id = id; }

  const Sprite* getSprite() const { return m_sprite; }
  const DocumentUndo* getUndo() const { return m_undo; }

  Sprite* getSprite() { return m_sprite; }
  DocumentUndo* getUndo() { return m_undo; }

  void addSprite(Sprite* sprite);

  //////////////////////////////////////////////////////////////////////
  // Notifications

  void notifyGeneralUpdate();
  void notifySpritePixelsModified(Sprite* sprite, const gfx::Region& region);

  //////////////////////////////////////////////////////////////////////
  // File related properties

  const char* getFilename() const;
  void setFilename(const char* filename);

  bool isModified() const;
  bool isAssociatedToFile() const;
  void markAsSaved();

  //////////////////////////////////////////////////////////////////////
  // Loaded options from file

  void setFormatOptions(const SharedPtr<FormatOptions>& format_options);

  //////////////////////////////////////////////////////////////////////
  // Boundaries

  int getBoundariesSegmentsCount() const;
  const _BoundSeg* getBoundariesSegments() const;

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
  Mask* getMask() const;

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

private:
  // Unique identifier for this document (it is assigned by Documents class).
  DocumentId m_id;

  // The main sprite.
  UniquePtr<Sprite> m_sprite;

  // Undo and redo information about the document.
  UniquePtr<DocumentUndo> m_undo;

  // Document's file name (from where it was loaded, where it is saved).
  std::string m_filename;

  // True if this sprite is associated to a file in the file-system.
  bool m_associated_to_file;

  // Selected mask region boundaries
  struct {
    int nseg;
    _BoundSeg* seg;
  } m_bound;

  // Mutex to modify the 'locked' flag.
  Mutex* m_mutex;

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
  UniquePtr<Mask> m_mask;
  bool m_maskVisible;

  // Current transformation.
  gfx::Transformation m_transformation;

  DISABLE_COPYING(Document);
};

#endif
