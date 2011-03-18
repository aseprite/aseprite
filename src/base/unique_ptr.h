// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_UNIQUE_PTR_H_INCLUDED
#define BASE_UNIQUE_PTR_H_INCLUDED

#include "base/disable_copying.h"

// Default deleter used by unique pointers (it calls "delete" operator).
template<class T>
class DefaultUniquePtrDeleter
{
public:
  void operator()(T* ptr)
  {
    delete ptr;
  }
};

// Keeps the life time of the pointee object between the scope of the
// UniquePtr instance.
template<class T, class D = DefaultUniquePtrDeleter<T> >
class UniquePtr
{
public:
  typedef T* pointer;
  typedef T element_type;
  typedef D deleter_type;

  UniquePtr()
    : m_ptr(pointer())
  {
  }

  // Constructor with default deleter.
  explicit UniquePtr(pointer ptr)
    : m_ptr(ptr)
  {
  }

  // Constructor with customized deleter.
  UniquePtr(pointer ptr, deleter_type deleter)
    : m_ptr(ptr)
    , m_deleter(deleter)
  {
  }

  // Releases one reference from the pointee.
  ~UniquePtr()
  {
    if (m_ptr)
      m_deleter(m_ptr);
  }

  void reset(pointer ptr = pointer())
  {
    if (m_ptr)
      m_deleter(m_ptr);
    m_ptr = ptr;
  }

  pointer release()
  {
    pointer ptr(m_ptr);
    m_ptr = pointer();
    return ptr;
  }

  pointer get() const { return m_ptr; }
  element_type& operator*() const { return *m_ptr; }
  pointer operator->() const { return m_ptr; }
  operator pointer() const { return m_ptr; }

private:
  pointer m_ptr;			 // The pointee object.
  deleter_type m_deleter;		 // The deleter.

  DISABLE_COPYING(UniquePtr);
};

#endif
