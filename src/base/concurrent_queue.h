// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_CONCURRENT_QUEUE_H_INCLUDED
#define BASE_CONCURRENT_QUEUE_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"

#include <queue>

namespace base {

  template<typename T>
  class concurrent_queue {
  public:
    concurrent_queue() {
    }

    ~concurrent_queue() {
    }

    bool empty() const {
      bool result;
      {
        scoped_lock hold(m_mutex);
        result = m_queue.empty();
      }
      return result;
    }

    void push(const T& value) {
      scoped_lock hold(m_mutex);
      m_queue.push(value);
    }

    bool try_pop(T& value) {
      if (!m_mutex.try_lock())
        return false;

      scoped_unlock unlock(m_mutex);
      if (m_queue.empty())
        return false;

      value = m_queue.front();
      m_queue.pop();
      return true;
    }

  private:
    std::queue<T> m_queue;
    mutable mutex m_mutex;

    DISABLE_COPYING(concurrent_queue);
  };

} // namespace base

#endif
