// SHE library
// Copyright (C) 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_GL_CONTEXT_CGL_INCLUDED
#define SHE_GL_CONTEXT_CGL_INCLUDED
#pragma once

#include "she/gl/gl_context.h"

#include <OpenGL/OpenGL.h>
#include <dlfcn.h>

namespace she {

class GLContextCGL : public GLContext {
public:
  typedef void* NativeHandle;

  GLContextCGL(void*)
    : m_glctx(nullptr) {
  }

  ~GLContextCGL() {
    destroyGLContext();
  }

  bool createGLContext() override {
    CGLPixelFormatAttribute attributes[] = {
#if MAC_OS_X_VERSION_10_7
      kCGLPFAOpenGLProfile,
      (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
#endif
      kCGLPFADoubleBuffer,
      (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatObj pixFormat;
    GLint npix;
    CGLChoosePixelFormat(attributes, &pixFormat, &npix);
    if (!pixFormat)
      return false;

    CGLCreateContext(pixFormat, nullptr, &m_glctx);
    CGLReleasePixelFormat(pixFormat);
    if (!m_glctx)
      return false;

    CGLSetCurrentContext(m_glctx);
    return true;
  }

  void destroyGLContext() override {
    if (m_glctx) {
      CGLReleaseContext(m_glctx);
      m_glctx = nullptr;
    }
  }

  int getStencilBits() override {
    return 0;
  }

  int getSampleCount() override {
    return 0;
  }

private:
  CGLContextObj m_glctx;
};

} // namespace she

#endif
