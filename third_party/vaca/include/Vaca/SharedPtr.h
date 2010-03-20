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

#ifndef VACA_SHAREDPTR_H
#define VACA_SHAREDPTR_H

#include "Vaca/base.h"
#include "Vaca/Referenceable.h"

namespace Vaca {

/**
   A pointer which maintains reference counting and
   automatically deletes the pointed object when it is
   no longer referenced.

   What is a shared pointer? It is a class that wraps a pointer to a
   dynamically allocated object. Various shared pointers can share
   the same object counting references for that object. When the
   last shared pointer is destroyed, the object is automatically
   deleted.

   The SharedPtr is mainly used to wrap classes that handle
   graphics resources (like Brush, Pen, Image, Icon, etc.).

   @tparam T Must be of Referenceable type, because Referenceable has
	     the reference counter.
*/
template<class T>
class SharedPtr
{
  T* m_ptr;

public:

  SharedPtr() {
    m_ptr = NULL;
  }

  /*explicit*/ SharedPtr(T* ptr) {
    m_ptr = ptr;
    ref();
  }

  SharedPtr(const SharedPtr<T>& other) {
    m_ptr = other.get();
    ref();
  }

  template<class T2>
  SharedPtr(const SharedPtr<T2>& other) {
    m_ptr = static_cast<T*>(other.get());
    ref();
  }

  virtual ~SharedPtr() {
    unref();
  }

  void reset(T* ptr = NULL) {
    if (m_ptr != ptr) {
      unref();
      m_ptr = ptr;
      ref();
    }
  }

  SharedPtr& operator=(const SharedPtr<T>& other) {
    if (m_ptr != other.get()) {
      unref();
      m_ptr = other.get();
      ref();
    }
    return *this;
  }

  template<class T2>
  SharedPtr& operator=(const SharedPtr<T2>& other) {
    if (m_ptr != static_cast<T*>(other.get())) {
      unref();
      m_ptr = static_cast<T*>(other.get());
      ref();
    }
    return *this;
  }

  inline T* get() const { return m_ptr; }
  inline T& operator*() const { return *m_ptr; }
  inline T* operator->() const { return m_ptr; }
  inline operator T*() const { return m_ptr; }

private:

  void ref() {
    if (m_ptr)
      ((Referenceable*)m_ptr)->ref();
  }

  void unref() {
    if (m_ptr) {
      if (((Referenceable*)m_ptr)->unref() == 0)
	((Referenceable*)m_ptr)->destroy();
      m_ptr = NULL;
    }
  }
};

/**
   Compares if two shared-pointers points to the same place (object, memory address).

   @see @ref SharedPtr
*/
template<class T>
bool operator==(const SharedPtr<T>& ptr1, const SharedPtr<T>& ptr2)
{
  return ptr1.get() == ptr2.get();
}

/**
   Compares if two shared-pointers points to different places (objects, memory addresses).

   @see @ref SharedPtr
*/
template<class T>
bool operator!=(const SharedPtr<T>& ptr1, const SharedPtr<T>& ptr2)
{
  return ptr1.get() != ptr2.get();
}

} // namespace Vaca

#endif // VACA_SHAREDPTR_H
