// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_GL_CONTEXT_WGL_INCLUDED
#define SHE_GL_CONTEXT_WGL_INCLUDED
#pragma once

#include "she/gl/gl_context.h"

#include <windows.h>

namespace she {

class GLContextWGL : public GLContext {
public:
  typedef HWND NativeHandle;

  GLContextWGL(HWND hwnd)
    : m_hwnd(hwnd)
    , m_glrc(nullptr) {
  }

  ~GLContextWGL() {
    destroyGLContext();
  }

  bool createGLContext() override {
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
      return false;
    }

    wglMakeCurrent(hdc, m_glrc);
    ReleaseDC(m_hwnd, hdc);
    return true;
  }

  void destroyGLContext() override {
    if (m_glrc) {
      wglMakeCurrent(nullptr, nullptr);
      wglDeleteContext(m_glrc);
      m_glrc = nullptr;
    }
  }

  int getStencilBits() override {
    HDC hdc = GetDC(m_hwnd);
    int pixelFormat = GetPixelFormat(hdc);
    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(hdc, pixelFormat, sizeof(pfd), &pfd);
    ReleaseDC(m_hwnd, hdc);
    return pfd.cStencilBits;
  }

  int getSampleCount() override {
    return 0;
  }

private:
  HWND m_hwnd;
  HGLRC m_glrc;
};

} // namespace she

#endif
