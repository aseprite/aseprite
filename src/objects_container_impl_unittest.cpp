/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include <gtest/gtest.h>

#include "objects_container_impl.h"

using namespace undo;

TEST(ObjectsContainerImpl, AddObjectReturnsSameIdForSameObject)
{
  ObjectsContainerImpl objs;
  int a, b;

  ObjectId idA = objs.addObject(&a);
  ObjectId idB = objs.addObject(&b);

  EXPECT_NE(idA, idB);

  EXPECT_EQ(idA, objs.addObject(&a));
  EXPECT_EQ(idB, objs.addObject(&b));
}

TEST(ObjectsContainerImpl, GetObjectAndRemoveObject)
{
  ObjectsContainerImpl objs;
  int a, b;

  ObjectId idA = objs.addObject(&a);
  ObjectId idB = objs.addObject(&b);
  EXPECT_EQ(&a, objs.getObjectT<int>(idA));
  EXPECT_EQ(&b, objs.getObjectT<int>(idB));

  objs.removeObject(idA);
  objs.removeObject(idB);
  EXPECT_THROW(objs.getObject(idA), ObjectNotFoundException);
  EXPECT_THROW(objs.getObject(idB), ObjectNotFoundException);

  EXPECT_THROW(objs.removeObject(idA), ObjectNotFoundException);
  EXPECT_THROW(objs.removeObject(idB), ObjectNotFoundException);
}

TEST(ObjectsContainerImpl, InsertExistObjectsThrows)
{
  ObjectsContainerImpl objs;
  int a, b;

  ObjectId id1 = objs.addObject(&a);
  ObjectId id2 = id1 + 1;

  // Existent ID and pointer
  EXPECT_THROW(objs.insertObject(id1, &a), ExistentObjectException);

  // Existent pointer
  EXPECT_THROW(objs.insertObject(id2, &a), ExistentObjectException);

  // Existent ID
  EXPECT_THROW(objs.insertObject(id1, &b), ExistentObjectException);

  // OK, new object with new ID
  EXPECT_NO_THROW(objs.insertObject(id2, &b));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
