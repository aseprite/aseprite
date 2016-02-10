// SHE library
// Copyright (C) 2015, 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_GL_CONTEXT_INCLUDED
#define SHE_GL_CONTEXT_INCLUDED
#pragma once

namespace she {

class GLContext {
public:
  virtual ~GLContext() { }
  virtual bool createGLContext() = 0;
  virtual void destroyGLContext() = 0;
  virtual int getStencilBits() = 0;
  virtual int getSampleCount() = 0;
  virtual void swapBuffers() { }
};

} // namespace she

#endif
