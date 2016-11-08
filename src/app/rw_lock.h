// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RW_LOCK_H_INCLUDED
#define APP_RW_LOCK_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/mutex.h"

namespace app {

  // A readers-writer lock implementation
  class RWLock {
  public:
    enum LockType {
      ReadLock,
      WriteLock
    };

    RWLock();
    ~RWLock();

    // Locks the object to read or write on it, returning true if the
    // object can be accessed in the desired mode.
    bool lock(LockType lockType, int timeout);

    // If you've locked the object to read, using this method you can
    // raise your access level to write it.
    bool upgradeToWrite(int timeout);

    // If we've locked the object to write, using this method we can
    // lower our access to read-only.
    void downgradeToRead();

    // Unlocks a previously successfully lock() operation.
    void unlock();

  private:
    // Mutex to modify the 'locked' flag.
    base::mutex m_mutex;

    // True if some thread is writing the object.
    bool m_write_lock;

    // Greater than zero when one or more threads are reading the object.
    int m_read_locks;

    DISABLE_COPYING(RWLock);
  };

} // namespace app

#endif
