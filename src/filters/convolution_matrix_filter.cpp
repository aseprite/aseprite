/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include "filters/convolution_matrix_filter.h"

#include "filters/convolution_matrix.h"
#include "filters/filter_indexed_data.h"
#include "filters/filter_manager.h"
#include "filters/neighboring_pixels.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rgbmap.h"

ConvolutionMatrixFilter::ConvolutionMatrixFilter()
  : m_matrix(NULL)
  , m_tiledMode(TILED_NONE)
{
}

void ConvolutionMatrixFilter::setMatrix(const SharedPtr<ConvolutionMatrix>& matrix)
{
  m_matrix = matrix;
  m_lines.resize(matrix->getHeight());
}

void ConvolutionMatrixFilter::setTiledMode(TiledMode tiledMode)
{
  m_tiledMode = tiledMode;
}

namespace {

  struct GetPixelsDelegate
  {
    ase_uint32 color;
    int div;
    const int* matrixData;

    void reset(const ConvolutionMatrix* matrix) {
      div = matrix->getDiv();
      matrixData = &matrix->value(0, 0);
    }

  };

  struct GetPixelsDelegateRgba : public GetPixelsDelegate
  {
    int r, g, b, a;

    void reset(const ConvolutionMatrix* matrix) {
      GetPixelsDelegate::reset(matrix);
      r = g = b = a = 0;
    }

    void operator()(RgbTraits::pixel_t color)
    {
      if (*matrixData) {
	if (_rgba_geta(color) == 0)
	  div -= *matrixData;
	else {
	  r += _rgba_getr(color) * (*matrixData);
	  g += _rgba_getg(color) * (*matrixData);
	  b += _rgba_getb(color) * (*matrixData);
	  a += _rgba_geta(color) * (*matrixData);
	}
      }
      matrixData++;
    }

  };

  struct GetPixelsDelegateGrayscale : public GetPixelsDelegate
  {
    int v, a;

    void reset(const ConvolutionMatrix* matrix) {
      GetPixelsDelegate::reset(matrix);
      v = a = 0;
    }

    void operator()(GrayscaleTraits::pixel_t color)
    {
      if (*matrixData) {
	if (_graya_geta(color) == 0)
	  div -= *matrixData;
	else {
	  v += _graya_getv(color) * (*matrixData);
	  a += _graya_geta(color) * (*matrixData);
	}
      }
      matrixData++;
    }

  };

  struct GetPixelsDelegateIndexed : public GetPixelsDelegate
  {
    const Palette* pal;
    int r, g, b, index;

    GetPixelsDelegateIndexed(const Palette* pal) : pal(pal) { }

    void reset(const ConvolutionMatrix* matrix) {
      GetPixelsDelegate::reset(matrix);
      r = g = b = index = 0;
    }

    void operator()(GrayscaleTraits::pixel_t color)
    {
      if (*matrixData) {
	r += _rgba_getr(pal->getEntry(color)) * (*matrixData);
	g += _rgba_getg(pal->getEntry(color)) * (*matrixData);
	b += _rgba_getb(pal->getEntry(color)) * (*matrixData);
	index += color * (*matrixData);
      }
      matrixData++;
    }

  };

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
  ase_uint32* dst_address = (ase_uint32*)filterMgr->getDestinationAddress();
  Target target = filterMgr->getTarget();
  ase_uint32 color;
  GetPixelsDelegateRgba delegate;
  int x = filterMgr->getX();
  int x2 = x+filterMgr->getWidth();
  int y = filterMgr->getY();

  for (; x<x2; ++x) {
    // Avoid the non-selected region
    if (filterMgr->skipPixel()) {
      ++dst_address;
      continue;
    }

    delegate.reset(m_matrix);
    get_neighboring_pixels<RgbTraits>(src, x, y,
				      m_matrix->getWidth(),
				      m_matrix->getHeight(),
				      m_matrix->getCenterX(),
				      m_matrix->getCenterY(),
				      m_tiledMode, delegate);

    color = image_getpixel_fast<RgbTraits>(src, x, y);
    if (delegate.div == 0) {
      *(dst_address++) = color;
      continue;
    }

    if (target & TARGET_RED_CHANNEL) {
      delegate.r = delegate.r / delegate.div + m_matrix->getBias();
      delegate.r = MID(0, delegate.r, 255);
    }
    else
      delegate.r = _rgba_getr(color);

    if (target & TARGET_GREEN_CHANNEL) {
      delegate.g = delegate.g / delegate.div + m_matrix->getBias();
      delegate.g = MID(0, delegate.g, 255);
    }
    else
      delegate.g = _rgba_getg(color);

    if (target & TARGET_BLUE_CHANNEL) {
      delegate.b = delegate.b / delegate.div + m_matrix->getBias();
      delegate.b = MID(0, delegate.b, 255);
    }
    else
      delegate.b = _rgba_getb(color);

    if (target & TARGET_ALPHA_CHANNEL) {
      delegate.a = delegate.a / m_matrix->getDiv() + m_matrix->getBias();
      delegate.a = MID(0, delegate.a, 255);
    }
    else
      delegate.a = _rgba_geta(color);

    *(dst_address++) = _rgba(delegate.r, delegate.g, delegate.b, delegate.a);
  }
}

void ConvolutionMatrixFilter::applyToGrayscale(FilterManager* filterMgr)
{
  if (!m_matrix)
    return;

  const Image* src = filterMgr->getSourceImage();
  ase_uint16* dst_address = (ase_uint16*)filterMgr->getDestinationAddress();
  Target target = filterMgr->getTarget();
  ase_uint16 color;
  GetPixelsDelegateGrayscale delegate;
  int x = filterMgr->getX();
  int x2 = x+filterMgr->getWidth();
  int y = filterMgr->getY();

  for (; x<x2; ++x) {
    // Avoid the non-selected region
    if (filterMgr->skipPixel()) {
      ++dst_address;
      continue;
    }

    delegate.reset(m_matrix);
    get_neighboring_pixels<GrayscaleTraits>(src, x, y,
					    m_matrix->getWidth(),
					    m_matrix->getHeight(),
					    m_matrix->getCenterX(),
					    m_matrix->getCenterY(),
					    m_tiledMode, delegate);

    color = image_getpixel_fast<GrayscaleTraits>(src, x, y);
    if (delegate.div == 0) {
      *(dst_address++) = color;
      continue;
    }
    
    if (target & TARGET_GRAY_CHANNEL) {
      delegate.v = delegate.v / delegate.div + m_matrix->getBias();
      delegate.v = MID(0, delegate.v, 255);
    }
    else
      delegate.v = _graya_getv(color);

    if (target & TARGET_ALPHA_CHANNEL) {
      delegate.a = delegate.a / m_matrix->getDiv() + m_matrix->getBias();
      delegate.a = MID(0, delegate.a, 255);
    }
    else
      delegate.a = _graya_geta(color);

    *(dst_address++) = _graya(delegate.v, delegate.a);
  }
}

void ConvolutionMatrixFilter::applyToIndexed(FilterManager* filterMgr)
{
  if (!m_matrix)
    return;

  const Image* src = filterMgr->getSourceImage();
  ase_uint8* dst_address = (ase_uint8*)filterMgr->getDestinationAddress();
  const Palette* pal = filterMgr->getIndexedData()->getPalette();
  const RgbMap* rgbmap = filterMgr->getIndexedData()->getRgbMap();
  Target target = filterMgr->getTarget();
  ase_uint8 color;
  GetPixelsDelegateIndexed delegate(pal);
  int x = filterMgr->getX();
  int x2 = x+filterMgr->getWidth();
  int y = filterMgr->getY();

  for (; x<x2; ++x) {
    // Avoid the non-selected region
    if (filterMgr->skipPixel()) {
      ++dst_address;
      continue;
    }

    delegate.reset(m_matrix);
    get_neighboring_pixels<IndexedTraits>(src, x, y,
					  m_matrix->getWidth(),
					  m_matrix->getHeight(),
					  m_matrix->getCenterX(),
					  m_matrix->getCenterY(),
					  m_tiledMode, delegate);

    color = image_getpixel_fast<IndexedTraits>(src, x, y);
    if (delegate.div == 0) {
      *(dst_address++) = color;
      continue;
    }

    if (target & TARGET_INDEX_CHANNEL) {
      delegate.index = delegate.index / m_matrix->getDiv() + m_matrix->getBias();
      delegate.index = MID(0, delegate.index, 255);

      *(dst_address++) = delegate.index;
    }
    else {
      if (target & TARGET_RED_CHANNEL) {
	delegate.r = delegate.r / delegate.div + m_matrix->getBias();
	delegate.r = MID(0, delegate.r, 255);
      }
      else
	delegate.r = _rgba_getr(pal->getEntry(color));

      if (target & TARGET_GREEN_CHANNEL) {
	delegate.g =  delegate.g / delegate.div + m_matrix->getBias();
	delegate.g = MID(0, delegate.g, 255);
      }
      else
	delegate.g = _rgba_getg(pal->getEntry(color));

      if (target & TARGET_BLUE_CHANNEL) {
	delegate.b = delegate.b / delegate.div + m_matrix->getBias();
	delegate.b = MID(0, delegate.b, 255);
      }
      else
	delegate.b = _rgba_getb(pal->getEntry(color));

      *(dst_address++) = rgbmap->mapColor(delegate.r, delegate.g, delegate.b);
    }
  }
}
