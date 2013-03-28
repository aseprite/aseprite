// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_MEMORY_H_INCLUDED
#define BASE_MEMORY_H_INCLUDED

void* base_malloc (size_t bytes);
void* base_malloc0(size_t bytes);
void* base_realloc(void* mem, size_t bytes);
void  base_free   (void* mem);
char* base_strdup (const char* string);

#ifdef MEMLEAK
void base_memleak_init();
void base_memleak_exit();
#endif

#endif
