// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/chrono.h"

#ifdef _WIN32
  #include "base/chrono_win32.h"
#else
  #include "base/chrono_unix.h"
#endif

namespace base {

  Chrono::Chrono() : m_impl(new ChronoImpl) {
  }

  Chrono::~Chrono() {
    delete m_impl;
  }

  void Chrono::reset() {
    m_impl->reset();
  }

  double Chrono::elapsed() const {
    return m_impl->elapsed();
  }

} // namespace base
