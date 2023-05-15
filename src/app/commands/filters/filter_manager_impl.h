// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_MANAGER_IMPL_H_INCLUDED
#pragma once

#include "app/commands/filters/cels_target.h"
#include "app/context_access.h"
#include "app/site.h"
#include "app/tx.h"
#include "base/exception.h"
#include "base/task.h"
#include "doc/image_impl.h"
#include "doc/image_ref.h"
#include "doc/pixel_format.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "gfx/rect.h"

#include <cstring>
#include <memory>
#include <vector>

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
  class Doc;
  class Editor;

  using namespace filters;

  class InvalidAreaException : public base::Exception {
  public:
    InvalidAreaException() throw()
    : base::Exception("The active selection to apply the effect is out of the canvas.") { }
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
    class IProgressDelegate {   // TODO replace this with base::task_token
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

    void setTarget(Target target);
    void setCelsTarget(CelsTarget celsTarget);

    void begin();
#ifdef ENABLE_UI
    void beginForPreview();
#endif
    void end();
    bool applyStep();
    void applyToTarget();

    void initTransaction();
    bool isTransaction() const;
    void commitTransaction();

    Doc* document();
    doc::Sprite* sprite() { return m_site.sprite(); }
    doc::Layer* layer() { return m_site.layer(); }
    doc::frame_t frame() { return m_site.frame(); }
    doc::Image* destinationImage() const { return m_dst.get(); }
    gfx::Point position() const { return gfx::Point(0, 0); }

#ifdef ENABLE_UI
    // Updates the current editor to show the progress of the preview.
    void flush();
    void disablePreview();
#endif
    void setTaskToken(base::task_token& token);

    // FilterManager implementation
    doc::PixelFormat pixelFormat() const override;
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
    bool isMaskActive() const override;
    base::task_token& taskToken() const override;

    // FilterIndexedData implementation
    const doc::Palette* getPalette() const override;
    const doc::RgbMap* getRgbMap() const override;
    doc::Palette* getNewPalette() override;
    doc::PalettePicks getPalettePicks() override;

  private:
    void init(doc::Cel* cel);
    void apply();
    void applyToCel(doc::Cel* cel);
    bool updateBounds(doc::Mask* mask);

    // Returns true if the palette was changed (true when the filter
    // modifies the palette).
    bool paletteHasChanged();
    void restoreSpritePalette();
    void applyToPaletteIfNeeded();

#ifdef ENABLE_UI
    void redrawColorPalette();
#endif

    ContextReader m_reader;
    std::unique_ptr<ContextWriter> m_writer;
    Site& m_site;
    Filter* m_filter;
    doc::Cel* m_cel;
    doc::ImageRef m_src;
    doc::ImageRef m_dst;
    int m_row;
#ifdef ENABLE_UI
    int m_nextRowToFlush;
#endif
    gfx::Rect m_bounds;
    doc::Mask* m_mask;
    std::unique_ptr<doc::Mask> m_previewMask;
    doc::ImageBits<doc::BitmapTraits> m_maskBits;
    doc::ImageBits<doc::BitmapTraits>::iterator m_maskIterator;
    Target m_targetOrig;          // Original targets
    Target m_target;              // Filtered targets
    CelsTarget m_celsTarget;
    std::unique_ptr<doc::Palette> m_oldPalette;
    std::unique_ptr<Tx> m_tx;
    base::task_token m_noToken;
    base::task_token* m_taskToken;

    // Hooks
    float m_progressBase;
    float m_progressWidth;
    IProgressDelegate* m_progressDelegate;
  };

} // namespace app

#endif
