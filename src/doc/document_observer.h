// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_DOCUMENT_OBSERVER_H_INCLUDED
#define DOC_DOCUMENT_OBSERVER_H_INCLUDED
#pragma once

namespace doc {
  class Document;
  class DocumentEvent;

  class DocumentObserver {
  public:
    virtual ~DocumentObserver() { }

    virtual void onFileNameChanged(Document* doc) { }

    // General update. If an observer receives this event, it's because
    // anything in the document could be changed.
    virtual void onGeneralUpdate(DocumentEvent& ev) { }

    virtual void onPixelFormatChanged(DocumentEvent& ev) { }

    virtual void onAddLayer(DocumentEvent& ev) { }
    virtual void onAddFrame(DocumentEvent& ev) { }
    virtual void onAddCel(DocumentEvent& ev) { }

    virtual void onBeforeRemoveLayer(DocumentEvent& ev) { }
    virtual void onAfterRemoveLayer(DocumentEvent& ev) { }

    // Called when a frame is removed. It's called after the frame was
    // removed, and the sprite's total number of frames is modified.
    virtual void onRemoveFrame(DocumentEvent& ev) { }

    virtual void onRemoveCel(DocumentEvent& ev) { }

    virtual void onSpriteSizeChanged(DocumentEvent& ev) { }
    virtual void onSpriteTransparentColorChanged(DocumentEvent& ev) { }
    virtual void onSpritePixelRatioChanged(DocumentEvent& ev) { }

    virtual void onLayerNameChange(DocumentEvent& ev) { }
    virtual void onLayerOpacityChange(DocumentEvent& ev) { }
    virtual void onLayerBlendModeChange(DocumentEvent& ev) { }
    virtual void onLayerRestacked(DocumentEvent& ev) { }
    virtual void onLayerMergedDown(DocumentEvent& ev) { }

    virtual void onCelMoved(DocumentEvent& ev) { }
    virtual void onCelCopied(DocumentEvent& ev) { }
    virtual void onCelFrameChanged(DocumentEvent& ev) { }
    virtual void onCelPositionChanged(DocumentEvent& ev) { }
    virtual void onCelOpacityChange(DocumentEvent& ev) { }

    virtual void onFrameDurationChanged(DocumentEvent& ev) { }

    virtual void onImagePixelsModified(DocumentEvent& ev) { }
    virtual void onSpritePixelsModified(DocumentEvent& ev) { }
    virtual void onExposeSpritePixels(DocumentEvent& ev) { }

    // When the number of total frames available is modified.
    virtual void onTotalFramesChanged(DocumentEvent& ev) { }

    // The selection has changed.
    virtual void onSelectionChanged(DocumentEvent& ev) { }

    // Called to destroy the observable. (Here you could call "delete this".)
    virtual void dispose() { }
  };

} // namespace doc

#endif
