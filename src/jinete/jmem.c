/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro/unicode.h>

#include "jinete/jbase.h"
#include "jinete/jmutex.h"

#if !defined MEMLEAK

/**********************************************************************/
/* with outleak detection */
/**********************************************************************/

void *jmalloc(unsigned long n_bytes)
{
  if (n_bytes) {
    void *mem;

    mem = malloc(n_bytes);
    if (mem)
      return mem;
  }

  return NULL;
}

void *jmalloc0(unsigned long n_bytes)
{
  if (n_bytes) {
    void *mem;

    mem = calloc(1, n_bytes);
    if (mem)
      return mem;
  }

  return NULL;
}

void *jrealloc(void *mem, unsigned long n_bytes)
{
  if (n_bytes) {
    void *newmem = realloc(mem, n_bytes);
    if (newmem)
      return newmem;
  }

  if (mem)
    jfree(mem);

  return NULL;
}

void jfree(void *mem)
{
  if (mem)
    free(mem);
}

char *jstrdup(const char *string)
{
  return ustrdup(string);
}

#else

/**********************************************************************/
/* with leak detection */
/**********************************************************************/

typedef struct slot_t
{
  void *backtrace[4];
  void *ptr;
  unsigned long size;
  struct slot_t *next;
} slot_t;

static slot_t *headslot;
static JMutex mutex;

void jmemleak_init()
{
  headslot = NULL;
  mutex = jmutex_new();
}

void jmemleak_exit()
{
  FILE *f = fopen("_ase_memlog.txt", "wt");
  slot_t *it;

  if (f) {
    /* memory leaks */
    for (it=headslot; it!=NULL; it=it->next) {
      fprintf(f,
	      "Leak:\n%p\n%p\n%p\n%p\nptr: %p, size: %lu\n",
	      it->backtrace[0],
	      it->backtrace[1],
	      it->backtrace[2],
	      it->backtrace[3],
	      it->ptr, it->size);
    }
    fclose(f);
  }

  jmutex_free(mutex);
}

static void addslot(void *ptr, unsigned long size)
{
  slot_t *p = malloc(sizeof(slot_t));

  p->backtrace[0] = __builtin_return_address(4); /* a GCC extension */
  p->backtrace[1] = __builtin_return_address(3);
  p->backtrace[2] = __builtin_return_address(2);
  p->backtrace[3] = __builtin_return_address(1);
  p->ptr = ptr;
  p->size = size;
  p->next = headslot;
  
  jmutex_lock(mutex);
  headslot = p;
  jmutex_unlock(mutex);
}

static void delslot(void *ptr)
{
  slot_t *it, *prev = NULL;

  jmutex_lock(mutex);

  for (it=headslot; it!=NULL; prev=it, it=it->next) {
    if (it->ptr == ptr) {
      if (prev)
	prev->next = it->next;
      else
	headslot = it->next;

      free(it);
      break;
    }
  }

  jmutex_unlock(mutex);
}

void *jmalloc(unsigned long n_bytes)
{
  if (n_bytes) {
    void *mem;

    mem = malloc(n_bytes);
    if (mem) {
      addslot(mem, n_bytes);
      return mem;
    }
  }

  return NULL;
}

void *jmalloc0(unsigned long n_bytes)
{
  if (n_bytes) {
    void *mem;

    mem = calloc(1, n_bytes);
    if (mem) {
      addslot(mem, n_bytes);
      return mem;
    }
  }

  return NULL;
}

void *jrealloc(void *mem, unsigned long n_bytes)
{
  if (n_bytes) {
    void *newmem = realloc(mem, n_bytes);
    if (newmem) {
      if (mem) delslot(mem);
      addslot(newmem, n_bytes);
      return newmem;
    }
  }

  if (mem)
    jfree(mem);

  return NULL;
}

void jfree(void *mem)
{
  delslot(mem);
  if (mem)
    free(mem);
}

char *jstrdup(const char *string)
{
  void *mem = ustrdup(string);
  if (mem)
    addslot(mem, strlen(mem));
  return mem;
}

#endif
