// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>

#include "gui/jthread.h"

#if defined ALLEGRO_WINDOWS
  #include <winalleg.h>
  #include <process.h>
#elif defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX
  #include <pthread.h>
#endif

struct pthread_proxy_data {
  void (*proc)(void*);
  void* data;
};

#if defined ALLEGRO_WINDOWS
static DWORD WINAPI pthread_proxy(void* arg)
{
  pthread_proxy_data* ptr = reinterpret_cast<pthread_proxy_data*>(arg);
  void (*proc)(void*) = ptr->proc;
  void* data = ptr->data;
  jfree(ptr);

  (*proc)(data);
  return 0;
}
#elif defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX
static void* pthread_proxy(void* arg)
{
  pthread_proxy_data* ptr = reinterpret_cast<pthread_proxy_data*>(arg);
  void (*proc)(void*) = ptr->proc;
  void* data = ptr->data;
  jfree(ptr);

  (*proc)(data);
  return NULL;
}
#endif
  
JThread jthread_new(void (*proc)(void*), void* data)
{
  struct pthread_proxy_data *ptr = jnew(struct pthread_proxy_data, 1);
  ptr->proc = proc;
  ptr->data = data;

#if defined ALLEGRO_WINDOWS

  DWORD id;
  HANDLE thread = CreateThread(NULL, 0, pthread_proxy, ptr,
			       CREATE_SUSPENDED, &id);
  if (thread)
    ResumeThread(thread);
  return thread;

#elif defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX

  pthread_t thread = 0;
  if (pthread_create(&thread, NULL, pthread_proxy, ptr))
    return NULL;
  else
    return (JThread)thread;

#else
  #error ASE does not support threads for your platform
#endif
}

void jthread_join(JThread thread)
{
#if defined ALLEGRO_WINDOWS
  WaitForSingleObject(thread, INFINITE);
#elif defined ALLEGRO_UNIX || defined ALLEGRO_MACOSX
  pthread_join((pthread_t)thread, NULL);
#else
  #error ASE does not support threads for your platform
#endif
}
