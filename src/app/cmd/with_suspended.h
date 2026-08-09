// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_SUSPEND_H_INCLUDED
#define APP_CMD_WITH_SUSPEND_H_INCLUDED
#pragma once

#include "doc/object.h"

#include <type_traits>

namespace app { namespace cmd {

// Auxiliary class to keep a doc::Object in memory but without IDs,
// and to restore all its IDs when it's required.
template<typename T>
class WithSuspended {
public:
  ~WithSuspended()
  {
    // If the object is suspended it's not included in the Sprite
    // hierarchy, so it will not be automatically deleted, thereby we
    // have to delete the object to avoid a memory leak.
    if constexpr (std::is_pointer_v<T>)
      delete m_object;
  }

  T object() { return m_object; }
  size_t size() const { return m_size; }

  void suspend(T object)
  {
    ASSERT(!m_object);

    m_object = object;
    m_object->suspendObject();
    m_size = m_object->getMemSize();
  }

  T restore()
  {
    ASSERT(m_object);
    T object = m_object;

    m_object->restoreObject();
    m_object = nullptr;
    m_size = 0;

    return object;
  }

private:
  size_t m_size = 0;
  T m_object = nullptr;
};

}} // namespace app::cmd

#endif
