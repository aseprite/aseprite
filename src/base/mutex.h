// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_MUTEX_H_INCLUDED
#define BASE_MUTEX_H_INCLUDED

#include "base/disable_copying.h"

class Mutex
{
public:
  Mutex();
  ~Mutex();

  void lock();
  bool tryLock();
  void unlock();

private:
  class MutexImpl;
  MutexImpl* m_impl;

  DISABLE_COPYING(Mutex);
};

#endif
