// Aseprite Document Library
// Copyright (c) 2025 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/image.h"

#include "doc/image_io.h"

namespace doc {

ImageImplBase::ImageImplBase(const ImageSpec& spec, const ImageBufferPtr& buffer)
  : Image(spec)
  , m_buffer(buffer)
{
}

int ImageImplBase::getMemSize() const
{
  return sizeof(ImageImplBase) + size_t(m_stream ? (size_t)m_stream->tellp() : 0) +
         (m_buffer ? m_buffer->size() : 0);
}

void ImageImplBase::suspendObject()
{
  ASSERT(m_buffer);
  ASSERT(!m_stream);

  m_stream = std::make_unique<std::stringstream>();
  write_image_pixels(*m_stream, this);
  m_buffer.reset();

  Image::suspendObject();
}

void ImageImplBase::restoreObject()
{
  Image::restoreObject();

  ASSERT(!m_buffer);
  ASSERT(m_stream);

  initialize();

  m_stream->seekp(0);
  read_image_pixels(*m_stream, this);
  m_stream.reset();
}

void copy_bitmaps(Image* dst, const Image* src, gfx::Clip area)
{
  if (!area.clip(dst->width(), dst->height(), src->width(), src->height()))
    return;

  // Copy process
  ImageConstIterator<BitmapTraits> src_it(src, area.srcBounds(), area.src.x, area.src.y);
  ImageIterator<BitmapTraits> dst_it(dst, area.dstBounds(), area.dst.x, area.dst.y);

  int end_x = area.dst.x + area.size.w;

  for (int end_y = area.dst.y + area.size.h; area.dst.y < end_y; ++area.dst.y, ++area.src.y) {
    for (int x = area.dst.x; x < end_x; ++x) {
      *dst_it = *src_it;
      ++src_it;
      ++dst_it;
    }
  }
}

} // namespace doc
