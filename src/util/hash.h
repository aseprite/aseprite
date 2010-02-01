/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef UTIL_HASH_H_INCLUDED
#define UTIL_HASH_H_INCLUDED

typedef struct HashBucket
{
  char* key;
  void* data;
  struct HashBucket *next;
} HashBucket;

typedef struct HashTable
{
  int size;
  HashBucket **table;
} HashTable;

HashTable* hash_new(int size);
void hash_free(HashTable* table, void (*func)(void*));

void* hash_insert(HashTable* table, const char* key, void* data);
void* hash_lookup(HashTable* table, const char* key);
void* hash_remove(HashTable* table, const char* key);
void hash_enumerate(HashTable* table, void (*callback)(const char*, void*));

#endif
