/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010 David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __ASE_CONFIG_H
#error You cannot use config.h two times
#endif

#define __ASE_CONFIG_H
#define _CRT_SECURE_NO_WARNINGS

// General information
#define PACKAGE			"ASE"
#define VERSION			"0.8.1-beta"
#define ASE_BETA		1
#define WEBSITE			"http://www.aseprite.org/"
#define COPYRIGHT		"Copyright (C) 2001-2010 David Capello"

#define PRINTF			verbose_printf

// Defined in src/core/core.c
void verbose_printf(const char* format, ...);

// Strings
const char* msgids_get(const char* id); // src/intl/msgids.[ch]

#define _(msgid) (msgids_get(msgid))

#include <math.h>
#undef PI
#define PI 3.14159265358979323846

#include <allegro/base.h>
#include <allegro/debug.h>	// ASSERT

typedef uint8_t		ase_uint8;
typedef uint16_t	ase_uint16;
typedef uint32_t	ase_uint32;

//////////////////////////////////////////////////////////////////////
// Overloaded new/delete operators to detect memory-leaks

#if defined __cplusplus && defined MEMLEAK

#include <new>
#include "jinete/jbase.h"

inline void* operator new(std::size_t size)
{
  void* ptr = jmalloc(size);
  if (!ptr)
    throw std::bad_alloc();
  return ptr;
}

inline void operator delete(void* ptr)
{
  if (!ptr)
    return;
  jfree(ptr);
}

inline void* operator new[](std::size_t size)
{
  void* ptr = jmalloc(size);
  if (!ptr)
    throw std::bad_alloc();
  return ptr;
}

inline void operator delete[](void* ptr)
{
  if (!ptr)
    return;
  jfree(ptr);
}

#endif
