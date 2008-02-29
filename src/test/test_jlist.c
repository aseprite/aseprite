/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include "test/test.h"

#include <allegro.h>

#include "jinete/jlist.h"

static void test_append_clear(void)
{
  JList q;

  q = jlist_new();
  assert(q != NULL);
  assert(jlist_length(q) == 0);

  jlist_append(q, (void *)10);
  assert(jlist_length(q) == 1);

  jlist_append(q, (void *)20);
  jlist_append(q, (void *)30);
  assert(jlist_length(q) == 3);
  assert(jlist_nth_data(q, 0) == (void *)10);
  assert(jlist_nth_data(q, 1) == (void *)20);
  assert(jlist_nth_data(q, 2) == (void *)30);

  jlist_clear(q);
  assert(jlist_length(q) == 0);

  jlist_free(q);
}

static void test_prepend(void)
{
  JList q;

  q = jlist_new();
  jlist_prepend(q, (void *)30);
  jlist_prepend(q, (void *)20);
  jlist_prepend(q, (void *)10);

  assert(jlist_length(q) == 3);
  assert(jlist_nth_data(q, 0) == (void *)10);
  assert(jlist_nth_data(q, 1) == (void *)20);
  assert(jlist_nth_data(q, 2) == (void *)30);

  jlist_free(q);
}

static void test_insert(void)
{
}

static void test_insert_before(void)
{
}

static void test_remove(void)
{
}

static void test_remove_all(void)
{
}

static void test_remove_link(void)
{
}

static void test_delete_link(void)
{
}

static void test_copy(void)
{
}

static void test_nth_link(void)
{
}

static void test_find(void)
{
}

int main(int argc, char *argv[])
{
  allegro_init();

  test_append_clear();
  test_prepend();
  test_insert();
  test_insert_before();
  test_remove();
  test_remove_all();
  test_remove_link();
  test_delete_link();
  test_copy();
  test_nth_link();
  test_find();

  allegro_exit();
  return 0;
}

END_OF_MAIN();
