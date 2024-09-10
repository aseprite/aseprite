// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
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
#include "app/util/new_image_from_mask.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "doc/mask.h"
#include "doc/selected_layers.h"
#include "doc/selected_objects.h"
#include "doc/slice.h"
#include "gfx/border.h"

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
    struct Item;
    using ItemContentPartFunc = std::function<
                                 void(const doc::Image* src,
                                      const doc::Mask* mask,
                                      const gfx::Rect& bounds)>;

    class ItemContent {
    public:
      ItemContent(const Item *item) : m_item(item) {}
      virtual ~ItemContent() {};
      virtual void forEachPart(ItemContentPartFunc fn) = 0;
      virtual void copy(const Image* src) = 0;

    protected:
      const Item* m_item = nullptr;
    };

    using ItemContentRef = std::shared_ptr<ItemContent>;

    class SingleSlice : public ItemContent {
    public:
      SingleSlice(const Item *item,
                  const ImageRef& image) : SingleSlice(item, image, item->oldKey.bounds().origin()) {
      }

      SingleSlice(const Item *item,
                  const ImageRef& image,
                  const gfx::Point& origin) : ItemContent(item)
                                       , m_img(image) {
        m_mask = std::make_shared<Mask>();
        m_mask->freeze();
        m_mask->fromImage(m_img.get(), origin);
      }

      ~SingleSlice() {
        m_mask->unfreeze();
      }

      void forEachPart(ItemContentPartFunc fn) override {
        fn(m_img.get(), m_mask.get(), m_item->newKey.bounds());
      }

      void copy(const Image* src) override {
        doc::Mask srcMask;
        srcMask.freeze();
        srcMask.add(m_item->oldKey.bounds());
        // TODO: See if part of the code in fromImage can be replaced or
        // refactored to use mask_image (in cel_ops.h)?
        srcMask.fromImage(src, srcMask.origin());
        copy_masked_zones(m_img.get(), src, &srcMask, srcMask.bounds().x, srcMask.bounds().y);

        m_mask->add(srcMask);
        srcMask.unfreeze();
      }

      const Image* image() { return m_img.get(); }
      Mask* mask() { return m_mask.get(); }

    private:
      // Images containing the parts of each selected layer of the sprite under
      // the slice bounds that will be transformed when Slice Transform is
      // enabled
      ImageRef m_img;
      // Masks for each of the images in imgs vector
      MaskRef m_mask;
    };

    class NineSlice : public ItemContent {
    public:
      NineSlice(const Item *item,
                const ImageRef& image) : ItemContent(item) {

        if (!m_item->oldKey.hasCenter()) return;

        const gfx::Rect totalBounds(m_item->oldKey.bounds().size());
        gfx::Rect bounds[9];
        totalBounds.nineSlice(m_item->oldKey.center(), bounds);
        for (int i=0; i<9; ++i) {
          if (!bounds[i].isEmpty()) {
            ImageRef img;
            img.reset(Image::create(image->pixelFormat(), bounds[i].w, bounds[i].h));
            img->copy(image.get(), gfx::Clip(0, 0, bounds[i]));
            m_part[i] = std::make_unique<SingleSlice>(m_item, img, bounds[i].origin());
          }
        }
      }

      ~NineSlice() {}

      void forEachPart(ItemContentPartFunc fn) override {
        gfx::Rect bounds[9];
        m_item->newKey.bounds().nineSlice(m_item->newKey.center(), bounds);
        for (int i=0; i<9; ++i) {
          if (m_part[i])
            fn(m_part[i]->image(), m_part[i]->mask(),  bounds[i]);
        }
      }

      void copy(const Image* src) override {
        if (!m_item->oldKey.hasCenter()) return;

        const gfx::Rect totalBounds(m_item->oldKey.bounds().size());
        gfx::Rect bounds[9];
        totalBounds.nineSlice(m_item->oldKey.center(), bounds);
        for (int i=0; i<9; ++i) {
          if (!bounds[i].isEmpty()) {
            ImageRef img;
            img.reset(Image::create(src->pixelFormat(), bounds[i].w, bounds[i].h));
            img->copy(src, gfx::Clip(0, 0, bounds[i]));
            m_part[i]->copy(img.get());
          }
        }
      }

    private:
      std::unique_ptr<SingleSlice> m_part[9] = {nullptr, nullptr, nullptr,
                                                nullptr, nullptr, nullptr,
                                                nullptr, nullptr, nullptr};
    };


    struct Item {
      doc::Slice* slice;
      doc::SliceKey oldKey;
      doc::SliceKey newKey;
      // Vector of each selected layer's part of the sprite under
      // the slice bounds that will be transformed when Slice Transform is
      // enabled. Contains one ItemContentRef by layer.
      std::vector<ItemContentRef> content;
      // Part of the sprite of each selected layer's merged into one image per
      // slice. This is used to give feedback to the users when they are
      // transforming the selected slices.
      ItemContentRef mergedContent;

      // Adds image to the content vector. The image should correspond to some
      // part of a single layer cel's image.
      // Internally this method builds a merged version of the images to speed
      // up the drawing when the user updates this Item's slice in real time.
      void pushContent(const ImageRef& image) {
        if (content.empty()) {
          const gfx::Rect& srcBounds = image->bounds();
          ImageRef mergedImage;
          mergedImage.reset(Image::create(image->pixelFormat(), srcBounds.w, srcBounds.h));
          mergedImage->clear(image->maskColor());
          mergedContent = (this->oldKey.hasCenter() ? (ItemContentRef)std::make_shared<NineSlice>(this, mergedImage)
                                                    : std::make_shared<SingleSlice>(this, mergedImage));
        }

        mergedContent->copy(image.get());

        ItemContentRef ssc = (this->oldKey.hasCenter() ? (ItemContentRef)std::make_shared<NineSlice>(this, image)
                                                       : std::make_shared<SingleSlice>(this, image));
        content.push_back(ssc);
      }

      bool isNineSlice() const { return oldKey.hasCenter(); }

      // If this item manages a 9-slice key, returns the border surrounding the
      // center rectangle of the SliceKey. Otherwise returns an empty border.
      gfx::Border border() const {
        gfx::Border border;
        if (isNineSlice()) {
          border = gfx::Border(oldKey.center().x,
                               oldKey.center().y,
                               oldKey.bounds().w-oldKey.center().x2(),
                               oldKey.bounds().h-oldKey.center().y2());
        }
        return border;
      }
    };

    // Initializes the content of the Items. So each item will contain the
    // part of the cel's layers within the corresponding slice.
    void initializeItemsContent();

    Item getItemForSlice(doc::Slice* slice);
    gfx::Rect selectedSlicesBounds() const;

    void drawExtraCel(int layerIdx = -1);
    void drawItem(doc::Image* dst,
                  const Item& item,
                  const gfx::Point& itemsBoundsOrigin,
                  int layerIdx);
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
