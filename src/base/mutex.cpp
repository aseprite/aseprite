// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/mutex.h"

#ifdef WIN32
  #include "base/mutex_win32.h"
#else
  #include "base/mutex_pthread.h"
#endif 

Mutex::Mutex()
  : m_impl(new MutexImpl)
{
}

Mutex::~Mutex()
{
  delete m_impl;
}

void Mutex::lock()
{
  return m_impl->lock();
}

bool Mutex::tryLock()
{
  return m_impl->tryLock();
}

void Mutex::unlock()
{
  return m_impl->unlock();
}
