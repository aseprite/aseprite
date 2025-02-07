// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/copy_rect.h"

#include "doc/image.h"

#include <algorithm>

namespace app { namespace cmd {

CopyRect::CopyRect(Image* dst, const Image* src, const gfx::Clip& clip)
  : WithImage(dst)
  , m_clip(clip)
{
  if (!m_clip.clip(dst->width(), dst->height(), src->width(), src->height()))
    return;

  // Fill m_data with "src" data

  int lineSize = src->bytesPerPixel() * m_clip.size.w;
  m_data.resize(lineSize * m_clip.size.h);

  auto it = m_data.begin();
  for (int v = 0; v < m_clip.size.h; ++v) {
    uint8_t* addr = src->getPixelAddress(m_clip.dst.x, m_clip.dst.y + v);

    std::copy(addr, addr + lineSize, it);
    it += lineSize;
  }
}

void CopyRect::onExecute()
{
  swap();
}

void CopyRect::onUndo()
{
  swap();
}

void CopyRect::onRedo()
{
  swap();
}

void CopyRect::swap()
{
  if (m_clip.size.w < 1 || m_clip.size.h < 1)
    return;

  Image* image = this->image();
  int lineSize = this->lineSize();
  std::vector<uint8_t> tmp(lineSize);

  auto it = m_data.begin();
  for (int v = 0; v < m_clip.size.h; ++v) {
    uint8_t* addr = image->getPixelAddress(m_clip.dst.x, m_clip.dst.y + v);

    std::copy(addr, addr + lineSize, tmp.begin());
    std::copy(it, it + lineSize, addr);
    std::copy(tmp.begin(), tmp.end(), it);

    it += lineSize;
  }

  image->incrementVersion();
}

int CopyRect::lineSize()
{
  return image()->bytesPerPixel() * m_clip.size.w;
}

}} // namespace app::cmd
