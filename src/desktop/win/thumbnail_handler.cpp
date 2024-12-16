// Desktop Integration
// Copyright (C) 2021-2022  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/base.h"
#include "dio/decode_delegate.h"
#include "dio/decode_file.h"
#include "dio/file_interface.h"
#include "doc/color.h"
#include "doc/document.h"
#include "doc/image_ref.h"
#include "doc/image_traits.h"
#include "doc/pixel_format.h"
#include "doc/sprite.h"
#include "render/render.h"

#include "desktop/win/thumbnail_handler.h"

#include <algorithm>
#include <cassert>
#include <new>

#include <objbase.h>
#include <shlwapi.h>

namespace desktop {

namespace {

class DecodeDelegate : public dio::DecodeDelegate {
public:
  DecodeDelegate() : m_sprite(nullptr) {}
  ~DecodeDelegate() { delete m_sprite; }

  bool decodeOneFrame() override { return true; }
  void onSprite(doc::Sprite* sprite) override { m_sprite = sprite; }

  doc::Sprite* sprite() { return m_sprite; }

private:
  doc::Sprite* m_sprite;
};

class StreamAdaptor : public dio::FileInterface {
public:
  StreamAdaptor(IStream* stream) : m_stream(stream), m_ok(m_stream != nullptr) {}

  bool ok() const { return m_ok; }

  size_t tell()
  {
    LARGE_INTEGER delta;
    delta.QuadPart = 0;

    ULARGE_INTEGER newPos;
    HRESULT hr = m_stream->Seek(delta, STREAM_SEEK_CUR, &newPos);
    if (FAILED(hr)) {
      m_ok = false;
      return 0;
    }
    return newPos.QuadPart;
  }

  void seek(size_t absPos)
  {
    LARGE_INTEGER pos;
    pos.QuadPart = absPos;

    ULARGE_INTEGER newPos;
    HRESULT hr = m_stream->Seek(pos, STREAM_SEEK_SET, &newPos);
    if (FAILED(hr))
      m_ok = false;
  }

  uint8_t read8()
  {
    if (!m_ok)
      return 0;

    unsigned char byte = 0;
    ULONG count;
    HRESULT hr = m_stream->Read((void*)&byte, 1, &count);
    if (FAILED(hr) || count != 1) {
      m_ok = false;
      return 0;
    }
    return byte;
  }

  size_t readBytes(uint8_t* buf, size_t n)
  {
    if (!m_ok)
      return 0;

    ULONG count;
    HRESULT hr = m_stream->Read((void*)buf, (ULONG)n, &count);
    if (FAILED(hr) || count != n)
      m_ok = false;
    return count;
  }

  void write8(uint8_t value)
  {
    // Do nothing, we don't write in the file
  }

  IStream* m_stream;
  bool m_ok;
};

} // anonymous namespace

// static
HRESULT ThumbnailHandler::CreateInstance(REFIID riid, void** ppv)
{
  *ppv = nullptr;

  ThumbnailHandler* obj = new (std::nothrow) ThumbnailHandler;
  if (!obj)
    return E_OUTOFMEMORY;

  HRESULT hr = obj->QueryInterface(riid, ppv);
  obj->Release();
  return hr;
}

ThumbnailHandler::ThumbnailHandler() : m_ref(1)
{
}

ThumbnailHandler::~ThumbnailHandler()
{
}

// IUnknown
HRESULT ThumbnailHandler::QueryInterface(REFIID riid, void** ppv)
{
  *ppv = nullptr;
  static const QITAB qit[] = {
    QITABENT(ThumbnailHandler, IInitializeWithStream),
    QITABENT(ThumbnailHandler, IThumbnailProvider),
    { 0 },
  };
  return QISearch(this, qit, riid, ppv);
}

ULONG ThumbnailHandler::AddRef()
{
  return InterlockedIncrement(&m_ref);
}

ULONG ThumbnailHandler::Release()
{
  ULONG ref = InterlockedDecrement(&m_ref);
  if (!ref)
    delete this;
  return ref;
}

// IInitializeWithStream
HRESULT ThumbnailHandler::Initialize(IStream* pStream, DWORD grfMode)
{
  if (!pStream)
    return E_INVALIDARG;

  m_stream.reset();
  return pStream->QueryInterface(IID_IStream, (void**)&m_stream);
}

// IThumbnailProvider
HRESULT ThumbnailHandler::GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha)
{
  if (cx < 1 || !phbmp || !pdwAlpha)
    return E_INVALIDARG;

  if (!m_stream.get())
    return E_FAIL;

  doc::ImageRef image;
  int w, h;

  try {
    DecodeDelegate delegate;
    StreamAdaptor adaptor(m_stream.get());
    if (!dio::decode_file(&delegate, &adaptor))
      return E_FAIL;

    const doc::Sprite* spr = delegate.sprite();
    w = spr->width();
    h = spr->height();
    int wh = std::max<int>(w, h);

    image.reset(doc::Image::create(doc::IMAGE_RGB, cx * w / wh, cx * h / wh));
    image->clear(0);

#undef TRANSPARENT // Windows defines TRANSPARENT macro
    render::Render render;
    render.setBgOptions(render::BgOptions::MakeTransparent());
    render.setProjection(render::Projection(doc::PixelRatio(1, 1), render::Zoom(cx, wh)));
    render.renderSprite(image.get(),
                        spr,
                        0,
                        gfx::ClipF(0, 0, 0, 0, image->width(), image->height()));

    w = image->width();
    h = image->height();
  }
  catch (const std::exception&) {
    // TODO convert exception into a HRESULT
    return E_FAIL;
  }

  BITMAPINFO bi;
  ZeroMemory(&bi, sizeof(bi));
  bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = -h;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;

  unsigned char* data = nullptr;
  *phbmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, (void**)&data, nullptr, 0);
  if (!*phbmp)
    return E_FAIL;

  for (int y = 0; y < h; ++y) {
    doc::RgbTraits::address_t row = (doc::RgbTraits::address_t)image->getPixelAddress(0, y);
    for (int x = 0; x < w; ++x, ++row) {
      doc::color_t c = *row;
      *(data++) = doc::rgba_getb(c);
      *(data++) = doc::rgba_getg(c);
      *(data++) = doc::rgba_getr(c);
      *(data++) = doc::rgba_geta(c);
    }
  }

  *pdwAlpha = WTSAT_ARGB;
  return S_OK;
}

} // namespace desktop
