// SHE library
// Copyright (C) 2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_H_INCLUDED
#define SHE_H_INCLUDED

namespace she {

  class System {
  public:
    virtual ~System() { }
    virtual void dispose() = 0;
  };

  System* CreateSystem();

  template<typename T>
  class ScopedHandle {
  public:
    ScopedHandle(T* handle) : m_handle(handle) { }
    ~ScopedHandle() { m_handle->dispose(); }
  private:
    T* m_handle;

    // Cannot copy
    ScopedHandle(const ScopedHandle&);
    ScopedHandle& operator=(const ScopedHandle&);
  };

} // namespace she

#endif
