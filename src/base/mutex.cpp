// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/mutex.h"

#ifdef WIN32
  #include "base/mutex_win32.h"
#else
  #include "base/mutex_pthread.h"
#endif

namespace base {

mutex::mutex()
  : m_impl(new mutex_impl)
{
}

mutex::~mutex()
{
  delete m_impl;
}

void mutex::lock()
{
  return m_impl->lock();
}

bool mutex::try_lock()
{
  return m_impl->try_lock();
}

void mutex::unlock()
{
  return m_impl->unlock();
}

} // namespace base
