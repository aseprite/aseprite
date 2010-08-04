/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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
 *   * Neither the name of the author nor the names of its contributors may
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro/unicode.h>

#include "Vaca/Mutex.h"
#include "Vaca/ScopedLock.h"

#if !defined MEMLEAK

/**********************************************************************/
/* with outleak detection */
/**********************************************************************/

void *jmalloc(unsigned long n_bytes)
{
  ASSERT(n_bytes != 0);
  return malloc(n_bytes);
}

void *jmalloc0(unsigned long n_bytes)
{
  ASSERT(n_bytes != 0);
  return calloc(1, n_bytes);
}

void *jrealloc(void *mem, unsigned long n_bytes)
{
  ASSERT(n_bytes != 0);
  return realloc(mem, n_bytes);
}

void jfree(void *mem)
{
  ASSERT(mem != NULL);
  free(mem);
}

char *jstrdup(const char *string)
{
  ASSERT(string != NULL);
  return ustrdup(string);
}

#else

//////////////////////////////////////////////////////////////////////
// With leak detection

#if defined(__GNUC__)
  #define BACKTRACE_LEVELS 4
#else
  #define BACKTRACE_LEVELS 16
#endif

#if defined _MSC_VER

#include <windows.h>
#include <dbghelp.h>

// This is an implementation of the __builtin_return_address GCC
// extension for the MSVC compiler.
// 
// Author: Unknown
// Modified by David Capello to return NULL when the callstack
// is not as high as the specified "level".
// 
__declspec (naked) void* __builtin_return_address(int level)
{
   __asm
   {
       push ebx

       mov eax, ebp
       mov ebx, DWORD PTR [esp + 8] // level
__next:
       test ebx, ebx
       je  __break
       dec ebx
       mov eax, DWORD PTR [eax]
       test eax, eax
       je  __done
       jmp __next
__break:
       mov eax, DWORD PTR [eax + 4]
__done:
       pop ebx
       ret
   }
}

#endif

typedef struct slot_t
{
  void* backtrace[BACKTRACE_LEVELS];
  void* ptr;
  unsigned long size;
  struct slot_t* next;
} slot_t;

using Vaca::Mutex;
using Vaca::ScopedLock;

static bool memleak_status = false;
static slot_t* headslot;
static Mutex mutex;

void _jmemleak_init()
{
  ASSERT(!memleak_status);

  headslot = NULL;
  mutex = new Mutex();

  memleak_status = true;
}

void _jmemleak_exit()
{
  ASSERT(memleak_status);
  memleak_status = false;

  FILE* f = fopen("_ase_memlog.txt", "wt");
  slot_t* it;
  
  if (f != NULL) {
#ifdef _MSC_VER
    struct SYMBOL_INFO_EX {
      SYMBOL_INFO header;
      char filename[512];
    } si;
    si.header.SizeOfStruct = sizeof(SYMBOL_INFO_EX);

    IMAGEHLP_LINE line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

    HANDLE hproc = ::GetCurrentProcess();
    ::SymInitialize(hproc, NULL, false);

    char filename[MAX_PATH];
    ::GetModuleFileName(NULL, filename, sizeof filename);
    ::SymLoadModule(hproc, NULL, filename, NULL, 0, 0);

    char path[MAX_PATH];
    strcpy(path, filename);
    if (strrchr(path, '\\'))
      *strrchr(path, '\\') = 0;
    else
      *path = 0;
    ::SymSetSearchPath(hproc, path);
#endif

    // Memory leaks
    for (it=headslot; it!=NULL; it=it->next) {
      fprintf(f, "\nLEAK address: %p, size: %lu\n", it->ptr, it->size);

      for (int c=0; c<BACKTRACE_LEVELS; ++c) {
#ifdef _MSC_VER
	DWORD displacement;
	if (::SymGetLineFromAddr(hproc, (DWORD)it->backtrace[c], &displacement, &line) &&
	    ::SymFromAddr(hproc, (DWORD)it->backtrace[c], NULL, &si.header)) {
	  fprintf(f, "%p : %s(%lu) [%s]\n",
		  it->backtrace[c],
		  line.FileName, line.LineNumber,
		  si.header.Name);
	}
	else
#endif
	  fprintf(f, "%p\n", it->backtrace[c]);
      }
    }
    fclose(f);

#ifdef _MSC_VER
    ::SymCleanup(hproc);
#endif
  }

  delete mutex;
}

static void addslot(void *ptr, unsigned long size)
{
  if (!memleak_status)
    return;

  slot_t* p = reinterpret_cast<slot_t*>(malloc(sizeof(slot_t)));

  ASSERT(ptr != NULL);
  ASSERT(size != 0);

  // __builtin_return_address is a GCC extension
#if defined(__GNUC__)
  p->backtrace[0] = __builtin_return_address(4);
  p->backtrace[1] = __builtin_return_address(3);
  p->backtrace[2] = __builtin_return_address(2);
  p->backtrace[3] = __builtin_return_address(1);
#else
  for (int c=0; c<BACKTRACE_LEVELS; ++c)
    p->backtrace[c] = __builtin_return_address(BACKTRACE_LEVELS-c);
#endif

  p->ptr = ptr;
  p->size = size;
  p->next = headslot;

  ScopedLock lock(mutex);
  headslot = p;
}

static void delslot(void *ptr)
{
  if (!memleak_status)
    return;

  slot_t *it, *prev = NULL;

  ASSERT(ptr != NULL);

  ScopedLock lock(mutex);

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
}

void *jmalloc(unsigned long n_bytes)
{
  void *mem;

  ASSERT(n_bytes != 0);

  mem = malloc(n_bytes);
  if (mem != NULL) {
    addslot(mem, n_bytes);
    return mem;
  }
  else
    return NULL;
}

void *jmalloc0(unsigned long n_bytes)
{
  void *mem;

  ASSERT(n_bytes != 0);

  mem = calloc(1, n_bytes);
  if (mem != NULL) {
    addslot(mem, n_bytes);
    return mem;
  }
  else
    return NULL;
}

void *jrealloc(void *mem, unsigned long n_bytes)
{
  void *newmem;

  ASSERT(n_bytes != 0);

  newmem = realloc(mem, n_bytes);
  if (newmem != NULL) {
    if (mem != NULL)
      delslot(mem);

    addslot(newmem, n_bytes);
    return newmem;
  }
  else
    return NULL;
}

void jfree(void *mem)
{
  ASSERT(mem != NULL);
  delslot(mem);
  free(mem);
}

char* jstrdup(const char *string)
{
  char* mem;

  ASSERT(string != NULL);

  mem = ustrdup(string);
  if (mem != NULL)
    addslot(mem, ustrsizez(mem));

  return mem;
}

#endif
