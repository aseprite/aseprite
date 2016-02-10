// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_GL_CONTEXT_EGL_INCLUDED
#define SHE_GL_CONTEXT_EGL_INCLUDED
#pragma once

#include "she/gl/gl_context.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace she {

class GLContextEGL : public GLContext {
public:
  GLContextEGL(void* nativeDisplay)
    : m_nativeDisplay(nativeDisplay)
    , m_context(EGL_NO_CONTEXT)
    , m_display(EGL_NO_DISPLAY)
    , m_surface(EGL_NO_SURFACE)
  {
  }

  ~GLContextEGL() {
    destroyGLContext();
  }

  bool createGLContext() override {
    m_display = getD3DEGLDisplay((HDC)GetDC((HWND)m_nativeDisplay));
    if (m_display == EGL_NO_DISPLAY) {
      LOG("Cannot create EGL display");
      return false;
    }

    EGLint majorVersion;
    EGLint minorVersion;
    if (!eglInitialize(m_display, &majorVersion, &minorVersion))
      return false;

    static const EGLint configAttribs[] = {
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_RED_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_BLUE_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_NONE
    };
    EGLConfig surfaceConfig;
    EGLint numConfigs;
    if (!eglChooseConfig(m_display, configAttribs, &surfaceConfig, 1, &numConfigs))
      return false;

    static const EGLint surfaceAttribs[] = {
      EGL_WIDTH, 1,
      EGL_HEIGHT, 1,
      EGL_NONE
    };
    m_surface = eglCreateWindowSurface(
      m_display,
      surfaceConfig,
      (EGLNativeWindowType)m_nativeDisplay,
      surfaceAttribs);
    if (m_surface == EGL_NO_SURFACE)
      return false;

    static const EGLint contextAttribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
    m_context = eglCreateContext(m_display, surfaceConfig,
                                 EGL_NO_CONTEXT,
                                 contextAttribs);
    if (m_context == EGL_NO_CONTEXT)
      return false;

    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context))
      return false;

    eglGetConfigAttrib(m_display, surfaceConfig, EGL_STENCIL_SIZE, &m_stencilBits);
    eglGetConfigAttrib(m_display, surfaceConfig, EGL_SAMPLES, &m_sampleCount);

    return true;
  }

  void destroyGLContext() override {
    if (m_display != EGL_NO_DISPLAY) {
      eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

      if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(m_display, m_surface);
        m_surface = EGL_NO_SURFACE;
      }

      if (m_context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_display, m_context);
        m_context = EGL_NO_CONTEXT;
      }

      eglTerminate(m_display);
      m_display = EGL_NO_DISPLAY;
    }
  }

  int getStencilBits() override {
    return m_stencilBits;
  }

  int getSampleCount() override {
    return m_sampleCount;
  }

  void swapBuffers() override {
    eglSwapBuffers(m_display, m_surface);
  }

private:
  static void* getD3DEGLDisplay(void* nativeDisplay) {
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT;
    eglGetPlatformDisplayEXT =
      (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
    if (!eglGetPlatformDisplayEXT)
      return eglGetDisplay(static_cast<EGLNativeDisplayType>(nativeDisplay));

    EGLint attribs[3][3] = {
      {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_NONE
      },
      {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
        EGL_NONE
      },
      {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE,
        EGL_NONE
      }
    };

    EGLDisplay display = EGL_NO_DISPLAY;
    for (int i=0; i<3 && display == EGL_NO_DISPLAY; ++i) {
      display = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                         nativeDisplay, attribs[i]);
    }
    return display;
  }

  void* m_nativeDisplay;
  void* m_context;
  void* m_display;
  void* m_surface;
  EGLint m_stencilBits;
  EGLint m_sampleCount;
};

} // namespace she

#endif
