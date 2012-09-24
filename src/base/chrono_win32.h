// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class base::Chrono::ChronoImpl {
public:
  ChronoImpl() {
    QueryPerformanceFrequency(&m_freq);
    reset();
  }

  void reset() {
    QueryPerformanceCounter(&m_point);
  }

  double elapsed() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return static_cast<double>(now.QuadPart - m_point.QuadPart)
         / static_cast<double>(m_freq.QuadPart);
  }

private:
  LARGE_INTEGER m_point;
  LARGE_INTEGER m_freq;
};
