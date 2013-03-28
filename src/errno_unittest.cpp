/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include <gtest/gtest.h>

#include <errno.h>
#include "base/thread.h"

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
  base::thread thr(&run_thread);
  thr.join();

  // See if errno was not modified in this thread.
  EXPECT_EQ(33, errno);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
