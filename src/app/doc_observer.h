// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_OBSERVER_H_INCLUDED
#define APP_DOC_OBSERVER_H_INCLUDED
#pragma once

namespace doc {
  class Remap;
}

namespace app {
  class Doc;
  class DocEvent;

  class DocObserver {
  public:
    virtual ~DocObserver() { }

    virtual void onCloseDocument(Doc* doc) { }
    virtual void onFileNameChanged(Doc* doc) { }

    // General update. If an observer receives this event, it's because
    // anything in the document could be changed.
    virtual void onGeneralUpdate(DocEvent& ev) { }

    virtual void onColorSpaceChanged(DocEvent& ev) { }
    virtual void onPixelFormatChanged(DocEvent& ev) { }
    virtual void onPaletteChanged(DocEvent& ev) { }

    virtual void onAddLayer(DocEvent& ev) { }
    virtual void onAddFrame(DocEvent& ev) { }
    virtual void onAddCel(DocEvent& ev) { }
    virtual void onAddSlice(DocEvent& ev) { }
    virtual void onAddTag(DocEvent& ev) { }

    virtual void onBeforeRemoveLayer(DocEvent& ev) { }
    virtual void onAfterRemoveLayer(DocEvent& ev) { }

    // Called when a frame is removed. It's called after the frame was
    // removed, and the sprite's total number of frames is modified.
    virtual void onRemoveFrame(DocEvent& ev) { }
    virtual void onRemoveTag(DocEvent& ev) { }
    virtual void onBeforeRemoveCel(DocEvent& ev) { }
    virtual void onAfterRemoveCel(DocEvent& ev) { }
    virtual void onRemoveSlice(DocEvent& ev) { }

    virtual void onSpriteSizeChanged(DocEvent& ev) { }
    virtual void onSpriteTransparentColorChanged(DocEvent& ev) { }
    virtual void onSpritePixelRatioChanged(DocEvent& ev) { }
    virtual void onSpriteGridBoundsChanged(DocEvent& ev) { }

    virtual void onLayerNameChange(DocEvent& ev) { }
    virtual void onLayerOpacityChange(DocEvent& ev) { }
    virtual void onLayerBlendModeChange(DocEvent& ev) { }
    virtual void onLayerRestacked(DocEvent& ev) { }
    virtual void onLayerMergedDown(DocEvent& ev) { }

    virtual void onCelMoved(DocEvent& ev) { }
    virtual void onCelCopied(DocEvent& ev) { }
    virtual void onCelFrameChanged(DocEvent& ev) { }
    virtual void onCelPositionChanged(DocEvent& ev) { }
    virtual void onCelOpacityChange(DocEvent& ev) { }
    virtual void onCelZIndexChange(DocEvent& ev) { }

    virtual void onUserDataChange(DocEvent& ev) { }

    virtual void onFrameDurationChanged(DocEvent& ev) { }

    virtual void onImagePixelsModified(DocEvent& ev) { }
    virtual void onSpritePixelsModified(DocEvent& ev) { }
    virtual void onExposeSpritePixels(DocEvent& ev) { }

    // When the number of total frames available is modified.
    virtual void onTotalFramesChanged(DocEvent& ev) { }

    // The selection has changed.
    virtual void onSelectionChanged(DocEvent& ev) { }
    virtual void onSelectionBoundariesChanged(DocEvent& ev) { }

    // When the tag range changes
    virtual void onTagChange(DocEvent& ev) { }

    // When the tag is renamed
    virtual void onTagRename(DocEvent& ev) { }

    // Slices
    virtual void onSliceNameChange(DocEvent& ev) { }

    // The tileset has changed.
    virtual void onTilesetChanged(DocEvent& ev) { }

    // The collapsed/expanded flag of a specific layer changed.
    virtual void onLayerCollapsedChanged(DocEvent& ev) { }

    // The tileset was remapped (e.g. when tiles are re-ordered).
    virtual void onRemapTileset(DocEvent& ev, const doc::Remap& remap) { }

    // When the tile management plugin property is changed.
    virtual void onTileManagementPluginChange(DocEvent& ev) { }

    // When a new tilemap cel/tile is created in certain situations,
    // like after drawing in an empty tilemap cel, or when we paste a
    // cel into an empty tilemap cel, so a new tile is created.
    //
    // This is useful for a tile management plugin to know when a new
    // tile is added automatically by the editor or the timeline
    // through draw_image_into_new_tilemap_cel(), and the plugin can
    // do some extra work with it.
    //
    // Warning: This must be triggered from the UI thread (because
    // scripts will listen this event).
    virtual void onAfterAddTile(DocEvent& ev) { }

  };

} // namespace app

#endif
