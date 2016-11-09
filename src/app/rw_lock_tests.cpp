// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/rw_lock.h"

using namespace app;

TEST(RWLock, MultipleReaders)
{
  RWLock a;
  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  a.unlock();
  a.unlock();
  a.unlock();
  a.unlock();
}

TEST(RWLock, OneWriter)
{
  RWLock a;

  EXPECT_TRUE(a.lock(RWLock::WriteLock, 0));
  EXPECT_FALSE(a.lock(RWLock::ReadLock, 0));
  a.unlock();

  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  a.unlock();

  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  a.unlock();
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  a.unlock();
  EXPECT_TRUE(a.lock(RWLock::WriteLock, 0));
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  a.unlock();
}

TEST(RWLock, UpgradeToWrite)
{
  RWLock a;

  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  EXPECT_TRUE(a.upgradeToWrite(0));
  EXPECT_FALSE(a.lock(RWLock::ReadLock, 0));
  EXPECT_FALSE(a.lock(RWLock::WriteLock, 0));
  a.downgradeToRead();
  a.unlock();
}

TEST(RWLock, WeakLock)
{
  RWLock a;
  RWLock::WeakLock flag = RWLock::WeakUnlocked;

  EXPECT_TRUE(a.weakLock(&flag));
  EXPECT_EQ(RWLock::WeakLocked, flag);
  EXPECT_FALSE(a.lock(RWLock::ReadLock, 0));
  EXPECT_EQ(RWLock::WeakUnlocking, flag);
  a.weakUnlock();
  EXPECT_EQ(RWLock::WeakUnlocked, flag);

  EXPECT_TRUE(a.lock(RWLock::ReadLock, 0));
  EXPECT_FALSE(a.weakLock(&flag));
  a.unlock();
}
