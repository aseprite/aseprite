// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/thread.h"

#ifdef WIN32
  #include <windows.h>
  #include <process.h>
#else
  #include <pthread.h>		// Use pthread library in Unix-like systems
#endif

namespace {

#ifdef WIN32

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
  : m_native_handle(NULL)
  , m_id()
{
}

base::thread::~thread()
{
  if (joinable())
    detach();

#ifdef USE_PTHREADS
  delete (pthread_t*)m_native_handle;
#endif
}

bool base::thread::joinable() const
{
  return (m_id != this_thread::get_id());
}

void base::thread::join()
{
  if (joinable()) {
#ifdef WIN32
    ::WaitForSingleObject(m_native_handle, INFINITE);
#else
    ::pthread_join(*(pthread_t*)m_native_handle, NULL);
#endif
    detach();
  }
}

void base::thread::detach()
{
  if (joinable()) {
#ifdef WIN32
    ::CloseHandle(m_native_handle);
#else
    ::pthread_detach(*(pthread_t*)m_native_handle);
    delete (pthread_t*)m_native_handle;
#endif
    m_native_handle = NULL;
    m_id = id();
  }
}

void base::thread::launch_thread(func_wrapper* f)
{
#ifdef WIN32

  DWORD native_id;
  m_native_handle = ::CreateThread(NULL, 0, win32_thread_proxy, (LPVOID)f,
				   CREATE_SUSPENDED, &native_id);
  m_id.m_native_id = native_id;
  ResumeThread(m_native_handle);

#else

  pthread_t thread;
  if (::pthread_create(&thread, NULL, pthread_thread_proxy, f))
    m_native_handle = new pthread_t(thread);
  else
    m_native_handle = NULL;

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

base::thread::id base::thread::details::get_current_thread_id()
{
#ifdef WIN32
  return id(::GetCurrentThreadId());
#else
  return id((unsigned int)::pthread_self());
#endif
}

base::thread::id base::this_thread::get_id()
{
  return thread::details::get_current_thread_id();
}

void base::this_thread::yield()
{
#ifdef WIN32
  ::Sleep(0);
#else
  // TODO
#endif
}

void base::this_thread::sleep_for(int milliseconds)
{
#ifdef WIN32
  ::Sleep(milliseconds);
#else
  // TODO
#endif
}
