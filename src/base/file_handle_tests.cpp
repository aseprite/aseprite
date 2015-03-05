// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include <gtest/gtest.h>

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/path.h"

#include <vector>

using namespace base;

#ifdef _MSC_VER
  #define posix_open   _open
  #define posix_close  _close
  #define posix_read   _read
  #define posix_write  _write
#else
  #define posix_open   open
  #define posix_close  close
  #define posix_read   read
  #define posix_write  write
#endif

TEST(FileHandle, Descriptors)
{
  const char* fn = "test.txt";

  // Delete the file if it exists.
  ASSERT_NO_THROW({
      if (is_file(fn))
        delete_file(fn);
    });

  // Create file.
  ASSERT_NO_THROW({
      int fd = open_file_descriptor_with_exception(fn, "wb");
      posix_close(fd);
    });

  // Truncate file.
  ASSERT_NO_THROW({
      int fd = open_file_descriptor_with_exception(fn, "wb");
      EXPECT_EQ(6, posix_write(fd, "hello", 6));
      posix_close(fd);
    });

  // Read.
  ASSERT_NO_THROW({
      int fd = open_file_descriptor_with_exception(fn, "rb");
      std::vector<char> buf(6);
      EXPECT_EQ(6, posix_read(fd, &buf[0], 6));
      EXPECT_EQ("hello", std::string(&buf[0]));
      posix_close(fd);
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
