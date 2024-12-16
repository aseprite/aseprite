// Aseprite
// Copyright (C) 2019-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_EXPAND_CEL_CANVAS_H_INCLUDED
#define APP_UTIL_EXPAND_CEL_CANVAS_H_INCLUDED
#pragma once

#include "app/tilemap_mode.h"
#include "app/tileset_mode.h"
#include "doc/frame.h"
#include "doc/grid.h"
#include "doc/image_ref.h"
#include "doc/layer.h"
#include "filters/tiled_mode.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"

namespace doc {
class Cel;
class Image;
class Layer;
class Sprite;
class Tileset;
} // namespace doc

namespace app {
class CmdSequence;
class Doc;
class Site;

using namespace filters;
using namespace doc;

// This class can be used to expand the canvas of the current cel to
// the visible portion of sprite. If the user cancels the operation,
// you've a rollback() method to restore the cel to its original
// state.  If all changes are committed, some undo information is
// stored in the document's UndoHistory to go back to the original
// state using "Undo" command.
class ExpandCelCanvas {
public:
  enum Flags {
    None = 0,
    NeedsSource = 1,
    // Use tiles mode but with pixels bounds in dst image (e.g. for
    // selection preview)
    PixelsBounds = 2,
    // True if you want to preview the changes in the tileset. Only
    // useful in TilesetMode::Manual mode when editing tiles in a
    // tilemap. See getDestTileset() for details.
    TilesetPreview = 4,
    // Enable when we are going to use the expanded cel canvas for
    // preview purposes of a selection tools.
    SelectionPreview = 8,
  };

  ExpandCelCanvas(Site site,
                  Layer* layer,
                  const TiledMode tiledMode,
                  CmdSequence* cmds,
                  const Flags flags);
  ~ExpandCelCanvas();

  // Commit changes made in getDestCanvas() in the cel's image. Adds
  // information in the undo history so the user can undo the
  // modifications in the canvas.
  void commit();

  // Restore the cel as its original state as when ExpandCelCanvas()
  // was created.
  void rollback();

  gfx::Point getCelOrigin() const;

  Image* getSourceCanvas(); // You can read pixels from here
  Image* getDestCanvas();   // You can write pixels right here

  Tileset* getDestTileset(); // You can use this as a preview-tileset
                             // when the user is editing in "Manual" mode

  void validateSourceCanvas(const gfx::Region& rgn);
  void validateDestCanvas(const gfx::Region& rgn);
  void validateDestTileset(const gfx::Region& rgn, const gfx::Region& forceRgn);
  void invalidateDestCanvas();
  void invalidateDestCanvas(const gfx::Region& rgn);
  void copyValidDestToSourceCanvas(const gfx::Region& rgn);

  const Cel* getCel() const { return m_cel; }
  const doc::Grid& getGrid() const { return m_grid; }

private:
  gfx::Rect getTrimDstImageBounds() const;
  ImageRef trimDstImage(const gfx::Rect& bounds) const;
  void copySourceTilestToDestTileset();

  bool isTilesetPreview() const { return ((m_flags & TilesetPreview) == TilesetPreview); }

  bool isSelectionPreview() const { return ((m_flags & SelectionPreview) == SelectionPreview); }

  // This is the common case where we want to preview a change in
  // the given layer of ExpandCelCanvas ctor.
  bool previewSpecificLayerChanges() const
  {
    return (m_layer && m_layer->isImage() && !isSelectionPreview());
  }

  Doc* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  frame_t m_frame;
  Cel* m_cel;
  ImageRef m_celImage;
  bool m_celCreated;
  gfx::Point m_origCelPos;
  Flags m_flags;
  gfx::Rect m_bounds;
  ImageRef m_srcImage;
  ImageRef m_dstImage;
  std::unique_ptr<Tileset> m_dstTileset;
  bool m_closed;
  bool m_committed;
  CmdSequence* m_cmds;
  gfx::Region m_validSrcRegion;
  gfx::Region m_validDstRegion;

  // True if we can compare src image with dst image to patch the
  // cel. This is false when dst is copied to the src, so we cannot
  // reduce the patched region because both images will be the same.
  bool m_canCompareSrcVsDst;

  doc::Grid m_grid;
  TilemapMode m_tilemapMode;
  TilesetMode m_tilesetMode;
};

} // namespace app

#endif
