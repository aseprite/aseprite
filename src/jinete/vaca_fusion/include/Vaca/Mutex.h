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

#ifndef VACA_MUTEX_H
#define VACA_MUTEX_H

#include "Vaca/base.h"
#include "Vaca/NonCopyable.h"

namespace Vaca {

/**
   An object to synchronize threads using mutual exclusion of critical
   sections.

   This kind of mutex can be used to synchronize multiple threads of
   the same process. No multiple processes!

   @win32
     This is a @msdn{CRITICAL_SECTION} wrapper.
   @endwin32

   @see ScopedLock, ConditionVariable, Thread,
	@wikipedia{Critical_section, Critical Section in Wikipedia}
	@wikipedia{Mutex, Mutex in Wikipedia}
*/
class VACA_DLL Mutex : private NonCopyable
{
  class MutexPimpl;
  MutexPimpl* m_pimpl;

public:

  Mutex();
  ~Mutex();

  void lock();
  bool tryLock();
  void unlock();

};

} // namespace Vaca

#endif // VACA_MUTEX_H
