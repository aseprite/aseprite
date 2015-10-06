// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_window_win.h"

#include "she/event_queue.h"
#include "she/skia/skia_display.h"
#include "she/system.h"

#if SK_SUPPORT_GPU

  #include "GrContext.h"
  #include "she/gl/gl_context_wgl.h"

#endif

namespace she {

SkiaWindow::SkiaWindow(EventQueue* queue, SkiaDisplay* display)
  : m_queue(queue)
  , m_display(display)
  , m_backend(Backend::NONE)
#if SK_SUPPORT_GPU
  , m_sampleCount(0)
  , m_stencilBits(0)
#endif
{
}

SkiaWindow::~SkiaWindow()
{
  switch (m_backend) {

    case Backend::NONE:
      // Do nothing
      break;

#if SK_SUPPORT_GPU

    case Backend::GL:
    case Backend::ANGLE:
      detachGL();
      break;

#endif // SK_SUPPORT_GPU
  }
}

void SkiaWindow::queueEventImpl(Event& ev)
{
  ev.setDisplay(m_display);
  m_queue->queueEvent(ev);
}

void SkiaWindow::paintImpl(HDC hdc)
{
  switch (m_backend) {

    case Backend::NONE:
      paintHDC(hdc);
      break;

#if SK_SUPPORT_GPU

    case Backend::GL:
    case Backend::ANGLE:
      // Flush operations to the SkCanvas
      {
        SkiaSurface* surface = static_cast<SkiaSurface*>(m_display->getSurface());
        surface->flush();
      }

      // If we are drawing inside an off-screen texture, here we have
      // to blit that texture into the main framebuffer.
      if (m_skSurfaceDirect != m_skSurface) {
        GrBackendObject texID = m_skSurface->getTextureHandle(
          SkSurface::kFlushRead_BackendHandleAccess);

        GrBackendTextureDesc texDesc;
        texDesc.fFlags = kNone_GrBackendTextureFlag;
        texDesc.fOrigin = kBottomLeft_GrSurfaceOrigin;
        texDesc.fWidth = m_lastSize.w / scale();
        texDesc.fHeight = m_lastSize.h / scale();
        texDesc.fConfig = kSkia8888_GrPixelConfig;
        texDesc.fSampleCnt = m_sampleCount;
        texDesc.fTextureHandle = texID;
        SkAutoTUnref<SkImage> image(SkImage::NewFromTexture(m_grCtx, texDesc));

        SkRect dstRect(SkRect::MakeWH(SkIntToScalar(m_lastSize.w),
                                      SkIntToScalar(m_lastSize.h)));

        SkPaint paint;
        m_skSurfaceDirect->getCanvas()->drawImageRect(
          image, dstRect, &paint,
          SkCanvas::kStrict_SrcRectConstraint);

        m_skSurfaceDirect->getCanvas()->flush();
      }

      // Flush GL context
      m_glCtx->gl()->fFunctions.fFlush();
      break;

#endif // SK_SUPPORT_GPU
  }
}

void SkiaWindow::paintHDC(HDC hdc)
{
  SkiaSurface* surface = static_cast<SkiaSurface*>(m_display->getSurface());
  const SkBitmap& bitmap = surface->bitmap();

  BITMAPINFO bmi;
  memset(&bmi, 0, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = bitmap.width();
  bmi.bmiHeader.biHeight = -bitmap.height();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;

  ASSERT(bitmap.width() * bitmap.bytesPerPixel() == bitmap.rowBytes());
  bitmap.lockPixels();

  int ret = StretchDIBits(hdc,
    0, 0, bitmap.width()*scale(), bitmap.height()*scale(),
    0, 0, bitmap.width(), bitmap.height(),
    bitmap.getPixels(),
    &bmi, DIB_RGB_COLORS, SRCCOPY);
  (void)ret;

  bitmap.unlockPixels();
}

#if SK_SUPPORT_GPU

bool SkiaWindow::attachGL()
{
  if (!m_glCtx) {
    try {
      auto wglCtx = new GLContextSkia<GLContextWGL>(handle());
      m_stencilBits = wglCtx->getStencilBits();
      m_sampleCount = wglCtx->getSampleCount();

      m_glCtx.reset(wglCtx);
      m_grCtx.reset(GrContext::Create(kOpenGL_GrBackend,
                                      (GrBackendContext)m_glCtx->gl()));
    }
    catch (const std::exception& ex) {
      LOG("Cannot create GL context: %s\n", ex.what());
      detachGL();
    }
  }

  if (m_glCtx)
    return true;
  else
    return false;
}

#if SK_ANGLE

bool SkiaWindow::attachANGLE()
{
  return false;                 // TODO
}

#endif // SK_ANGLE

void SkiaWindow::detachGL()
{
  m_skSurfaceDirect.reset(nullptr);
  m_skSurface.reset(nullptr);
  m_grRenderTarget.reset(nullptr);
  m_grCtx.reset(nullptr);
  m_glCtx.reset(nullptr);
}

void SkiaWindow::createRenderTarget(const gfx::Size& size)
{
  int scale = m_display->scale();
  m_lastSize = size;

  GrBackendRenderTargetDesc desc;
  desc.fWidth = size.w;
  desc.fHeight = size.h;
  desc.fConfig = kSkia8888_GrPixelConfig;
  desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
  desc.fSampleCnt = m_sampleCount;
  desc.fStencilBits = m_stencilBits;
  desc.fRenderTargetHandle = 0; // direct frame buffer
  m_grRenderTarget.reset(m_grCtx->textureProvider()->wrapBackendRenderTarget(desc));
  m_skSurfaceDirect.reset(
    SkSurface::NewRenderTargetDirect(m_grRenderTarget));

  if (scale == 1) {
    m_skSurface.reset(m_skSurfaceDirect);
  }
  else {
    m_skSurface.reset(
      SkSurface::NewRenderTarget(
        m_grCtx,
        SkSurface::kYes_Budgeted,
        SkImageInfo::MakeN32Premul(MAX(1, size.w / scale),
                                   MAX(1, size.h / scale)),
        m_sampleCount));
  }

  if (!m_skSurface)
    throw std::runtime_error("Error creating OpenGL surface for main display");

  m_display->setSkiaSurface(new SkiaSurface(m_skSurface));
}

#endif // SK_SUPPORT_GPU

void SkiaWindow::resizeImpl(const gfx::Size& size)
{
  bool gpu = instance()->gpuAcceleration();
  (void)gpu;

#if SK_SUPPORT_GPU
#if SK_ANGLE
  if (gpu && attachANGLE()) {
    m_backend = Backend::ANGLE;
  }
  else
#endif // SK_ANGLE
  if (gpu && attachGL()) {
    m_backend = Backend::GL;
  }
  else
#endif // SK_SUPPORT_GPU
  {
#if SK_SUPPORT_GPU
    detachGL();
#endif
    m_backend = Backend::NONE;
  }

  if (m_glCtx)
    createRenderTarget(size);

  m_display->resize(size);
}

} // namespace she
