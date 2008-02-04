/* Jinete - a GUI library
 * Copyright (C) 2003, 2004, 2005, 2007, 2008 David A. Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <allegro.h>
#include <assert.h>

#include "jinete/jthread.h"

#undef UNIX_LIKE
#if defined ALLEGRO_UNIX || \
    defined ALLEGRO_LINUX || \
    defined ALLEGRO_MACOSX || \
    defined ALLEGRO_BEOS || \
    defined ALLEGRO_QNX
#define UNIX_LIKE
#endif

#if defined ALLEGRO_WINDOWS
#include <winalleg.h>
#include <process.h>
#elif defined UNIX_LIKE
#include <pthread.h>
#endif

#if defined UNIX_LIKE
struct pthread_proxy_data {
  void (*proc)(JThread thread, void *data);
  void *data;
};

static void *pthread_proxy(void *arg)
{
  pthread_proxy_data *ptr = arg;
  void (*proc)(JThread thread, void *data) = ptr->proc;
  void *data = ptr->data;
  jfree(ptr);

  (*proc)(data);
  return NULL;
}
#endif
  
JThread jthread_new(void (*proc)(void *data), void *data)
{
#if defined ALLEGRO_WINDOWS
  return (JThread)_beginthread(proc, 0, data);
#elif defined UNIX_LIKE
  pthread_t thread = 0;
  struct pthread_proxy_data *ptr = jnew(struct pthread_proxy_data, 1);
  ptr->proc = proc;
  ptr->data = data;

  if (pthread_create(&thread, NULL, pthread_proxy, ptr))
    return NULL;
  else
    return (JThread)thread;
#else
  #error ASE doesn\'t support threads for your platform
#endif
}

void jthread_join(JThread thread)
{
#if defined ALLEGRO_WINDOWS
  WaitForSingleObject(thread, INFINITE);
#elif defined UNIX_LIKE
  pthread_join((pthread_t)thread, NULL);
#else
  #error ASE doesn\'t support threads for your platform
#endif
}
