/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <stdlib.h>
#include <string.h>

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
    mem = realloc(mem, n_bytes);
    if (mem)
      return mem;
  }

  if (mem)
    free(mem);

  return NULL;
}

void jfree(void *mem)
{
  if (mem)
    free(mem);
}

char *jstrdup(const char *string)
{
  return strdup(string);
}

