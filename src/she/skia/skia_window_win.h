// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_WINDOW_WIN_INCLUDED
#define SHE_SKIA_SKIA_WINDOW_WIN_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/unique_ptr.h"
#include "she/skia/skia_surface.h"
#include "she/win/window.h"

#if SK_SUPPORT_GPU
  #include "she/gl/gl_context_wgl.h"
  #include "she/skia/gl_context_skia.h"
#endif

namespace she {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public WinWindow<SkiaWindow> {
public:
  enum class Backend { NONE, GL, ANGLE };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display,
             int width, int height, int scale);
  ~SkiaWindow();

  void queueEventImpl(Event& ev);
  void paintImpl(HDC hdc);
  void resizeImpl(const gfx::Size& size);

private:
  void paintHDC(HDC dc);

#if SK_SUPPORT_GPU
  bool attachGL();
#if SK_ANGLE
  bool attachANGLE();
#endif // SK_ANGLE
  void detachGL();
  void createRenderTarget(const gfx::Size& size);
  void setSurface(SkSurface* surface);
#endif // SK_SUPPORT_GPU

  EventQueue* m_queue;
  SkiaDisplay* m_display;
  Backend m_backend;
#if SK_SUPPORT_GPU
  base::UniquePtr<GLContextSkia<GLContextWGL> > m_glCtx;
  SkAutoTUnref<GrContext> m_grCtx;
  SkAutoTUnref<GrRenderTarget> m_grRenderTarget;
  SkAutoTDelete<SkSurface> m_skSurfaceDirect;
  SkSurface* m_skSurface;
  int m_sampleCount;
  int m_stencilBits;
  gfx::Size m_lastSize;
#endif // SK_SUPPORT_GPU

  DISABLE_COPYING(SkiaWindow);
};

} // namespace she

#endif
