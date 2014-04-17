// Aseprite Base Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SCOPED_LOCK_H_INCLUDED
#define BASE_SCOPED_LOCK_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

namespace base {

  class scoped_unlock {
  public:
    scoped_unlock(mutex& m) : m_mutex(m) {
    }

    ~scoped_unlock() {
      m_mutex.unlock();
    }

    mutex& get_mutex() const {
      return m_mutex;
    }

  private:
    mutex& m_mutex;

    // Undefined constructors.
    scoped_unlock();
    DISABLE_COPYING(scoped_unlock);
  };

  // An object to safely lock and unlock mutexes.
  //
  // The constructor of scoped_lock locks the mutex, and the destructor
  // unlocks the mutex. In this way you can safely use scoped_lock inside
  // a try/catch block without worrying about the lock state of the
  // mutex if some exception is thrown.
  class scoped_lock : public scoped_unlock {
  public:
    scoped_lock(mutex& m) : scoped_unlock(m) {
      get_mutex().lock();
    }

  private:
    // Undefined constructors.
    scoped_lock();
    DISABLE_COPYING(scoped_lock);
  };

} // namespace base

#endif
