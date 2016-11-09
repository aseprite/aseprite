// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// Uncomment this line in case that you want TRACE() lock/unlock
// operations.
//#define DEBUG_OBJECT_LOCKS

#include "app/document.h"

#include "base/scoped_lock.h"
#include "base/thread.h"

namespace app {

using namespace base;

RWLock::RWLock()
  : m_write_lock(false)
  , m_read_locks(0)
  , m_weak_lock(nullptr)
{
}

RWLock::~RWLock()
{
  ASSERT(!m_write_lock);
  ASSERT(m_read_locks == 0);
  ASSERT(m_weak_lock == nullptr);
}

bool RWLock::lock(LockType lockType, int timeout)
{
  while (timeout >= 0) {
    {
      scoped_lock lock(m_mutex);

      // Check that there is no weak lock
      if (m_weak_lock) {
        if (*m_weak_lock == WeakLocked)
          *m_weak_lock = WeakUnlocking;

        // Wait some time
        if (*m_weak_lock == WeakUnlocking)
          goto go_wait;

        ASSERT(*m_weak_lock == WeakUnlocked);
      }

      switch (lockType) {

        case ReadLock:
          // If no body is writting the object...
          if (!m_write_lock) {
            // We can read it
            ++m_read_locks;
            return true;
          }
          break;

        case WriteLock:
          // If no body is reading and writting...
          if (m_read_locks == 0 && !m_write_lock) {
            // We can start writting the object...
            m_write_lock = true;

#ifdef DEBUG_OBJECT_LOCKS
            TRACE("LCK: lock: Locked <%p> to write\n", this);
#endif
            return true;
          }
          break;

      }

    go_wait:;
    }

    if (timeout > 0) {
      int delay = MIN(100, timeout);
      timeout -= delay;

#ifdef DEBUG_OBJECT_LOCKS
      TRACE("LCK: lock: wait 100 msecs for <%p>\n", this);
#endif

      base::this_thread::sleep_for(double(delay) / 1000.0);
    }
    else
      break;
  }

#ifdef DEBUG_OBJECT_LOCKS
  TRACE("LCK: lock: Cannot lock <%p> to %s (has %d read locks and %d write locks)\n",
    this, (lockType == ReadLock ? "read": "write"), m_read_locks, m_write_lock);
#endif

  return false;
}

void RWLock::downgradeToRead()
{
  scoped_lock lock(m_mutex);

  ASSERT(m_read_locks == 0);
  ASSERT(m_write_lock);

  m_write_lock = false;
  m_read_locks = 1;
}

void RWLock::unlock()
{
  scoped_lock lock(m_mutex);

  if (m_write_lock) {
    m_write_lock = false;
  }
  else if (m_read_locks > 0) {
    --m_read_locks;
  }
  else {
    ASSERT(false);
  }
}

bool RWLock::weakLock(WeakLock* weak_lock_flag)
{
  scoped_lock lock(m_mutex);

  if (m_weak_lock ||
      m_write_lock ||
      m_read_locks > 0)
    return false;

  m_weak_lock = weak_lock_flag;
  *m_weak_lock = WeakLocked;
  return true;
}

void RWLock::weakUnlock()
{
  ASSERT(m_weak_lock);
  ASSERT(*m_weak_lock != WeakLock::WeakUnlocked);
  ASSERT(!m_write_lock);
  ASSERT(m_read_locks == 0);

  if (m_weak_lock) {
    *m_weak_lock = WeakLock::WeakUnlocked;
    m_weak_lock = nullptr;
  }
}

bool RWLock::upgradeToWrite(int timeout)
{
  while (timeout >= 0) {
    {
      scoped_lock lock(m_mutex);

      // Check that there is no weak lock
      if (m_weak_lock) {
        if (*m_weak_lock == WeakLocked)
          *m_weak_lock = WeakUnlocking;

        // Wait some time
        if (*m_weak_lock == WeakUnlocking)
          goto go_wait;

        ASSERT(*m_weak_lock == WeakUnlocked);
      }

      // this only is possible if there are just one reader
      if (m_read_locks == 1) {
        ASSERT(!m_write_lock);
        m_read_locks = 0;
        m_write_lock = true;

#ifdef DEBUG_OBJECT_LOCKS
        TRACE("LCK: upgradeToWrite: Locked <%p> to write\n", this);
#endif

        return true;
      }

    go_wait:;
    }

    if (timeout > 0) {
      int delay = MIN(100, timeout);
      timeout -= delay;

#ifdef DEBUG_OBJECT_LOCKS
      TRACE("LCK: upgradeToWrite: wait 100 msecs for <%p>\n", this);
#endif

      base::this_thread::sleep_for(double(delay) / 1000.0);
    }
    else
      break;
  }

#ifdef DEBUG_OBJECT_LOCKS
  TRACE("LCK: upgradeToWrite: Cannot lock <%p> to write (has %d read locks and %d write locks)\n",
    this, m_read_locks, m_write_lock);
#endif

  return false;
}

} // namespace app
