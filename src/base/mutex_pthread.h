// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_MUTEX_PTHREAD_H_INCLUDED
#define BASE_MUTEX_PTHREAD_H_INCLUDED

#include <pthread.h>
#include <errno.h>

class Mutex::MutexImpl
{
public:

  MutexImpl() {
    pthread_mutex_init(&m_handle, NULL);
  }

  ~MutexImpl() {
    pthread_mutex_destroy(&m_handle);
  }

  void lock() {
    pthread_mutex_lock(&m_handle);
  }

  bool tryLock() {
    return pthread_mutex_trylock(&m_handle) != EBUSY;
  }

  void unlock() {
    pthread_mutex_unlock(&m_handle);
  }

private:
  pthread_mutex_t m_handle;

};

#endif

