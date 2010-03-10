// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VACA_SCOPEDLOCK_H
#define VACA_SCOPEDLOCK_H

#include "Vaca/base.h"
#include "Vaca/Mutex.h"
#include "Vaca/NonCopyable.h"

namespace Vaca {

/**
   An object to safely lock and unlock mutexes.

   The constructor of ScopedLock locks the mutex, the destructor
   unlocks the mutex. In this way you can safely use ScopedLock
   inside a try/catch block without worrying about to the lock
   state of the mutex.

   For example:
   @code
   try {
     ScopedLock hold(mutex);
     throw Exception();
   }
   catch (...) {
     // the mutex is unlocked here
   }
   // if you don't throw a exception, the mutex is unlocked here too
   @endcode

   @see Mutex, ConditionVariable
*/
class ScopedLock : private NonCopyable
{
  Mutex& m_mutex;

  // not defined
  ScopedLock();

public:

  /**
     Creates the ScopedLock locking the specified mutex.

     @param mutex
       Mutex to be hold by the ScopedLock's life-time.
  */
  ScopedLock(Mutex& mutex)
    : m_mutex(mutex)
  {
    m_mutex.lock();
  }

  /**
     Destroys the ScopedLock unlocking the held mutex.
  */
  ~ScopedLock()
  {
    m_mutex.unlock();
  }

  /**
     Returns which mutex is being held.
  */
  Mutex& getMutex() const
  {
    return m_mutex;
  }

};

} // namespace Vaca

#endif // VACA_SCOPEDLOCK_H
