// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/path.h"

#include <vector>

#ifdef WIN32
#include <windows.h>
#else
#include <fcntl.h>
#endif

using namespace base;

#pragma warning (disable: 4996)

TEST(FileHandle, Descriptors)
{
  const char* fn = "test.txt";

  // Delete the file if it exists.
  ASSERT_NO_THROW({
      if (file_exists(fn))
        delete_file(fn);
    });

  // Create file.
  ASSERT_NO_THROW({
      int fd = open_file_descriptor_with_exception(fn, "wb");
      close(fd);
    });

  // Truncate file.
  ASSERT_NO_THROW({
      int fd = open_file_descriptor_with_exception(fn, "wb");
      EXPECT_EQ(6, write(fd, "hello", 6));
      close(fd);
    });

  // Read.
  ASSERT_NO_THROW({
      int fd = open_file_descriptor_with_exception(fn, "rb");
      std::vector<char> buf(6);
      EXPECT_EQ(6, read(fd, &buf[0], 6));
      EXPECT_EQ("hello", std::string(&buf[0]));
      close(fd);
    });

  ASSERT_NO_THROW({
      delete_file(fn);
    });
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
