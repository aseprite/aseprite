// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/thread.h"

#ifdef _WIN32
  #include <windows.h>
  #include <process.h>
#else
  #include <pthread.h>          // Use pthread library in Unix-like systems
#endif

#if !defined(_WIN32)
  #include <unistd.h>
  #include <sys/time.h>
#endif

namespace {

#ifdef _WIN32

  static DWORD WINAPI win32_thread_proxy(LPVOID data)
  {
    base::thread::details::thread_proxy(data);
    return 0;
  }

#else

  static void* pthread_thread_proxy(void* data)
  {
    base::thread::details::thread_proxy(data);
    return NULL;
  }

#endif

}

base::thread::thread()
  : m_native_handle((native_handle_type)0)
{
}

base::thread::~thread()
{
  if (joinable())
    detach();
}

bool base::thread::joinable() const
{
  return m_native_handle != (native_handle_type)0;
}

void base::thread::join()
{
  if (joinable()) {
#ifdef _WIN32
    ::WaitForSingleObject(m_native_handle, INFINITE);
#else
    ::pthread_join((pthread_t)m_native_handle, NULL);
#endif
    detach();
  }
}

void base::thread::detach()
{
  if (joinable()) {
#ifdef _WIN32
    ::CloseHandle(m_native_handle);
    m_native_handle = (native_handle_type)0;
#else
    ::pthread_detach((pthread_t)m_native_handle);
#endif
  }
}

base::thread::native_handle_type base::thread::native_handle()
{
  return m_native_handle;
}

void base::thread::launch_thread(func_wrapper* f)
{
  m_native_handle = (native_handle_type)0;

#ifdef _WIN32

  DWORD native_id;
  m_native_handle = ::CreateThread(NULL, 0, win32_thread_proxy, (LPVOID)f,
                                   CREATE_SUSPENDED, &native_id);
  ResumeThread(m_native_handle);

#else

  pthread_t thread;
  if (::pthread_create(&thread, NULL, pthread_thread_proxy, f) == 0)
    m_native_handle = (void*)thread;

#endif
}

void base::thread::details::thread_proxy(void* data)
{
  func_wrapper* f = reinterpret_cast<func_wrapper*>(data);

  // Call operator() of func_wrapper class (this is a virtual method).
  (*f)();

  // Delete the data (it was created in the thread() ctor).
  delete f;
}

void base::this_thread::yield()
{
#ifdef _WIN32

  ::Sleep(0);

#elif defined(HAVE_SCHED_YIELD) && defined(_POSIX_PRIORITY_SCHEDULING)

  sched_yield();

#else

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  select(0, NULL, NULL, NULL, &timeout);

#endif
}

void base::this_thread::sleep_for(double seconds)
{
#ifdef _WIN32

  ::Sleep(DWORD(seconds * 1000.0));

#else

  usleep(seconds * 1000000.0);

#endif
}

base::thread::native_handle_type base::this_thread::native_handle()
{
#ifdef _WIN32

  return GetCurrentThread();

#else

  return (void*)pthread_self();

#endif
}
