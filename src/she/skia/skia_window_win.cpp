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

#if SK_SUPPORT_GPU

  #include "GrContext.h"
  #include "she/skia/gl_context_wgl.h"

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

      // Flush GL context
      m_grInterface->fFunctions.fFlush();

      // We don't use double-buffer
      //ASSERT(m_glCtx);
      //if (m_glCtx)
      //  m_glCtx->swapBuffers();
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
    GLContextWGL* wglCtx = new GLContextWGL(handle(), kGLES_GrGLStandard);
    m_stencilBits = wglCtx->getStencilBits();
    m_sampleCount = wglCtx->getSampleCount();

    m_glCtx.reset(wglCtx);
    ASSERT(m_glCtx->isValid());
    if (!m_glCtx->isValid()) {
      detachGL();
      return false;
    }

    m_grInterface.reset(SkRef(m_glCtx->gl()));
    m_grInterface.reset(GrGLInterfaceRemoveNVPR(m_grInterface));
    m_grCtx.reset(GrContext::Create(kOpenGL_GrBackend, (GrBackendContext)m_grInterface.get()));
  }

  if (m_glCtx) {
    m_glCtx->makeCurrent();
    return true;
  }
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
  m_grCtx.reset(nullptr);
  m_grInterface.reset(nullptr);
  m_glCtx.reset(nullptr);
}

void SkiaWindow::createRenderTarget(const gfx::Size& size)
{
  GrBackendRenderTargetDesc desc;
  desc.fWidth = size.w / m_display->scale();
  desc.fHeight = size.h / m_display->scale();
  desc.fConfig = kSkia8888_GrPixelConfig;
  desc.fOrigin = kBottomLeft_GrSurfaceOrigin;
  desc.fSampleCnt = m_sampleCount;
  desc.fStencilBits = m_stencilBits;
  GrGLint buffer;
  m_grInterface->fFunctions.fGetIntegerv(0x8CA6, &buffer); // GL_FRAMEBUFFER_BINDING = 0x8CA6
  desc.fRenderTargetHandle = buffer;

  m_grRenderTarget.reset(m_grCtx->textureProvider()->wrapBackendRenderTarget(desc));

  m_skSurface.reset(SkSurface::NewRenderTargetDirect(m_grRenderTarget));
  m_display->setSkiaSurface(new SkiaSurface(m_skSurface));
}

#endif // SK_SUPPORT_GPU

void SkiaWindow::resizeImpl(const gfx::Size& size)
{
#if SK_SUPPORT_GPU
#if SK_ANGLE
  if (attachANGLE()) {
    m_backend = Backend::ANGLE;
  }
  else
#endif // SK_ANGLE
  if (attachGL()) {
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
