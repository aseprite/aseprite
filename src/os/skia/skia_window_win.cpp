// LAF OS Library
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/skia/skia_window_win.h"

#include "base/log.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/skia/skia_display.h"
#include "os/system.h"

#undef max // To avoid include/private/SkPathRef.h(451): error C2589: '(': illegal token on right side of '::'

#if SK_SUPPORT_GPU

  #include "GrBackendSurface.h"
  #include "GrContext.h"
  #include "gl/GrGLDefines.h"
  #include "os/gl/gl_context_wgl.h"
  #if SK_ANGLE
    #include "os/gl/gl_context_egl.h"
    #include "gl/GrGLAssembleInterface.h"
  #endif

#endif

#include <windows.h>
#include "os/win/window_dde.h"

#include <iostream>

namespace os {

SkiaWindow::SkiaWindow(EventQueue* queue, SkiaDisplay* display,
                       int width, int height, int scale)
  : WinWindow(width, height, scale)
  , m_queue(queue)
  , m_display(display)
  , m_backend(Backend::NONE)
#if SK_SUPPORT_GPU
  , m_skSurface(nullptr)
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

void SkiaWindow::onQueueEvent(Event& ev)
{
  ev.setDisplay(m_display);
  m_queue->queueEvent(ev);
}

void SkiaWindow::onPaint(HDC hdc)
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
#if 0                           // TODO
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
        sk_sp<SkImage> image(SkImage::MakeFromTexture(m_grCtx.get(), texDesc));

        SkRect dstRect(SkRect::MakeWH(SkIntToScalar(m_lastSize.w),
                                      SkIntToScalar(m_lastSize.h)));

        SkPaint paint;
        m_skSurfaceDirect->getCanvas()->drawImageRect(
          image, dstRect, &paint,
          SkCanvas::kStrict_SrcRectConstraint);

        m_skSurfaceDirect->getCanvas()->flush();
#endif
      }

      // Flush GL context
      m_glInterfaces->fFunctions.fFlush();
      m_glCtx->swapBuffers();
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

  int ret = StretchDIBits(hdc,
    0, 0, bitmap.width()*scale(), bitmap.height()*scale(),
    0, 0, bitmap.width(), bitmap.height(),
    bitmap.getPixels(),
    &bmi, DIB_RGB_COLORS, SRCCOPY);
  (void)ret;
}

#if SK_SUPPORT_GPU

#if SK_ANGLE

struct ANGLEAssembleContext {
  ANGLEAssembleContext() {
    fEGL = GetModuleHandle(L"libEGL.dll");
    fGL = GetModuleHandle(L"libGLESv2.dll");
  }

  bool isValid() const { return SkToBool(fEGL) && SkToBool(fGL); }

  HMODULE fEGL;
  HMODULE fGL;
};

static GrGLFuncPtr angle_get_gl_proc(void* ctx, const char name[]) {
  const ANGLEAssembleContext& context = *reinterpret_cast<const ANGLEAssembleContext*>(ctx);
  GrGLFuncPtr proc = (GrGLFuncPtr) GetProcAddress(context.fGL, name);
  if (proc) {
    return proc;
  }
  proc = (GrGLFuncPtr) GetProcAddress(context.fEGL, name);
  if (proc) {
    return proc;
  }
  return eglGetProcAddress(name);
}

static const GrGLInterface* get_angle_gl_interface() {
  ANGLEAssembleContext context;
  if (!context.isValid()) {
    return nullptr;
  }
  return GrGLAssembleGLESInterface(&context, angle_get_gl_proc);
}

bool SkiaWindow::attachANGLE()
{
  if (!m_glCtx) {
    try {
      std::unique_ptr<GLContext> ctx(new GLContextEGL(handle()));
      if (!ctx->createGLContext())
        throw std::runtime_error("Cannot create EGL context");

      m_glInterfaces.reset(get_angle_gl_interface());
      if (!m_glInterfaces || !m_glInterfaces->validate())
        throw std::runtime_error("Cannot create EGL interfaces\n");

      m_stencilBits = ctx->getStencilBits();
      m_sampleCount = ctx->getSampleCount();

      m_glCtx.reset(ctx.release());
      m_grCtx.reset(
        GrContext::Create(kOpenGL_GrBackend,
                          (GrBackendContext)m_glInterfaces.get()));

      LOG("OS: Using EGL backend\n");
    }
    catch (const std::exception& ex) {
      LOG(ERROR) << "OS: Error initializing EGL backend: " << ex.what() << "\n";
      detachGL();
    }
  }

  if (m_glCtx)
    return true;
  else
    return false;
}

#endif // SK_ANGLE

bool SkiaWindow::attachGL()
{
  if (!m_glCtx) {
    try {
      std::unique_ptr<GLContext> ctx(new GLContextWGL(handle()));
      if (!ctx->createGLContext())
        throw std::runtime_error("Cannot create WGL context\n");

      m_glInterfaces.reset(GrGLCreateNativeInterface());
      if (!m_glInterfaces || !m_glInterfaces->validate())
        throw std::runtime_error("Cannot create WGL interfaces\n");

      m_stencilBits = ctx->getStencilBits();
      m_sampleCount = ctx->getSampleCount();

      m_glCtx.reset(ctx.release());
      m_grCtx.reset(
        GrContext::Create(kOpenGL_GrBackend,
                          (GrBackendContext)m_glInterfaces.get()));

      LOG("OS: Using WGL backend\n");
    }
    catch (const std::exception& ex) {
      LOG(ERROR) << "OS: Error initializing WGL backend: " << ex.what() << "\n";
      detachGL();
    }
  }

  if (m_glCtx)
    return true;
  else
    return false;
}

void SkiaWindow::detachGL()
{
  if (m_glCtx && m_display)
    m_display->resetSkiaSurface();

  m_skSurface.reset(nullptr);
  m_skSurfaceDirect.reset(nullptr);
  m_grCtx.reset(nullptr);
  m_glCtx.reset(nullptr);
}

void SkiaWindow::createRenderTarget(const gfx::Size& size)
{
  int scale = m_display->scale();
  m_lastSize = size;

  GrGLint buffer;
  m_glInterfaces->fFunctions.fGetIntegerv(GR_GL_FRAMEBUFFER_BINDING, &buffer);
  GrGLFramebufferInfo info;
  info.fFBOID = (GrGLuint) buffer;

  GrBackendRenderTarget
    target(size.w, size.h,
           m_sampleCount,
           m_stencilBits,
           kSkia8888_GrPixelConfig,
           info);

  SkSurfaceProps props(SkSurfaceProps::kLegacyFontHost_InitType);

  m_skSurface.reset(nullptr); // set m_skSurface comparing with the old m_skSurfaceDirect
  m_skSurfaceDirect = SkSurface::MakeFromBackendRenderTarget(
    m_grCtx.get(), target,
    kBottomLeft_GrSurfaceOrigin,
    nullptr, &props);

  if (scale == 1) {
    m_skSurface = m_skSurfaceDirect;
  }
  else {
    m_skSurface =
      SkSurface::MakeRenderTarget(
        m_grCtx.get(),
        SkBudgeted::kYes,
        SkImageInfo::MakeN32Premul(MAX(1, size.w / scale),
                                   MAX(1, size.h / scale)),
        m_sampleCount,
        nullptr);
  }

  if (!m_skSurface)
    throw std::runtime_error("Error creating surface for main display");

  m_display->setSkiaSurface(new SkiaSurface(m_skSurface));
}

#endif // SK_SUPPORT_GPU

void SkiaWindow::onResize(const gfx::Size& size)
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

#if SK_SUPPORT_GPU
  if (m_glCtx)
    createRenderTarget(size);
#endif

  m_display->resize(size);
}

} // namespace os
