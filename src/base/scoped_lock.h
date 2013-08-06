// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_SCOPED_LOCK_H_INCLUDED
#define BASE_SCOPED_LOCK_H_INCLUDED

#include "base/disable_copying.h"

namespace base {

  // An object to safely lock and unlock mutexes.
  //
  // The constructor of scoped_lock locks the mutex, and the destructor
  // unlocks the mutex. In this way you can safely use scoped_lock inside
  // a try/catch block without worrying about the lock state of the
  // mutex if some exception is thrown.
  class scoped_lock {
  public:

    scoped_lock(mutex& m) : m_mutex(m) {
      m_mutex.lock();
    }

    ~scoped_lock() {
      m_mutex.unlock();
    }

    mutex& get_mutex() const {
      return m_mutex;
    }

  private:
    mutex& m_mutex;

    // Undefined constructors.
    scoped_lock();
    DISABLE_COPYING(scoped_lock);

  };

} // namespace base

#endif
