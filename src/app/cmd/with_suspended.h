// Aseprite
// Copyright (C) 2025-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_SUSPEND_H_INCLUDED
#define APP_CMD_WITH_SUSPEND_H_INCLUDED
#pragma once

#include "app/cmd_exception.h"
#include "app/cmd_serial.h"
#include "base/ints.h"
#include "doc/io.h"
#include "doc/object.h"

#include <type_traits>

namespace app { namespace cmd {

// Functions used to serialize suspended objects into the undo history.
void encode_suspended_object(doc::Object* obj, std::ostream& os);
doc::Object* decode_suspended_object(const doc::ObjectType type,
                                     std::istream& is,
                                     const doc::IdMapperIO& mapper,
                                     doc::SubObjectsIO* subObjects);

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
    if (!m_object)
      throw CmdException(fmt::format("No object ({}) to restore", typeid(T).name()));

    T object = m_object;

    m_object->restoreObject();
    m_object = nullptr;
    m_size = 0;

    return object;
  }

  void serializeObject(CmdSerial& s)
  {
    if (s.encoding()) {
      std::stringstream stream;
      encode_suspended_object(m_object, stream);
      s(stream);
    }
    else if (s.decoding()) {
      ASSERT(m_object == nullptr);

      std::stringstream stream;
      s(stream);
      m_object = static_cast<T>(
        decode_suspended_object(std::remove_pointer_t<T>::kType, stream, s, &s));
    }
  }

private:
  size_t m_size = 0;
  T m_object = nullptr;
};

}} // namespace app::cmd

#endif
