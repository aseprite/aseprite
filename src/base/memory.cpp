// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "base/mutex.h"
#include "base/scoped_lock.h"

// It is used so MSVC doesn't complain about deprecated POSIX names.
#pragma warning(disable:4996)

using namespace std;

#if !defined MEMLEAK            // Without leak detection

void* base_malloc(size_t bytes)
{
  assert(bytes != 0);
  return malloc(bytes);
}

void* base_malloc0(size_t bytes)
{
  assert(bytes != 0);
  return calloc(1, bytes);
}

void* base_realloc(void* mem, size_t bytes)
{
  assert(bytes != 0);
  return realloc(mem, bytes);
}

void base_free(void* mem)
{
  assert(mem != NULL);
  free(mem);
}

char* base_strdup(const char* string)
{
  assert(string != NULL);
  return strdup(string);
}

#else  // With leak detection

#define BACKTRACE_LEVELS 16

#ifdef _MSC_VER

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
       mov ebx, DWORD PTR[esp+8]
__next:
       test ebx, ebx
       je  __break
       dec ebx
       mov eax, DWORD PTR[eax]
       cmp eax, 0xffff
       jbe __outofstack
       jmp __next
__outofstack:
       mov eax, 0
       jmp __done
__break:
       mov eax, DWORD PTR[eax+4]
__done:
       pop ebx
       ret
   }
}

#endif

struct slot_t
{
  void* backtrace[BACKTRACE_LEVELS];
  void* ptr;
  size_t size;
  struct slot_t* next;
};

static bool memleak_status = false;
static slot_t* headslot;
static Mutex* mutex = NULL;

void base_memleak_init()
{
  assert(!memleak_status);

  headslot = NULL;
  mutex = new Mutex();

  memleak_status = true;
}

void base_memleak_exit()
{
  assert(memleak_status);
  memleak_status = false;

  FILE* f = fopen("_ase_memlog.txt", "wt");
  slot_t* it;

  if (f != NULL) {
#ifdef _MSC_VER
    struct SYMBOL_INFO_EX {
      SYMBOL_INFO header;
      char filename[MAX_SYM_NAME];
    } si;
    si.header.SizeOfStruct = sizeof(SYMBOL_INFO_EX);

    IMAGEHLP_LINE line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

    HANDLE hproc = ::GetCurrentProcess();
    if (!::SymInitialize(hproc, NULL, false))
      fprintf(f, "Error initializing SymInitialize()\nGetLastError = %d\n", ::GetLastError());

    char filename[MAX_PATH];
    ::GetModuleFileName(NULL, filename, sizeof(filename));
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

        if (::SymGetLineFromAddr(hproc, (DWORD)it->backtrace[c], &displacement, &line)) {
          si.header.Name[0] = 0;

          ::SymFromAddr(hproc, (DWORD)it->backtrace[c], NULL, &si.header);

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

static void addslot(void* ptr, size_t size)
{
  if (!memleak_status)
    return;

  slot_t* p = reinterpret_cast<slot_t*>(malloc(sizeof(slot_t)));

  assert(ptr != NULL);
  assert(size != 0);

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

  ScopedLock lock(*mutex);
  p->next = headslot;
  headslot = p;
}

static void delslot(void* ptr)
{
  if (!memleak_status)
    return;

  slot_t *it, *prev = NULL;

  assert(ptr != NULL);

  ScopedLock lock(*mutex);

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

void* base_malloc(size_t bytes)
{
  assert(bytes != 0);

  void* mem = malloc(bytes);
  if (mem != NULL) {
    addslot(mem, bytes);
    return mem;
  }
  else
    return NULL;
}

void* base_malloc0(size_t bytes)
{
  assert(bytes != 0);

  void* mem = calloc(1, bytes);
  if (mem != NULL) {
    addslot(mem, bytes);
    return mem;
  }
  else
    return NULL;
}

void* base_realloc(void* mem, size_t bytes)
{
  assert(bytes != 0);

  void* newmem = realloc(mem, bytes);
  if (newmem != NULL) {
    if (mem != NULL)
      delslot(mem);

    addslot(newmem, bytes);
    return newmem;
  }
  else
    return NULL;
}

void base_free(void* mem)
{
  assert(mem != NULL);

  delslot(mem);
  free(mem);
}

char* base_strdup(const char* string)
{
  assert(string != NULL);

  char* mem = strdup(string);
  if (mem != NULL)
    addslot(mem, strlen(mem) + 1);

  return mem;
}

#endif
