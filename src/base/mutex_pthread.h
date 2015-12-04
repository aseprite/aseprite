// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MUTEX_PTHREAD_H_INCLUDED
#define BASE_MUTEX_PTHREAD_H_INCLUDED
#pragma once

#include <pthread.h>
#include <errno.h>

class base::mutex::mutex_impl {
public:

  mutex_impl() {
    pthread_mutex_init(&m_handle, NULL);
  }

  ~mutex_impl() {
    pthread_mutex_destroy(&m_handle);
  }

  void lock() {
    pthread_mutex_lock(&m_handle);
  }

  bool try_lock() {
    return pthread_mutex_trylock(&m_handle) != EBUSY;
  }

  void unlock() {
    pthread_mutex_unlock(&m_handle);
  }

private:
  pthread_mutex_t m_handle;

};

#endif
