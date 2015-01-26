/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtest/gtest.h>

#include "app/filename_formatter.h"

#include "base/path.h"

using namespace app;

TEST(FilenameFormatter, Basic)
{
  EXPECT_EQ("C:/temp/file.png",
    filename_formatter("{fullname}",
      "C:/temp/file.png"));

  EXPECT_EQ("file.png",
    filename_formatter("{name}",
      "C:/temp/file.png"));

  EXPECT_EQ("C:/temp/other.png",
    filename_formatter("{path}/other.png",
      "C:/temp/file.png"));
}

TEST(FilenameFormatter, WithoutFrame)
{
  EXPECT_EQ("C:/temp/file.png",
    filename_formatter("{path}/{title}.png",
      "C:/temp/file.ase"));

  EXPECT_EQ("C:/temp/file.png",
    filename_formatter("{path}/{title}{frame}.{extension}",
      "C:/temp/file.png"));

  EXPECT_EQ("C:/temp/file{frame}.png",
    filename_formatter("{path}/{title}{frame}.{extension}",
      "C:/temp/file.png", "", -1, false));

  EXPECT_EQ("C:/temp/file (Background).png",
    filename_formatter("{path}/{title} ({layer}).{extension}",
      "C:/temp/file.png", "Background"));
}

TEST(FilenameFormatter, WithFrame)
{
  EXPECT_EQ("C:/temp/file0.png",
    filename_formatter("{path}/{title}{frame}.{extension}",
      "C:/temp/file.png", "", 0));

  EXPECT_EQ("C:/temp/file1.png",
    filename_formatter("{path}/{title}{frame}.{extension}",
      "C:/temp/file.png", "", 1));

  EXPECT_EQ("C:/temp/file10.png",
    filename_formatter("{path}/{title}{frame}.{extension}",
      "C:/temp/file.png", "", 10));

  EXPECT_EQ("C:/temp/file0.png",
    filename_formatter("{path}/{title}{frame0}.{extension}",
      "C:/temp/file.png", "", 0));

  EXPECT_EQ("C:/temp/file1.png",
    filename_formatter("{path}/{title}{frame1}.{extension}",
      "C:/temp/file.png", "", 0));

  EXPECT_EQ("C:/temp/file2.png",
    filename_formatter("{path}/{title}{frame1}.{extension}",
      "C:/temp/file.png", "", 1));

  EXPECT_EQ("C:/temp/file00.png",
    filename_formatter("{path}/{title}{frame00}.{extension}",
      "C:/temp/file.png", "", 0));

  EXPECT_EQ("C:/temp/file01.png",
    filename_formatter("{path}/{title}{frame01}.{extension}",
      "C:/temp/file.png", "", 0));

  EXPECT_EQ("C:/temp/file002.png",
    filename_formatter("{path}/{title}{frame000}.{extension}",
      "C:/temp/file.png", "", 2));

  EXPECT_EQ("C:/temp/file0032.png",
    filename_formatter("{path}/{title}{frame0032}.{extension}",
      "C:/temp/file.png", "", 0));

  EXPECT_EQ("C:/temp/file-Background-2.png",
    filename_formatter("{path}/{title}-{layer}-{frame}.{extension}",
      "C:/temp/file.png", "Background", 2));
}

TEST(SetFrameFormat, Tests)
{
  EXPECT_EQ("{path}/{title}{frame1}.{extension}",
    set_frame_format("{path}/{title}{frame}.{extension}",
      "{frame1}"));

  EXPECT_EQ("{path}/{title}{frame01}.{extension}",
    set_frame_format("{path}/{title}{frame}.{extension}",
      "{frame01}"));

  EXPECT_EQ("{path}/{title}{frame}.{extension}",
    set_frame_format("{path}/{title}{frame01}.{extension}",
      "{frame}"));
}

TEST(AddFrameFormat, Tests)
{
  EXPECT_EQ(base::fix_path_separators("{path}/{title}{frame001}.{extension}"),
    add_frame_format("{path}/{title}.{extension}",
      "{frame001}"));

  EXPECT_EQ("{path}/{title}{frame1}.{extension}",
    add_frame_format("{path}/{title}{frame1}.{extension}",
      "{frame001}"));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
