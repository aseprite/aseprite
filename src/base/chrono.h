// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_CHRONO_H_INCLUDED
#define BASE_CHRONO_H_INCLUDED

namespace base {

  class Chrono {
  public:
    Chrono();
    ~Chrono();
    void reset();
    double elapsed() const;

  private:
    class ChronoImpl;
    ChronoImpl* m_impl;
  };

} // namespace base

#endif // BASE_CHRONO_H_INCLUDED
