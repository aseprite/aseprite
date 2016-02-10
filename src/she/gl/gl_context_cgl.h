// SHE library
// Copyright (C) 2015, 2016  David Capello
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
  GLContextCGL()
    : m_glctx(nullptr)
    , m_stencilBits(0)
    , m_sampleCount(0) {
  }

  ~GLContextCGL() {
    destroyGLContext();
  }

  bool createGLContext() override {
    CGLPixelFormatAttribute attributes[] = {
      kCGLPFAOpenGLProfile,
      (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
      kCGLPFAAccelerated,
      kCGLPFADoubleBuffer,
      (CGLPixelFormatAttribute)0
    };
    CGLPixelFormatObj format;
    GLint npix;
    CGLChoosePixelFormat(attributes, &format, &npix);
    if (!format)
      return false;

    CGLDescribePixelFormat(format, 0, kCGLPFASamples, &m_sampleCount);
    CGLDescribePixelFormat(format, 0, kCGLPFAStencilSize, &m_stencilBits);

    CGLCreateContext(format, nullptr, &m_glctx);
    CGLReleasePixelFormat(format);
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
    return m_stencilBits;
  }

  int getSampleCount() override {
    return m_sampleCount;
  }

  CGLContextObj cglContext() {
    return m_glctx;
  }

private:
  CGLContextObj m_glctx;
  int m_stencilBits;
  int m_sampleCount;
};

} // namespace she

#endif
