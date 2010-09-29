// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_SHARED_PTR_H_INCLUDED
#define BASE_SHARED_PTR_H_INCLUDED

// A class which wraps a pointer and keeps reference counting to
// automatically delete the pointed object when it is no longer
// used.
template<class T>
class SharedPtr
{
  friend class SharedPtr;

public:

  SharedPtr()
    : m_ptr(0)
    , m_refCount(0) {
  }

  explicit SharedPtr(T* ptr)
    : m_ptr(ptr)
    , m_refCount(ptr ? new int(0): 0) {
    ref();
  }

  SharedPtr(const SharedPtr<T>& other)
    : m_ptr(other.m_ptr)
    , m_refCount(other.m_refCount) {
    ref();
  }

  template<class T2>
  SharedPtr(const SharedPtr<T2>& other)
    : m_ptr(static_cast<T*>(other.m_ptr))
    , m_refCount(other.m_refCount) {
    ref();
  }

  virtual ~SharedPtr() {
    unref();
  }

  void reset(T* ptr = 0) {
    if (m_ptr != ptr) {
      unref();
      m_ptr = ptr;
      m_refCount = (ptr ? new int(0): 0);
      ref();
    }
  }

  SharedPtr& operator=(const SharedPtr<T>& other) {
    if (m_ptr != other.m_ptr) {
      unref();
      m_ptr = other.m_ptr;
      m_refCount = other.m_refCount;
      ref();
    }
    return *this;
  }

  template<class T2>
  SharedPtr& operator=(const SharedPtr<T2>& other) {
    if (m_ptr != static_cast<T*>(other.m_ptr)) {
      unref();
      m_ptr = static_cast<T*>(other.m_ptr);
      m_refCount = other.m_refCount;
      ref();
    }
    return *this;
  }

  inline T* get() const { return m_ptr; }
  inline T& operator*() const { return *m_ptr; }
  inline T* operator->() const { return m_ptr; }
  inline operator T*() const { return m_ptr; }

  int getRefCount() const { return m_refCount ? *m_refCount: 0; }

private:

  // Adds a reference to the pointee.
  void ref() {
    if (m_ptr)
      ++(*m_refCount);
  }

  // Removes the reference to the pointee.
  void unref() {
    if (m_ptr) {
      if (--(*m_refCount) == 0) {
	delete m_ptr;
	delete m_refCount;
      }
      m_ptr = 0;
      m_refCount = 0;
    }
  }

  
  T* m_ptr;			// The pointee object.
  int* m_refCount;		// Number of references.
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

#endif
