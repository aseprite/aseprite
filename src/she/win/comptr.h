// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_COMPTR_H_INCLUDED
#define SHE_WIN_COMPTR_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

namespace she {

  template<class T>
  class ComPtr {
  public:
    ComPtr() : m_ptr(nullptr) { }
    ~ComPtr() {
      if (m_ptr) {
        m_ptr->Release();       // Call IUnknown::Release() automatically
#ifdef _DEBUG
        m_ptr = nullptr;
#endif
      }
    }
    T** operator&() { return &m_ptr; }
    T* operator->() { return m_ptr; }
  private:
    T* m_ptr;

    DISABLE_COPYING(ComPtr);
  };

} // namespace she

#endif
