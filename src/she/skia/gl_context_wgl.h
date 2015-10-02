// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <windows.h>

#include "GL/gl.h"
#include "gl/SkGLContext.h"

namespace she {

class GLContextWGL : public SkGLContext {
public:
  GLContextWGL(HWND hwnd, GrGLStandard forcedGpuAPI)
    : m_hwnd(hwnd)
    , m_glrc(nullptr) {
    HDC hdc = GetDC(m_hwnd);

    PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,                                // version number
      PFD_DRAW_TO_WINDOW |              // support window
      PFD_SUPPORT_OPENGL,               // support OpenGL
      PFD_TYPE_RGBA,                    // RGBA type
      24,                               // 24-bit color depth
      0, 0, 0, 0, 0, 0,                 // color bits ignored
      8,                                // 8-bit alpha buffer
      0,                                // shift bit ignored
      0,                                // no accumulation buffer
      0, 0, 0, 0,                       // accum bits ignored
      0,                                // no z-buffer
      0,                                // no stencil buffer
      0,                                // no auxiliary buffer
      PFD_MAIN_PLANE,                   // main layer
      0,                                // reserved
      0, 0, 0                           // layer masks ignored
    };
    int pixelFormat = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pixelFormat, &pfd);

    m_glrc = wglCreateContext(hdc);
    if (!m_glrc) {
      ReleaseDC(m_hwnd, hdc);
      return;
    }

    wglMakeCurrent(hdc, m_glrc);

    const GrGLInterface* gl = GrGLCreateNativeInterface();
    init(gl);
    if (!gl) {
      ReleaseDC(m_hwnd, hdc);
      destroy();
      return;
    }

    if (!gl->validate()) {
      ReleaseDC(m_hwnd, hdc);
      destroy();
      return;
    }

    ReleaseDC(m_hwnd, hdc);
  }

  ~GLContextWGL() {
    destroy();
  }

  void onPlatformMakeCurrent() const override {
    HDC hdc = GetDC(m_hwnd);
    wglMakeCurrent(hdc, m_glrc);
    ReleaseDC(m_hwnd, hdc);
  }

  void onPlatformSwapBuffers() const override {
    HDC hdc = GetDC(m_hwnd);
    SwapBuffers(hdc);
    ReleaseDC(m_hwnd, hdc);
  }

  GrGLFuncPtr onPlatformGetProcAddress(const char* name) const override {
    return reinterpret_cast<GrGLFuncPtr>(wglGetProcAddress(name));
  }

  int getStencilBits() {
    HDC hdc = GetDC(m_hwnd);
    int pixelFormat = GetPixelFormat(hdc);
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(hdc, pixelFormat, sizeof(pfd), &pfd);
    ReleaseDC(m_hwnd, hdc);
    return pfd.cStencilBits;
  }

  int getSampleCount() {
    return 0;                   // TODO
  }

private:
  void destroy() {
    teardown();

    if (m_glrc) {
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(m_glrc);
      m_glrc = nullptr;
    }
  }

  HWND m_hwnd;
  HGLRC m_glrc;
};

} // namespace she
