// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#pragma once

#include "base/exception.h"
#include "base/unique_ptr.h"
#include "doc/image_impl.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"
#include "doc/site.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "gfx/rect.h"

#include <cstring>

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class Mask;
  class Sprite;
}

namespace filters {
  class Filter;
}

namespace app {
  class Context;
  class Document;
  class Transaction;

  using namespace filters;

  class InvalidAreaException : public base::Exception {
  public:
    InvalidAreaException() throw()
    : base::Exception("The current mask/area to apply the effect is completelly invalid.") { }
  };

  class NoImageException : public base::Exception {
  public:
    NoImageException() throw()
    : base::Exception("There is not an active image to apply the effect.\n"
                      "Please select a layer/cel with an image and try again.") { }
  };

  class FilterManagerImpl : public FilterManager
                          , public FilterIndexedData {
  public:
    // Interface to report progress to the user and take input from him
    // to cancel the whole process.
    class IProgressDelegate {
    public:
      virtual ~IProgressDelegate() { }

      // Called to report the progress of the filter (with progress from 0.0 to 1.0).
      virtual void reportProgress(float progress) = 0;

      // Should return true if the user wants to cancel the filter.
      virtual bool isCancelled() = 0;
    };

    FilterManagerImpl(Context* context, Filter* filter);
    ~FilterManagerImpl();

    void setProgressDelegate(IProgressDelegate* progressDelegate);

    doc::PixelFormat pixelFormat() const;

    void setTarget(Target target);

    void begin();
    void beginForPreview();
    void end();
    bool applyStep();
    void applyToTarget();

    app::Document* document();
    doc::Sprite* sprite() { return m_site.sprite(); }
    doc::Layer* layer() { return m_site.layer(); }
    doc::frame_t frame() { return m_site.frame(); }
    doc::Image* destinationImage() const { return m_dst.get(); }
    gfx::Point position() const { return gfx::Point(0, 0); }

    // Updates the current editor to show the progress of the preview.
    void flush();

    // FilterManager implementation
    const void* getSourceAddress() override;
    void* getDestinationAddress() override;
    int getWidth() override { return m_bounds.w; }
    Target getTarget() override { return m_target; }
    FilterIndexedData* getIndexedData() override { return this; }
    bool skipPixel() override;
    const doc::Image* getSourceImage() override { return m_src.get(); }
    int x() const override { return m_bounds.x; }
    int y() const override { return m_bounds.y+m_row; }
    bool isFirstRow() const override { return m_row == 0; }

    // FilterIndexedData implementation
    const doc::Palette* getPalette() const override;
    const doc::RgbMap* getRgbMap() const override;
    doc::Palette* getNewPalette() override;
    doc::PalettePicks getPalettePicks() override;

  private:
    void init(doc::Cel* cel);
    void apply(Transaction& transaction);
    void applyToCel(Transaction& transaction, doc::Cel* cel);
    bool updateBounds(doc::Mask* mask);
    bool paletteHasChanged();
    void restoreSpritePalette();

    Context* m_context;
    doc::Site m_site;
    Filter* m_filter;
    doc::Cel* m_cel;
    doc::ImageRef m_src;
    doc::ImageRef m_dst;
    int m_row;
    int m_nextRowToFlush;
    gfx::Rect m_bounds;
    doc::Mask* m_mask;
    base::UniquePtr<doc::Mask> m_previewMask;
    doc::ImageBits<doc::BitmapTraits> m_maskBits;
    doc::ImageBits<doc::BitmapTraits>::iterator m_maskIterator;
    Target m_targetOrig;          // Original targets
    Target m_target;              // Filtered targets
    base::UniquePtr<doc::Palette> m_oldPalette;

    // Hooks
    float m_progressBase;
    float m_progressWidth;
    IProgressDelegate* m_progressDelegate;
  };

} // namespace app

#endif
