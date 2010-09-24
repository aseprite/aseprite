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

#include "Vaca/Referenceable.h"

#ifndef NDEBUG
  #include "base/mutex.h"
  #include "base/scoped_lock.h"
  #include <vector>
  #include <typeinfo>
  #ifdef VACA_ON_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
  #endif
#endif

#include <cassert>
#include <iostream>

using namespace Vaca;

#ifndef NDEBUG
  static Mutex s_mutex;
  static std::vector<Referenceable*> s_list;
#endif

/**
   Constructs a new referenceable object starting with zero references.
*/
Referenceable::Referenceable()
{
  m_refCount = 0;
#ifndef NDEBUG
  {
    ScopedLock hold(s_mutex);
    s_list.push_back(this);
  }
#endif
}

/**
   Destroys a referenceable object.

   When compiling with assertions it checks that the references'
   counter is really zero.
*/
Referenceable::~Referenceable()
{
#ifndef NDEBUG
  {
    ScopedLock hold(s_mutex);
    remove_from_container(s_list, this);
  }
#endif
  assert(m_refCount == 0);
}

/**
   Called by SharedPtr to destroy the referenceable.
*/
void Referenceable::destroy()
{
  delete this;
}

/**
   Makes a new reference to this object.

   You are responsible for removing references using the #unref
   member function. Remember that for each call to #ref that you made,
   there should be a corresponding #unref.

   @see unref
*/
void Referenceable::ref()
{
  ++m_refCount;
}

/**
   Deletes an old reference to this object.

   If assertions are activated this routine checks that the
   reference counter never get negative, because that implies
   an error of the programmer.

   @see ref
*/
unsigned Referenceable::unref()
{
  assert(m_refCount > 0);
  return --m_refCount;
}

/**
   Returns the current number of references that this object has.

   If it's zero you can delete the object safely.
*/
unsigned Referenceable::getRefCount()
{
  return m_refCount;
}

#ifndef NDEBUG
void Referenceable::showLeaks()
{
#ifdef VACA_ON_WINDOWS
  if (!s_list.empty())
    ::Beep(400, 100);
#endif

  for (std::vector<Referenceable*>::iterator
	 it=s_list.begin(); it!=s_list.end(); ++it) {
    std::clog << "leak Referenceable " << (*it) << "\n";
  }
}
#endif
