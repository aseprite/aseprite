/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "ui/list.h"
#include <gtest/gtest.h>

using namespace ui;

TEST(JList, AppendAndClear)
{
  JList q = jlist_new();
  ASSERT_TRUE(q != NULL);
  EXPECT_EQ(0, jlist_length(q));

  jlist_append(q, (void*)10);
  EXPECT_EQ(1, jlist_length(q));

  jlist_append(q, (void*)20);
  jlist_append(q, (void*)30);
  ASSERT_EQ(3, jlist_length(q));
  EXPECT_EQ((void*)10, jlist_nth_data(q, 0));
  EXPECT_EQ((void*)20, jlist_nth_data(q, 1));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 2));

  jlist_clear(q);
  EXPECT_EQ(0, jlist_length(q));

  jlist_free(q);
}

TEST(JList, Prepend)
{
  JList q = jlist_new();
  jlist_prepend(q, (void*)30);
  jlist_prepend(q, (void*)20);
  jlist_prepend(q, (void*)10);
  ASSERT_EQ(3, jlist_length(q));
  EXPECT_EQ((void*)10, jlist_nth_data(q, 0));
  EXPECT_EQ((void*)20, jlist_nth_data(q, 1));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 2));

  jlist_free(q);
}

TEST(JList, Insert)
{
  JList q = jlist_new();
  jlist_append(q, (void*)10);
  jlist_append(q, (void*)30);
  jlist_insert(q, (void*)20, 1);
  jlist_insert(q, (void*)50, 3);
  jlist_insert(q, (void*)40, 3);
  ASSERT_EQ(5, jlist_length(q));
  EXPECT_EQ((void*)10, jlist_nth_data(q, 0));
  EXPECT_EQ((void*)20, jlist_nth_data(q, 1));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 2));
  EXPECT_EQ((void*)40, jlist_nth_data(q, 3));
  EXPECT_EQ((void*)50, jlist_nth_data(q, 4));

  jlist_free(q);
}

TEST(JList, NthLink)
{
  JLink a, b, c;

  JList q = jlist_new();
  jlist_append(q, (void*)10);
  jlist_append(q, (void*)20);
  jlist_append(q, (void*)30);

  a = jlist_nth_link(q, 0);
  b = jlist_nth_link(q, 1);
  c = jlist_nth_link(q, 2);
  EXPECT_EQ((void*)10, a->data);
  EXPECT_EQ((void*)20, b->data);
  EXPECT_EQ((void*)30, c->data);

  jlist_free(q);
}

TEST(JList, InsertBefore)
{
  JLink a, b, c;

  JList q = jlist_new();
  jlist_append(q, (void*)20);
  jlist_append(q, (void*)40);
  jlist_append(q, (void*)60);

  a = jlist_nth_link(q, 0);
  b = jlist_nth_link(q, 1);
  c = jlist_nth_link(q, 2);

  jlist_insert_before(q, a, (void*)10);
  jlist_insert_before(q, b, (void*)30);
  jlist_insert_before(q, c, (void*)50);
  jlist_insert_before(q, NULL, (void*)70);
  ASSERT_EQ(7, jlist_length(q));
  EXPECT_EQ((void*)10, jlist_nth_data(q, 0));
  EXPECT_EQ((void*)20, jlist_nth_data(q, 1));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 2));
  EXPECT_EQ((void*)40, jlist_nth_data(q, 3));
  EXPECT_EQ((void*)50, jlist_nth_data(q, 4));
  EXPECT_EQ((void*)60, jlist_nth_data(q, 5));
  EXPECT_EQ((void*)70, jlist_nth_data(q, 6));

  jlist_free(q);
}

TEST(JList, RemoveAndRemoveAll)
{
  JList q = jlist_new();
  jlist_append(q, (void*)10);
  jlist_append(q, (void*)20);
  jlist_append(q, (void*)30);

  jlist_remove(q, (void*)20);
  ASSERT_EQ(2, jlist_length(q));
  EXPECT_EQ((void*)10, jlist_nth_data(q, 0));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 1));

  jlist_append(q, (void*)10);
  jlist_remove_all(q, (void*)10);
  ASSERT_EQ(1, jlist_length(q));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 0));

  jlist_free(q);
}

TEST(JList, RemoveLinkAndDeleteLink)
{
  JList q;
  JLink b;

  q = jlist_new();
  jlist_append(q, (void*)10);
  jlist_append(q, (void*)20);
  jlist_append(q, (void*)30);

  b = jlist_nth_link(q, 1);
  jlist_remove_link(q, b);
  EXPECT_EQ(2, jlist_length(q));

  jlist_delete_link(q, jlist_nth_link(q, 0));
  jlist_delete_link(q, jlist_nth_link(q, 0));
  EXPECT_EQ(0, jlist_length(q));

  delete b;
  jlist_free(q);
}

TEST(JList, Copy)
{
  JList q, r;

  q = jlist_new();
  jlist_append(q, (void*)10);
  jlist_append(q, (void*)20);
  jlist_append(q, (void*)30);
  ASSERT_EQ(3, jlist_length(q));
  EXPECT_EQ((void*)10, jlist_nth_data(q, 0));
  EXPECT_EQ((void*)20, jlist_nth_data(q, 1));
  EXPECT_EQ((void*)30, jlist_nth_data(q, 2));

  r = jlist_copy(q);
  ASSERT_EQ(3, jlist_length(r));
  EXPECT_EQ((void*)10, jlist_nth_data(r, 0));
  EXPECT_EQ((void*)20, jlist_nth_data(r, 1));
  EXPECT_EQ((void*)30, jlist_nth_data(r, 2));

  jlist_free(q);
  jlist_free(r);
}

TEST(JList, Find)
{
  JList q = jlist_new();
  jlist_append(q, (void*)10);
  jlist_append(q, (void*)20);
  jlist_append(q, (void*)30);

  EXPECT_EQ(jlist_nth_link(q, 0), jlist_find(q, (void*)10));
  EXPECT_EQ(jlist_nth_link(q, 1), jlist_find(q, (void*)20));
  EXPECT_EQ(jlist_nth_link(q, 2), jlist_find(q, (void*)30));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
