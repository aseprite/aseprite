// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters/convolution_matrix_filter.h"

#include "base/clamp.h"
#include "filters/convolution_matrix.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "filters/neighboring_pixels.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"

namespace filters {

using namespace doc;

namespace {

  struct GetPixelsDelegate {
    int div;
    const int* matrixData;

    void reset(const ConvolutionMatrix* matrix) {
      div = matrix->getDiv();
      matrixData = &matrix->value(0, 0);
    }
  };

  struct GetPixelsDelegateRgba : public GetPixelsDelegate {
    int r, g, b, a;

    void reset(const ConvolutionMatrix* matrix) {
      GetPixelsDelegate::reset(matrix);
      r = g = b = a = 0;
    }

    void operator()(RgbTraits::pixel_t color) {
      if (*matrixData) {
        if (rgba_geta(color) == 0)
          div -= *matrixData;
        else {
          r += rgba_getr(color) * (*matrixData);
          g += rgba_getg(color) * (*matrixData);
          b += rgba_getb(color) * (*matrixData);
          a += rgba_geta(color) * (*matrixData);
        }
      }
      matrixData++;
    }
  };

  struct GetPixelsDelegateGrayscale : public GetPixelsDelegate {
    int v, a;

    void reset(const ConvolutionMatrix* matrix) {
      GetPixelsDelegate::reset(matrix);
      v = a = 0;
    }

    void operator()(GrayscaleTraits::pixel_t color) {
      if (*matrixData) {
        if (graya_geta(color) == 0)
          div -= *matrixData;
        else {
          v += graya_getv(color) * (*matrixData);
          a += graya_geta(color) * (*matrixData);
        }
      }
      matrixData++;
    }
  };

  struct GetPixelsDelegateIndexed : public GetPixelsDelegate {
    const Palette* pal;
    int r, g, b, a, index;

    GetPixelsDelegateIndexed(const Palette* pal) : pal(pal) { }

    void reset(const ConvolutionMatrix* matrix) {
      GetPixelsDelegate::reset(matrix);
      r = g = b = a = index = 0;
    }

    void operator()(IndexedTraits::pixel_t color) {
      if (*matrixData) {
        index += color * (*matrixData);
        color_t rgba = pal->getEntry(color);
        if (rgba_geta(rgba) == 0)
          div -= *matrixData;
        else {
          r += rgba_getr(rgba) * (*matrixData);
          g += rgba_getg(rgba) * (*matrixData);
          b += rgba_getb(rgba) * (*matrixData);
          a += rgba_geta(rgba) * (*matrixData);
        }
      }
      matrixData++;
    }
  };

}

ConvolutionMatrixFilter::ConvolutionMatrixFilter()
  : m_matrix(NULL)
  , m_tiledMode(TiledMode::NONE)
{
}

void ConvolutionMatrixFilter::setMatrix(const std::shared_ptr<ConvolutionMatrix>& matrix)
{
  m_matrix = matrix;
}

void ConvolutionMatrixFilter::setTiledMode(TiledMode tiledMode)
{
  m_tiledMode = tiledMode;
}

const char* ConvolutionMatrixFilter::getName()
{
  return "Convolution Matrix";
}

void ConvolutionMatrixFilter::applyToRgba(FilterManager* filterMgr)
{
  if (!m_matrix)
    return;

  const Image* src = filterMgr->getSourceImage();
  uint32_t* dst_address = (uint32_t*)filterMgr->getDestinationAddress();
  Target target = filterMgr->getTarget();
  uint32_t color;
  GetPixelsDelegateRgba delegate;
  int x = filterMgr->x();
  int x2 = x+filterMgr->getWidth();
  int y = filterMgr->y();

  for (; x<x2; ++x) {
    // Avoid the non-selected region
    if (filterMgr->skipPixel()) {
      ++dst_address;
      continue;
    }

    delegate.reset(m_matrix.get());
    get_neighboring_pixels<RgbTraits>(src, x, y,
                                      m_matrix->getWidth(),
                                      m_matrix->getHeight(),
                                      m_matrix->getCenterX(),
                                      m_matrix->getCenterY(),
                                      m_tiledMode, delegate);

    color = get_pixel_fast<RgbTraits>(src, x, y);
    if (delegate.div == 0) {
      *(dst_address++) = color;
      continue;
    }

    if (target & TARGET_RED_CHANNEL) {
      delegate.r = delegate.r / delegate.div + m_matrix->getBias();
      delegate.r = base::clamp(delegate.r, 0, 255);
    }
    else
      delegate.r = rgba_getr(color);

    if (target & TARGET_GREEN_CHANNEL) {
      delegate.g = delegate.g / delegate.div + m_matrix->getBias();
      delegate.g = base::clamp(delegate.g, 0, 255);
    }
    else
      delegate.g = rgba_getg(color);

    if (target & TARGET_BLUE_CHANNEL) {
      delegate.b = delegate.b / delegate.div + m_matrix->getBias();
      delegate.b = base::clamp(delegate.b, 0, 255);
    }
    else
      delegate.b = rgba_getb(color);

    if (target & TARGET_ALPHA_CHANNEL) {
      delegate.a = delegate.a / m_matrix->getDiv() + m_matrix->getBias();
      delegate.a = base::clamp(delegate.a, 0, 255);
    }
    else
      delegate.a = rgba_geta(color);

    *(dst_address++) = rgba(delegate.r, delegate.g, delegate.b, delegate.a);
  }
}

void ConvolutionMatrixFilter::applyToGrayscale(FilterManager* filterMgr)
{
  if (!m_matrix)
    return;

  const Image* src = filterMgr->getSourceImage();
  uint16_t* dst_address = (uint16_t*)filterMgr->getDestinationAddress();
  Target target = filterMgr->getTarget();
  uint16_t color;
  GetPixelsDelegateGrayscale delegate;
  int x = filterMgr->x();
  int x2 = x+filterMgr->getWidth();
  int y = filterMgr->y();

  for (; x<x2; ++x) {
    // Avoid the non-selected region
    if (filterMgr->skipPixel()) {
      ++dst_address;
      continue;
    }

    delegate.reset(m_matrix.get());
    get_neighboring_pixels<GrayscaleTraits>(src, x, y,
                                            m_matrix->getWidth(),
                                            m_matrix->getHeight(),
                                            m_matrix->getCenterX(),
                                            m_matrix->getCenterY(),
                                            m_tiledMode, delegate);

    color = get_pixel_fast<GrayscaleTraits>(src, x, y);
    if (delegate.div == 0) {
      *(dst_address++) = color;
      continue;
    }

    if (target & TARGET_GRAY_CHANNEL) {
      delegate.v = delegate.v / delegate.div + m_matrix->getBias();
      delegate.v = base::clamp(delegate.v, 0, 255);
    }
    else
      delegate.v = graya_getv(color);

    if (target & TARGET_ALPHA_CHANNEL) {
      delegate.a = delegate.a / m_matrix->getDiv() + m_matrix->getBias();
      delegate.a = base::clamp(delegate.a, 0, 255);
    }
    else
      delegate.a = graya_geta(color);

    *(dst_address++) = graya(delegate.v, delegate.a);
  }
}

void ConvolutionMatrixFilter::applyToIndexed(FilterManager* filterMgr)
{
  if (!m_matrix)
    return;

  const Image* src = filterMgr->getSourceImage();
  uint8_t* dst_address = (uint8_t*)filterMgr->getDestinationAddress();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  Target target = filterMgr->getTarget();
  uint8_t color;
  GetPixelsDelegateIndexed delegate(pal);
  int x = filterMgr->x();
  int x2 = x+filterMgr->getWidth();
  int y = filterMgr->y();

  for (; x<x2; ++x) {
    // Avoid the non-selected region
    if (filterMgr->skipPixel()) {
      ++dst_address;
      continue;
    }

    delegate.reset(m_matrix.get());
    get_neighboring_pixels<IndexedTraits>(src, x, y,
                                          m_matrix->getWidth(),
                                          m_matrix->getHeight(),
                                          m_matrix->getCenterX(),
                                          m_matrix->getCenterY(),
                                          m_tiledMode, delegate);

    color = get_pixel_fast<IndexedTraits>(src, x, y);
    if (delegate.div == 0) {
      *(dst_address++) = color;
      continue;
    }

    if (target & TARGET_INDEX_CHANNEL) {
      delegate.index = delegate.index / m_matrix->getDiv() + m_matrix->getBias();
      delegate.index = base::clamp(delegate.index, 0, 255);

      *(dst_address++) = delegate.index;
    }
    else {
      color = pal->getEntry(color);

      if (target & TARGET_RED_CHANNEL) {
        delegate.r = delegate.r / delegate.div + m_matrix->getBias();
        delegate.r = base::clamp(delegate.r, 0, 255);
      }
      else
        delegate.r = rgba_getr(color);

      if (target & TARGET_GREEN_CHANNEL) {
        delegate.g =  delegate.g / delegate.div + m_matrix->getBias();
        delegate.g = base::clamp(delegate.g, 0, 255);
      }
      else
        delegate.g = rgba_getg(color);

      if (target & TARGET_BLUE_CHANNEL) {
        delegate.b = delegate.b / delegate.div + m_matrix->getBias();
        delegate.b = base::clamp(delegate.b, 0, 255);
      }
      else
        delegate.b = rgba_getb(color);

      if (target & TARGET_ALPHA_CHANNEL) {
        delegate.a = delegate.a / delegate.div + m_matrix->getBias();
        delegate.a = base::clamp(delegate.a, 0, 255);
      }
      else
        delegate.a = rgba_geta(color);

      *(dst_address++) = rgbmap->mapColor(delegate.r, delegate.g, delegate.b, delegate.a);
    }
  }
}

} // namespace filters
