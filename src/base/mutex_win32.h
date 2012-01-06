// ASE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_MUTEX_WIN32_H_INCLUDED
#define BASE_MUTEX_WIN32_H_INCLUDED

#include <windows.h>

class Mutex::MutexImpl
{
public:

  MutexImpl() {
    InitializeCriticalSection(&m_handle);
  }

  ~MutexImpl() {
    DeleteCriticalSection(&m_handle);
  }

  void lock() {
    EnterCriticalSection(&m_handle);
  }

  bool tryLock() {
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
