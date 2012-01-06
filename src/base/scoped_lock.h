// ASE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_SCOPED_LOCK_H_INCLUDED
#define BASE_SCOPED_LOCK_H_INCLUDED

#include "base/disable_copying.h"

// An object to safely lock and unlock mutexes.
//
// The constructor of ScopedLock locks the mutex, and the destructor
// unlocks the mutex. In this way you can safely use ScopedLock inside
// a try/catch block without worrying about the lock state of the
// mutex if some exception is thrown.
class ScopedLock
{
public:

  ScopedLock(Mutex& mutex) : m_mutex(mutex) {
    m_mutex.lock();
  }

  ~ScopedLock() {
    m_mutex.unlock();
  }

  Mutex& getMutex() const {
    return m_mutex;
  }

private:
  Mutex& m_mutex;

  // Undefined constructors.
  ScopedLock();
  DISABLE_COPYING(ScopedLock);

};

#endif
