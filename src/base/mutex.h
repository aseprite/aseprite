// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_MUTEX_H_INCLUDED
#define BASE_MUTEX_H_INCLUDED

#include "base/disable_copying.h"

namespace base {

  class mutex {
  public:
    mutex();
    ~mutex();

    void lock();
    bool try_lock();
    void unlock();

  private:
    class mutex_impl;
    mutex_impl* m_impl;

    DISABLE_COPYING(mutex);
  };

} // namespace base

#endif
