// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MUTEX_WIN32_H_INCLUDED
#define BASE_MUTEX_WIN32_H_INCLUDED
#pragma once

#include <windows.h>

class base::mutex::mutex_impl {
public:

  mutex_impl() {
    InitializeCriticalSection(&m_handle);
  }

  ~mutex_impl() {
    DeleteCriticalSection(&m_handle);
  }

  void lock() {
    EnterCriticalSection(&m_handle);
  }

  bool try_lock() {
#if(_WIN32_WINNT >= 0x0400)
    return TryEnterCriticalSection(&m_handle) ? true: false;
#else
    return false;
#endif
  }

  void unlock() {
    LeaveCriticalSection(&m_handle);
  }

private:
  CRITICAL_SECTION m_handle;
};

#endif
