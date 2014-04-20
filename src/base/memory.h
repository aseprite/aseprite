// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_MEMORY_H_INCLUDED
#define BASE_MEMORY_H_INCLUDED
#pragma once

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
