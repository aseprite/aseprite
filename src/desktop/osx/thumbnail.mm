// Desktop Integration
// Copyright (c) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "thumbnail.h"

#include "dio/decode_delegate.h"
#include "dio/decode_file.h"
#include "dio/file_interface.h"
#include "doc/sprite.h"
#include "render/render.h"

#include <Cocoa/Cocoa.h>

#include <algorithm>

namespace desktop {

namespace {

class DecodeDelegate : public dio::DecodeDelegate {
public:
  DecodeDelegate() : m_sprite(nullptr) { }
  ~DecodeDelegate() { delete m_sprite; }

  bool decodeOneFrame() override { return true; }
  void onSprite(doc::Sprite* sprite) override {
    m_sprite = sprite;
  }

  doc::Sprite* sprite() { return m_sprite; }

private:
  doc::Sprite* m_sprite;
};

class StreamAdaptor : public dio::FileInterface {
public:
  StreamAdaptor(NSData* data)
    : m_data(data)
    , m_ok(m_data != nullptr)
    , m_pos(0) {
  }

  bool ok() const {
    return m_ok;
  }

  size_t tell() {
    return m_pos;
  }

  void seek(size_t absPos) {
    m_pos = absPos;
  }

  uint8_t read8() {
    if (!m_ok)
      return 0;

    if (m_pos < m_data.length)
      return ((const uint8_t*)m_data.bytes)[m_pos++];
    else {
      m_ok = false;
      return 0;
    }
  }

  size_t readBytes(uint8_t* buf, size_t n) {
    if (!m_ok)
      return 0;

    if (m_pos < m_data.length) {
      n = std::min(n, m_data.length - m_pos);

      memcpy(buf, ((const uint8_t*)m_data.bytes)+m_pos, n);
      m_pos += n;
      return n;
    }
    else {
      m_ok = false;
      return 0;
    }
  }

  void write8(uint8_t value) {
    // Do nothing, we don't write in the file
  }

  NSData* m_data;
  bool m_ok;
  size_t m_pos;
};

} // anonymous namespace

CGImageRef get_thumbnail(CFURLRef url,
                         CFDictionaryRef options,
                         CGSize maxSize)
{
  auto data = [[NSData alloc] initWithContentsOfURL:(NSURL*)url];
  if (!data)
    return nullptr;

  doc::ImageRef image;
  int w, h;

  try {
    DecodeDelegate delegate;
    StreamAdaptor adaptor(data);
    if (!dio::decode_file(&delegate, &adaptor))
      return nullptr;

    const doc::Sprite* spr = delegate.sprite();
    if (!spr)
      return nullptr;

    w = spr->width();
    h = spr->height();
    int wh = std::max<int>(w, h);
    int cx;
    if (maxSize.width && maxSize.height)
      cx = std::min<int>(maxSize.width, maxSize.height);
    else
      cx = wh;

    image.reset(doc::Image::create(doc::IMAGE_RGB,
                                   cx * w / wh,
                                   cx * h / wh));
    image->clear(0);

    render::Render render;
    render.setBgOptions(render::BgOptions::MakeTransparent());
    render.setProjection(render::Projection(doc::PixelRatio(1, 1),
                                            render::Zoom(cx, wh)));
    render.renderSprite(image.get(), spr, 0,
                        gfx::ClipF(0, 0, 0, 0,
                                   image->width(), image->height()));

    w = image->width();
    h = image->height();
  }
  catch (const std::exception& e) {
    NSLog(@"AsepriteThumbnailer error: %s", e.what());
    return nullptr;
  }

  // TODO Premultiply alpha because CGBitmapContextCreate doesn't
  //      support unpremultiplied alpha (kCGImageAlphaFirst).

  CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
  CGContextRef gc = CGBitmapContextCreate(
    image->getPixelAddress(0, 0),
    w, h, 8, image->getRowStrideSize(), cs,
    kCGImageAlphaPremultipliedLast);
  CGColorSpaceRelease(cs);

  CGImageRef img = CGBitmapContextCreateImage(gc);
  CGContextRelease(gc);
  return img;
}

} // namespace desktop
