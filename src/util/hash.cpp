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

/* Hash table functions, taken from Snippets.
 * Public domain code by Jerry Coffin, with improvements by HenkJan Wolthuis.
 *
 * Hacked to pieces by Peter Wang, June 1999.
 * ... and again in June 2000 (made to use Allegro Unicode string
 * 	functions, and fixed hash_enumerate)
 * ... and again in April 2001 (changed the hashing function, made
 *	shorter, and added scmp)
 * ... and again in January 2002 (removed use of Unicode string
 *	functions and scmp)
 *
 * Adapted to ASE by David A. Capello
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "jinete/jbase.h"

#include "util/hash.h"

/* These are used in freeing a table.  Perhaps I should code up
 * something a little less grungy, but it works, so what the heck.
 */
static void (*function)(void*) = NULL;
static HashTable* the_table = NULL;

static unsigned long hash(const unsigned char* str);
static void free_node(const char* key, void* data);

/* hash_new:
 *  Creates a new hash table to the size asked for.
 */
HashTable* hash_new(int size)
{
  HashTable* table;
  int i;

  table = jnew(HashTable, 1);
  if (!table)
    return NULL;

  table->table = jnew(HashBucket* , size);
  if (!table->table) {
    jfree(table);
    return NULL;
  }

  table->size = size;
  for (i=0; i<size; i++)
    table->table[i] = NULL;

  return table;
}

/* hash_free:
 *  Frees a complete table by iterating over it and freeing each node.
 *  The second parameter is the address of a function it will call with a
 *  pointer to the data associated with each node.  This function is
 *  responsible for freeing the data, or doing whatever is needed with
 *  it.
 */
void hash_free(HashTable* table, void (*func)(void* ))
{
  function = func;
  the_table = table;

  hash_enumerate(table, free_node);
  jfree(table->table);
  table->table = NULL;
  table->size = 0;

  the_table = NULL;
  function = NULL;

  jfree(table);
}

/* hash_insert:
 *  Insert 'key' into hash table.
 *  Returns pointer to old data associated with the key, if any, or
 *  NULL if the key wasn't in the table previously.
 */
void* hash_insert(HashTable* table, const char* key, void* data)
{
  unsigned int val = hash(reinterpret_cast<const unsigned char*>(key)) % table->size;
  HashBucket* ptr;
    
  /* See if the current string has already been inserted, and if so,
   * replace the old data.
   */
  for (ptr = table->table[val]; NULL != ptr; ptr = ptr->next)
    if (0 == strcmp(key, ptr->key)) {
      void* old_data = ptr->data;
      ptr->data = data;
      return old_data;
    }
    
  /* This key must not be in the table yet.  We'll add it to the head of
   * the list at this spot in the hash table.  Speed would be
   * slightly improved if the list was kept sorted instead.  In this case,
   * this code would be moved into the loop above, and the insertion would
   * take place as soon as it was determined that the present key in the
   * list was larger than this one.
   */
  ptr = jnew (HashBucket, 1);
  if (NULL == ptr)
    return NULL;
  ptr->key = jstrdup (key);
  ptr->data = data;
  ptr->next = table->table[val];
  table->table[val] = ptr;
  return NULL;
}

/* hash_lookup:
 *  Look up a key and return the associated data.  
 *  Returns NULL if the key is not in the table.
 */
void* hash_lookup(HashTable* table, const char* key)
{
  unsigned int val = hash(reinterpret_cast<const unsigned char*>(key)) % table->size;
  HashBucket* ptr;
    
  for (ptr = table->table[val]; NULL != ptr; ptr = ptr->next) 
    if (strcmp (key, ptr->key) == 0)
      return ptr->data;

  return NULL;
}

/* hash_remove:
 *  Delete a key from the hash table and return associated
 *  data, or NULL if not present.
 */
void* hash_remove(HashTable* table, const char* key)
{
  unsigned int val = hash(reinterpret_cast<const unsigned char*>(key)) % table->size;
  void* data;
  HashBucket* ptr;
  HashBucket* last = NULL;
    
  for (ptr = table->table[val]; NULL != ptr; ptr = ptr->next) {
    if (0 == strcmp(key, ptr->key)) {
      if (NULL != last)
	last->next = ptr->next;
      else
	table->table[val] = ptr->next;
      data = ptr->data;
      jfree(ptr->key);
      jfree(ptr);
      return data;
    }
    last = ptr;
  }

  return NULL;
}

/* hash_enumerate:
 *  Simply invokes the callback given as the second parameter for each
 *  node in the table, passing it the key and the associated data.
 */
void hash_enumerate(HashTable* table, void (*callback)(const char*, void*))
{
  int i;
  HashBucket* bucket;
  HashBucket* next;

  for (i = 0; i < table->size; i++)
    for (bucket = table->table[i]; NULL != bucket; bucket = next) {
      next = bucket->next;
      callback(bucket->key, bucket->data);
    }
}

/* hash: 
 *  Hashes a string.
 */
static unsigned long hash(const unsigned char* str)
{
  unsigned long hash = 0, c;
  while ((c = *str++)) 
    hash = (hash * 17) ^ c;
  return hash;
}

/* free_table iterates the table, calling this repeatedly to free
 * each individual node.  This, in turn, calls one or two other
 * functions - one to free the storage used for the key, the other
 * passes a pointer to the data back to a function defined by the user,
 * process the data as needed.
 */
static void free_node(const char* key, void* data)
{
  if (function)
    function(hash_remove(the_table, key));
  else
    hash_remove(the_table, key);
}
