// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_GL_CONTEXT_INCLUDED
#define SHE_SKIA_GL_CONTEXT_INCLUDED
#pragma once

#ifndef SK_SUPPORT_GPU
  #error Skia was compiled without GPU support
#endif

#include "gl/GrGLInterface.h"

#include <stdexcept>

namespace she {

template<typename Base>
class GLContextSkia : public Base {
public:
  GLContextSkia(typename Base::NativeHandle nativeHandle) : Base(nativeHandle) {
    Base::createGLContext();
    if (!createSkiaInterfaces()) {
      Base::destroyGLContext();
      throw std::runtime_error("Cannot create OpenGL context");
    }
  }

  ~GLContextSkia() {
    m_gl.reset(nullptr);
  }

  const GrGLInterface* gl() const {
    return m_gl.get();
  }

private:
  bool createSkiaInterfaces() {
    SkAutoTUnref<const GrGLInterface> gl(GrGLCreateNativeInterface());
    if (!gl || !gl->validate())
      return false;

    m_gl.reset(gl.detach());
    return true;
  }

  SkAutoTUnref<const GrGLInterface> m_gl;
};

} // namespace she

#endif
