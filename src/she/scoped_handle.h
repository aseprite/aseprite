// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef SHE_SCOPED_HANDLE_H_INCLUDED
#define SHE_SCOPED_HANDLE_H_INCLUDED

namespace she {

  template<typename T>
  class ScopedHandle {
  public:
    ScopedHandle(T* handle) : m_handle(handle) { }
    ~ScopedHandle() { m_handle->dispose(); }

    T* operator->() { return m_handle; }
    operator T*() { return m_handle; }
  private:
    T* m_handle;

    // Cannot copy
    ScopedHandle(const ScopedHandle&);
    ScopedHandle& operator=(const ScopedHandle&);
  };

} // namespace she

#endif
