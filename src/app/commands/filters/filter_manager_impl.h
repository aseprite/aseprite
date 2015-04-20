// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#pragma once

#include "base/exception.h"
#include "base/unique_ptr.h"
#include "doc/image_bits.h"
#include "doc/image_traits.h"
#include "doc/pixel_format.h"
#include "doc/site.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"

#include <cstring>

namespace doc {
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
    doc::Image* destinationImage() const { return m_dst; }

    // Updates the current editor to show the progress of the preview.
    void flush();

    // FilterManager implementation
    const void* getSourceAddress();
    void* getDestinationAddress();
    int getWidth() { return m_w; }
    Target getTarget() { return m_target; }
    FilterIndexedData* getIndexedData() { return this; }
    bool skipPixel();
    const doc::Image* getSourceImage() { return m_src; }
    int x() { return m_x; }
    int y() { return m_y+m_row; }

    // FilterIndexedData implementation
    doc::Palette* getPalette();
    doc::RgbMap* getRgbMap();

  private:
    void init(const doc::Layer* layer, doc::Image* image, int offset_x, int offset_y);
    void apply(Transaction& transaction);
    void applyToImage(Transaction& transaction, doc::Layer* layer, doc::Image* image, int x, int y);
    bool updateMask(doc::Mask* mask, const doc::Image* image);

    Context* m_context;
    doc::Site m_site;
    Filter* m_filter;
    doc::Image* m_src;
    base::UniquePtr<doc::Image> m_dst;
    int m_row;
    int m_x, m_y, m_w, m_h;
    int m_offset_x, m_offset_y;
    doc::Mask* m_mask;
    base::UniquePtr<doc::Mask> m_preview_mask;
    doc::ImageBits<doc::BitmapTraits> m_maskBits;
    doc::ImageBits<doc::BitmapTraits>::iterator m_maskIterator;
    Target m_targetOrig;          // Original targets
    Target m_target;              // Filtered targets

    // Hooks
    float m_progressBase;
    float m_progressWidth;
    IProgressDelegate* m_progressDelegate;
  };

} // namespace app

#endif
