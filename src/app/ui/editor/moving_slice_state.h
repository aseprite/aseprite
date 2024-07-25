// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_SLICE_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_SLICE_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_hit.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/editor/pixels_movement.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/mask.h"
#include "doc/selected_layers.h"
#include "doc/selected_objects.h"
#include "doc/slice.h"

namespace app {
  class Editor;

  class MovingSliceState : public StandbyState {
  public:
    MovingSliceState(Editor* editor,
                     ui::MouseMessage* msg,
                     const EditorHit& hit,
                     const doc::SelectedObjects& selectedSlices);

    void onEnterState(Editor* editor) override;
    bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;

    bool requireBrushPreview() override { return false; }

  private:
    using DrawExtraCelContentFunc = std::function<void(const gfx::Rect& bounds, Image* dst)>;

    struct Item {
      doc::Slice* slice;
      doc::SliceKey oldKey;
      doc::SliceKey newKey;
      // Images containing the parts of each selected layer of the sprite under
      // the slice bounds that will be transformed when Slice Transform is
      // enabled
      std::vector<ImageRef> imgs;
      // Masks for each of the images in imgs vector
      std::vector<MaskRef> masks;

      // Image containing the result of merging all the images in the imgs
      // vector
      ImageRef mergedImg = nullptr;
      MaskRef mergedMask = nullptr;

      ~Item() {
        if (!masks.empty() && mergedMask != masks[0])
          mergedMask->unfreeze();
        for (auto& m : masks)
          m->unfreeze();
      }
    };

    Item getItemForSlice(doc::Slice* slice);
    gfx::Rect selectedSlicesBounds() const;

    void drawSliceContents();
    void drawSliceContentsByLayer(int layerIdx);
    void drawExtraCel(const gfx::Rect& bounds, DrawExtraCelContentFunc drawContent);
    void drawImage(doc::Image* dst,
                   const doc::Image* src,
                   const doc::Mask* mask,
                   const gfx::Rect& bounds);
    void stampExtraCelImage();

    void clearSlices();

    doc::frame_t m_frame;
    EditorHit m_hit;
    gfx::Point m_mouseStart;
    std::vector<Item> m_items;
    LayerList m_selectedLayers;
    Site m_site;
    ExtraCelRef m_extraCel = nullptr;
    Tx m_tx;
  };

} // namespace app

#endif
