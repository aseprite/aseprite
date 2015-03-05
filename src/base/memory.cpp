// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "base/mutex.h"
#include "base/scoped_lock.h"

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
#ifdef _MSC_VER
  return _strdup(string);
#else
  return strdup(string);
#endif
}

#else  // With leak detection

#define BACKTRACE_LEVELS 16

#ifdef _MSC_VER
  #include <windows.h>
  #include <dbghelp.h>

  typedef USHORT (WINAPI* RtlCaptureStackBackTraceType)(ULONG, ULONG, PVOID*, PULONG);
  static RtlCaptureStackBackTraceType pRtlCaptureStackBackTrace;
#endif

struct slot_t {
  void* backtrace[BACKTRACE_LEVELS];
  void* ptr;
  size_t size;
  struct slot_t* next;
};

static bool memleak_status = false;
static slot_t* headslot;
static base::mutex* mutex = NULL;

void base_memleak_init()
{
#ifdef _MSC_VER
  pRtlCaptureStackBackTrace =
    (RtlCaptureStackBackTraceType)(::GetProcAddress(
        ::LoadLibrary(L"kernel32.dll"),
        "RtlCaptureStackBackTrace"));
#endif

  assert(!memleak_status);

  headslot = NULL;
  mutex = new base::mutex();

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
      IMAGEHLP_SYMBOL64 header;
      char filename[MAX_SYM_NAME];
    } si;
    si.header.SizeOfStruct = sizeof(SYMBOL_INFO_EX);
    si.header.MaxNameLength = MAX_SYM_NAME;

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    ::SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

    HANDLE hproc = ::GetCurrentProcess();
    if (!::SymInitialize(hproc, NULL, TRUE))
      fprintf(f, "Error initializing SymInitialize()\nGetLastError = %d\n", ::GetLastError());

    char filename[MAX_PATH];
    ::GetModuleFileNameA(NULL, filename, sizeof(filename) / sizeof(filename[0]));
    ::SymLoadModule64(hproc, NULL, filename, NULL, 0, 0);
#endif

    // Memory leaks
    for (it=headslot; it!=NULL; it=it->next) {
      fprintf(f, "\nLEAK address: %p, size: %lu\n", it->ptr, it->size);

      for (int c=0; c<BACKTRACE_LEVELS; ++c) {
#ifdef _MSC_VER
        DWORD displacement;

        if (::SymGetLineFromAddr64(hproc, (DWORD)it->backtrace[c], &displacement, &line)) {
          si.header.Name[0] = 0;

          ::SymGetSymFromAddr64(hproc, (DWORD)it->backtrace[c], NULL, &si.header);

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
#elif defined(_MSC_VER)
  {
    for (int c=0; c<BACKTRACE_LEVELS; ++c)
      p->backtrace[c] = 0;

    pRtlCaptureStackBackTrace(0, BACKTRACE_LEVELS, p->backtrace, NULL);
  }
#else
  #error Not supported
#endif

  p->ptr = ptr;
  p->size = size;

  base::scoped_lock lock(*mutex);
  p->next = headslot;
  headslot = p;
}

static void delslot(void* ptr)
{
  if (!memleak_status)
    return;

  slot_t *it, *prev = NULL;

  assert(ptr != NULL);

  base::scoped_lock lock(*mutex);

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
