// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_SHARED_PTR_H_INCLUDED
#define BASE_SHARED_PTR_H_INCLUDED

// This class counts references for a SharedPtr.
class SharedPtrRefCounterBase
{
public:
  SharedPtrRefCounterBase() : m_count(0) { }
  virtual ~SharedPtrRefCounterBase() { }

  void add_ref()
  {
    ++m_count;
  }

  bool release()
  {
    --m_count;
    if (m_count == 0) {
      delete this;
      return true;
    }
    else
      return false;
  }

  long use_count() const
  {
    return m_count;
  }

private:
  long m_count;		// Number of references.
};

// Default deleter used by shared pointer (it calls "delete"
// operator).
template<class T>
class DefaultSharedPtrDeleter
{
public:
  void operator()(T* ptr)
  {
    delete ptr;
  }
};

// A reference counter with a custom deleter.
template<class T, class Deleter>
class SharedPtrRefCounterImpl : public SharedPtrRefCounterBase
{
public:
  SharedPtrRefCounterImpl(T* ptr, Deleter deleter)
    : m_ptr(ptr)
    , m_deleter(deleter)
  {
  }

  ~SharedPtrRefCounterImpl()
  {
    m_deleter(m_ptr);
  }

private:
  T* m_ptr;
  Deleter m_deleter;		// Used to destroy the pointer.
};

// Wraps a pointer and keeps reference counting to automatically
// delete the pointed object when it is no longer used.
template<class T>
class SharedPtr
{
public:
  typedef T element_type;

  SharedPtr()
    : m_ptr(0)
    , m_refCount(0)
  {
  }

  // Constructor with default deleter.
  explicit SharedPtr(T* ptr)
  {
    try {
      m_refCount = new SharedPtrRefCounterImpl<T, DefaultSharedPtrDeleter<T> >(ptr, DefaultSharedPtrDeleter<T>());
    }
    catch (...) {
      DefaultSharedPtrDeleter<T>()(ptr);
      throw;
    }
    m_ptr = ptr;
    add_ref();
  }

  // Constructor with customized deleter.
  template<class Deleter>
  SharedPtr(T* ptr, Deleter deleter)
  {
    try {
      m_refCount = new SharedPtrRefCounterImpl<T, Deleter>(ptr, deleter);
    }
    catch (...) {
      deleter(ptr);
      throw;
    }
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

  void reset(T* ptr = 0)
  {
    if (m_ptr != ptr) {
      release();
      if (ptr) {
	try {
	  m_refCount = new SharedPtrRefCounterImpl<T, DefaultSharedPtrDeleter<T> >(ptr, DefaultSharedPtrDeleter<T>());
	}
	catch (...) {
	  DefaultSharedPtrDeleter<T>()(ptr);
	  throw;
	}
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
      if (ptr) {
	try {
	  m_refCount = new SharedPtrRefCounterImpl<T, Deleter>(ptr, deleter);
	}
	catch (...) {
	  deleter(ptr);
	  throw;
	}
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
  operator T*() const { return m_ptr; }

  long use_count() const { return (m_refCount ? m_refCount->use_count(): 0); }
  bool unique() const { return use_count() == 1; }

private:

  // Adds a reference to the pointee.
  void add_ref()
  {
    if (m_refCount)
      m_refCount->add_ref();
  }

  // Removes the reference to the pointee.
  void release()
  {
    if (m_refCount && m_refCount->release()) {
      m_ptr = 0;
      m_refCount = 0;
    }
  }

  
  T* m_ptr;			       // The pointee object.
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

#endif
