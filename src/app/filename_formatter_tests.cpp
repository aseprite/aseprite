// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/filename_formatter.h"

#include "base/fs.h"

using namespace app;

TEST(FilenameFormatter, Basic)
{
  EXPECT_EQ(
    "C:/temp/file.png",
    filename_formatter(
      "{fullname}",
      FilenameInfo().filename("C:/temp/file.png")));

  EXPECT_EQ(
    "file.png",
    filename_formatter(
      "{name}",
      FilenameInfo().filename("C:/temp/file.png")));

  EXPECT_EQ(
    "C:/temp/other.png",
    filename_formatter(
      "{path}/other.png",
      FilenameInfo().filename("C:/temp/file.png")));
}

TEST(FilenameFormatter, WithoutFrame)
{
  EXPECT_EQ(
    "C:/temp/file.png",
    filename_formatter(
      "{path}/{title}.png",
      FilenameInfo().filename("C:/temp/file.ase")));

  EXPECT_EQ(
    "C:/temp/file.png",
    filename_formatter(
      "{path}/{title}{frame}.{extension}",
      FilenameInfo().filename("C:/temp/file.png")));

  EXPECT_EQ(
    "C:/temp/file{frame}.png",
    filename_formatter(
      "{path}/{title}{frame}.{extension}",
      FilenameInfo().filename("C:/temp/file.png"), false));

  EXPECT_EQ(
    "C:/temp/file (Background).png",
    filename_formatter(
      "{path}/{title} ({layer}).{extension}",
      FilenameInfo().filename("C:/temp/file.png").layerName("Background")));
}

TEST(FilenameFormatter, WithFrame)
{
  EXPECT_EQ(
    "C:/temp/file0.png",
    filename_formatter(
      "{path}/{title}{frame}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ(
    "C:/temp/file1.png",
    filename_formatter(
      "{path}/{title}{frame}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(1)));

  EXPECT_EQ(
    "C:/temp/file10.png",
    filename_formatter(
      "{path}/{title}{frame}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(10)));

  EXPECT_EQ(
    "C:/temp/file0.png",
    filename_formatter(
      "{path}/{title}{frame0}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ(
    "C:/temp/file1.png",
    filename_formatter(
      "{path}/{title}{frame1}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ(
    "C:/temp/file2.png",
    filename_formatter(
      "{path}/{title}{frame1}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(1)));

  EXPECT_EQ(
    "C:/temp/file00.png",
    filename_formatter(
      "{path}/{title}{frame00}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ(
    "C:/temp/file01.png",
    filename_formatter(
      "{path}/{title}{frame01}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ(
    "C:/temp/file002.png",
    filename_formatter(
      "{path}/{title}{frame000}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(2)));

  EXPECT_EQ(
    "C:/temp/file0032.png",
    filename_formatter(
      "{path}/{title}{frame0032}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ(
    "C:/temp/file-Background-2.png",
    filename_formatter(
      "{path}/{title}-{layer}-{frame}.{extension}",
      FilenameInfo().filename("C:/temp/file.png").layerName("Background").frame(2)));
}

TEST(SetFrameFormat, Tests)
{
  EXPECT_EQ(
    "{path}/{title}{frame1}.{extension}",
    set_frame_format(
      "{path}/{title}{frame}.{extension}",
      "{frame1}"));

  EXPECT_EQ(
    "{path}/{title}{frame01}.{extension}",
    set_frame_format(
      "{path}/{title}{frame}.{extension}",
      "{frame01}"));

  EXPECT_EQ(
    "{path}/{title}{frame}.{extension}",
    set_frame_format(
      "{path}/{title}{frame01}.{extension}",
      "{frame}"));
}

TEST(AddFrameFormat, Tests)
{
  EXPECT_EQ(
    base::fix_path_separators("{path}/{title}{frame001}.{extension}"),
    add_frame_format(
      "{path}/{title}.{extension}",
      "{frame001}"));

  EXPECT_EQ(
    "{path}/{title}{frame1}.{extension}",
    add_frame_format(
      "{path}/{title}{frame1}.{extension}",
      "{frame001}"));
}

TEST(FilenameFormatter, WithTagFrame)
{
  EXPECT_EQ(
    "./file_2_0.png",
    filename_formatter(
      "{path}/{title}_{frame}_{tagframe}.{extension}",
      FilenameInfo().filename("./file.png").frame(2).tagFrame(0)));

  EXPECT_EQ(
    "./file_2_1.png",
    filename_formatter(
      "{path}/{title}_{frame}_{tagframe1}.{extension}",
      FilenameInfo().filename("./file.png").frame(2).tagFrame(0)));

  EXPECT_EQ(
    "./file_2_25.png",
    filename_formatter(
      "{path}/{title}_{frame}_{tagframe24}.{extension}",
      FilenameInfo().filename("./file.png").frame(2).tagFrame(1)));
}
