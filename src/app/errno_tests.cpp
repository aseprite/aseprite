// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include <cerrno>
#include <thread>

static void run_thread()
{
  errno = 0;
  EXPECT_EQ(0, errno);
}

TEST(Errno, ThreadSafe)
{
  errno = 33;
  EXPECT_EQ(33, errno);

  // Run another thread that will be modify the errno variable, and
  // wait it (join).
  std::thread thr(&run_thread);
  thr.join();

  // See if errno was not modified in this thread.
  EXPECT_EQ(33, errno);
}
