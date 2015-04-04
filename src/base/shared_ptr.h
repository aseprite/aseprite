// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SHARED_PTR_H_INCLUDED
#define BASE_SHARED_PTR_H_INCLUDED
#pragma once

#include "base/debug.h"

namespace base {

// This class counts references for a SharedPtr.
class SharedPtrRefCounterBase {
public:
  SharedPtrRefCounterBase() : m_count(0) { }
  virtual ~SharedPtrRefCounterBase() { }

  void add_ref() {
    ++m_count;
  }

  void release() {
    --m_count;
    if (m_count == 0)
      delete this;
  }

  long use_count() const {
    return m_count;
  }

private:
  long m_count;         // Number of references.
};

// Default deleter used by shared pointer (it calls "delete"
// operator).
template<class T>
class DefaultSharedPtrDeleter {
public:
  void operator()(T* ptr) {
    delete ptr;
  }
};

// A reference counter with a custom deleter.
template<class T, class Deleter>
class SharedPtrRefCounterImpl : public SharedPtrRefCounterBase {
public:
  SharedPtrRefCounterImpl(T* ptr, Deleter deleter)
    : m_ptr(ptr)
    , m_deleter(deleter) {
  }

  ~SharedPtrRefCounterImpl() {
    if (m_ptr)
      m_deleter(m_ptr);
  }

private:
  T* m_ptr;
  Deleter m_deleter;            // Used to destroy the pointer.
};

// Wraps a pointer and keeps reference counting to automatically
// delete the pointed object when it is no longer used.
template<class T>
class SharedPtr {
public:
  typedef T element_type;

  SharedPtr()
    : m_ptr(nullptr)
    , m_refCount(nullptr)
  {
  }

  // Constructor with default deleter.
  explicit SharedPtr(T* ptr)
  {
    create_refcount(ptr, DefaultSharedPtrDeleter<T>());
    m_ptr = ptr;
    add_ref();
  }

  // Constructor with customized deleter.
  template<class Deleter>
  SharedPtr(T* ptr, Deleter deleter)
  {
    create_refcount(ptr, deleter);
    m_ptr = ptr;
    add_ref();
  }

  // Copy other pointer
  SharedPtr(const SharedPtr<T>& other)
    : m_ptr(other.m_ptr)
    , m_refCount(other.m_refCount)
  {
    add_ref();
  }

  // Copy other pointer (of static_casteable type)
  template<class Y>
  SharedPtr(const SharedPtr<Y>& other)
    : m_ptr(static_cast<T*>(const_cast<Y*>(other.m_ptr)))
    , m_refCount(const_cast<SharedPtrRefCounterBase*>(other.m_refCount))
  {
    add_ref();
  }

  // Releases one reference from the pointee.
  virtual ~SharedPtr()
  {
    release();
  }

  void reset(T* ptr = nullptr)
  {
    if (m_ptr != ptr) {
      release();
      m_ptr = nullptr;
      m_refCount = nullptr;

      if (ptr) {
        create_refcount(ptr, DefaultSharedPtrDeleter<T>());
        m_ptr = ptr;
        add_ref();
      }
    }
  }

  template<class Deleter>
  void reset(T* ptr, Deleter deleter)
  {
    if (m_ptr != ptr) {
      release();
      m_ptr = nullptr;
      m_refCount = nullptr;

      if (ptr) {
        create_refcount(ptr, deleter);
        m_ptr = ptr;
        add_ref();
      }
    }
  }

  SharedPtr& operator=(const SharedPtr<T>& other)
  {
    if (m_ptr != other.m_ptr) {
      release();
      m_ptr = other.m_ptr;
      m_refCount = other.m_refCount;
      add_ref();
    }
    return *this;
  }

  template<class Y>
  SharedPtr& operator=(const SharedPtr<Y>& other)
  {
    if (m_ptr != static_cast<T*>(other.m_ptr)) {
      release();
      m_ptr = static_cast<T*>(const_cast<Y*>(other.m_ptr));
      m_refCount = const_cast<SharedPtrRefCounterBase*>(other.m_refCount);
      add_ref();
    }
    return *this;
  }

  T* get() const { return m_ptr; }
  T& operator*() const { return *m_ptr; }
  T* operator->() const { return m_ptr; }
  explicit operator bool() const { return (m_ptr != nullptr); }

  long use_count() const { return (m_refCount ? m_refCount->use_count(): 0); }
  bool unique() const { return use_count() == 1; }

private:

  template<typename Deleter>
  void create_refcount(T* ptr, Deleter deleter) {
    if (ptr) {
      try {
        m_refCount = new SharedPtrRefCounterImpl<T, Deleter>(ptr, deleter);
      }
      catch (...) {
        if (ptr)
          deleter(ptr);
        throw;
      }
    }
    else
      m_refCount = nullptr;
  }

  // Adds a reference to the pointee.
  void add_ref()
  {
    if (m_refCount)
      m_refCount->add_ref();

    ASSERT((m_refCount && m_ptr) || (!m_refCount && !m_ptr));
  }

  // Removes the reference to the pointee.
  void release()
  {
    if (m_refCount)
      m_refCount->release();

    ASSERT((m_refCount && m_ptr) || (!m_refCount && !m_ptr));
  }

  T* m_ptr;                            // The pointee object.
  SharedPtrRefCounterBase* m_refCount; // Number of references.

  template<class> friend class SharedPtr;
};

// Compares if two shared-pointers points to the same place (object,
// memory address).
template<class T>
bool operator==(const SharedPtr<T>& ptr1, const SharedPtr<T>& ptr2)
{
  return ptr1.get() == ptr2.get();
}

// Compares if two shared-pointers points to different places
// (objects, memory addresses).
template<class T>
bool operator!=(const SharedPtr<T>& ptr1, const SharedPtr<T>& ptr2)
{
  return ptr1.get() != ptr2.get();
}

} // namespace base

#endif
