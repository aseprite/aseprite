// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_WINDOW_WIN_INCLUDED
#define SHE_SKIA_SKIA_WINDOW_WIN_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "she/skia/skia_surface.h"
#include "she/win/window.h"

#if SK_SUPPORT_GPU
  #include "gl/SkGLContext.h"
#endif

namespace she {

class EventQueue;
class SkiaDisplay;

class SkiaWindow : public Window<SkiaWindow> {
public:
  enum class Backend { NONE, GL, ANGLE };

  SkiaWindow(EventQueue* queue, SkiaDisplay* display);
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
#endif // SK_SUPPORT_GPU

  EventQueue* m_queue;
  SkiaDisplay* m_display;
  Backend m_backend;
#if SK_SUPPORT_GPU
  SkAutoTUnref<SkGLContext> m_glCtx;
  SkAutoTUnref<const GrGLInterface> m_grInterface;
  SkAutoTUnref<GrContext> m_grCtx;
  SkAutoTUnref<GrRenderTarget> m_grRenderTarget;
  SkAutoTDelete<SkSurface> m_skSurface;
  int m_sampleCount;
  int m_stencilBits;
#endif // SK_SUPPORT_GPU

  DISABLE_COPYING(SkiaWindow);
};

} // namespace she

#endif
