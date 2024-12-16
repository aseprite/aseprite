// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/app_test.h"

#include "app/filename_formatter.h"

#include "base/fs.h"

using namespace app;

TEST(FilenameFormatter, Basic)
{
  EXPECT_EQ("C:/temp/file.png",
            filename_formatter("{fullname}", FilenameInfo().filename("C:/temp/file.png")));

  EXPECT_EQ("file.png", filename_formatter("{name}", FilenameInfo().filename("C:/temp/file.png")));

  EXPECT_EQ("C:/temp/other.png",
            filename_formatter("{path}/other.png", FilenameInfo().filename("C:/temp/file.png")));
}

TEST(FilenameFormatter, WithoutFrame)
{
  EXPECT_EQ("C:/temp/file.png",
            filename_formatter("{path}/{title}.png", FilenameInfo().filename("C:/temp/file.ase")));

  EXPECT_EQ("C:/temp/file.png",
            filename_formatter("{path}/{title}{frame}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png")));

  EXPECT_EQ("C:/temp/file{frame}.png",
            filename_formatter("{path}/{title}{frame}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png"),
                               false));

  EXPECT_EQ(
    "C:/temp/file (Background).png",
    filename_formatter("{path}/{title} ({layer}).{extension}",
                       FilenameInfo().filename("C:/temp/file.png").layerName("Background")));
}

TEST(FilenameFormatter, WithFrame)
{
  EXPECT_EQ("C:/temp/file0.png",
            filename_formatter("{path}/{title}{frame}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ("C:/temp/file1.png",
            filename_formatter("{path}/{title}{frame}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(1)));

  EXPECT_EQ("C:/temp/file10.png",
            filename_formatter("{path}/{title}{frame}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(10)));

  EXPECT_EQ("C:/temp/file0.png",
            filename_formatter("{path}/{title}{frame0}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ("C:/temp/file1.png",
            filename_formatter("{path}/{title}{frame1}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ("C:/temp/file2.png",
            filename_formatter("{path}/{title}{frame1}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(1)));

  EXPECT_EQ("C:/temp/file00.png",
            filename_formatter("{path}/{title}{frame00}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ("C:/temp/file01.png",
            filename_formatter("{path}/{title}{frame01}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ("C:/temp/file002.png",
            filename_formatter("{path}/{title}{frame000}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(2)));

  EXPECT_EQ("C:/temp/file0032.png",
            filename_formatter("{path}/{title}{frame0032}.{extension}",
                               FilenameInfo().filename("C:/temp/file.png").frame(0)));

  EXPECT_EQ("C:/temp/file-Background-2.png",
            filename_formatter(
              "{path}/{title}-{layer}-{frame}.{extension}",
              FilenameInfo().filename("C:/temp/file.png").layerName("Background").frame(2)));
}

TEST(FilenameFormatter, WithTagFrame)
{
  EXPECT_EQ("./file_2_0.png",
            filename_formatter("{path}/{title}_{frame}_{tagframe}.{extension}",
                               FilenameInfo().filename("./file.png").frame(2).tagFrame(0)));

  EXPECT_EQ("./file_2_1.png",
            filename_formatter("{path}/{title}_{frame}_{tagframe1}.{extension}",
                               FilenameInfo().filename("./file.png").frame(2).tagFrame(0)));

  EXPECT_EQ("./file_2_25.png",
            filename_formatter("{path}/{title}_{frame}_{tagframe24}.{extension}",
                               FilenameInfo().filename("./file.png").frame(2).tagFrame(1)));
}

TEST(FilenameFormatter, WithGroup)
{
  EXPECT_EQ("C:/temp/file (-Eyes).png",
            filename_formatter("{path}/{title} ({group}-{layer}).{extension}",
                               FilenameInfo().filename("C:/temp/file.png").layerName("Eyes")));
  EXPECT_EQ("C:/temp/file (Face-Eyes).png",
            filename_formatter(
              "{path}/{title} ({group}-{layer}).{extension}",
              FilenameInfo().filename("C:/temp/file.png").groupName("Face").layerName("Eyes")));
}

TEST(FilenameFormatter, GetFrameInfo)
{
  int frameBase, width;

  EXPECT_EQ(false, get_frame_info_from_filename_format("hi.png", nullptr, nullptr));

  frameBase = width = -1;
  EXPECT_EQ(true, get_frame_info_from_filename_format("hi{frame}.png", &frameBase, &width));
  EXPECT_EQ(0, frameBase);
  EXPECT_EQ(0, width);

  frameBase = width = -1;
  EXPECT_EQ(true, get_frame_info_from_filename_format("hi{frame1}.png", &frameBase, &width));
  EXPECT_EQ(1, frameBase);
  EXPECT_EQ(1, width);

  frameBase = width = -1;
  EXPECT_EQ(true, get_frame_info_from_filename_format("hi{frame032}.png", &frameBase, &width));
  EXPECT_EQ(32, frameBase);
  EXPECT_EQ(3, width);
}

TEST(FilenameFormatter, DefaultFormat)
{
  std::string fn;

  fn = "/path/hello.png";
  EXPECT_EQ("{title}.{extension}", get_default_filename_format(fn, false, false, false, false));
  EXPECT_EQ("/path/hello.png", fn);

  fn = "/path/hello.png";
  EXPECT_EQ("{path}/{title}.{extension}",
            get_default_filename_format(fn, true, false, false, false));
  EXPECT_EQ("/path/hello.png", fn);

  fn = "/path/hello.png";
  EXPECT_EQ("{path}/{title}{frame1}.{extension}",
            get_default_filename_format(fn, true, true, false, false));
  EXPECT_EQ("/path/hello.png", fn);

  fn = "/path/hello.gif";
  EXPECT_EQ("{path}/{title}.{extension}",
            get_default_filename_format(fn, true, true, false, false));
  EXPECT_EQ("/path/hello.gif", fn);

  fn = "/path/hello.png";
  EXPECT_EQ("{path}/{title} ({layer}) {frame}.{extension}",
            get_default_filename_format(fn, true, true, true, false));
  EXPECT_EQ("/path/hello.png", fn);

  fn = "/path/hello.gif";
  EXPECT_EQ("{path}/{title} ({layer}).{extension}",
            get_default_filename_format(fn, true, true, true, false));
  EXPECT_EQ("/path/hello.gif", fn);

  fn = "/path/hello1.png";
  EXPECT_EQ("{path}/{title}{frame1}.{extension}",
            get_default_filename_format(fn, true, true, false, false));
  EXPECT_EQ("/path/hello.png", fn);

  fn = "/path/hello1.gif";
  EXPECT_EQ("{path}/{title}.{extension}",
            get_default_filename_format(fn, true, true, false, false));
  EXPECT_EQ("/path/hello1.gif", fn);

  fn = "/path/hello001.png";
  EXPECT_EQ("{path}/{title}{frame001}.{extension}",
            get_default_filename_format(fn, true, true, false, false));
  EXPECT_EQ("/path/hello.png", fn);

  // When layers or tags are used in the filename format, the 1 is not converted to {frame1}
  fn = "/path/hello1.png";
  EXPECT_EQ("{path}/{title} #{tag} {frame}.{extension}",
            get_default_filename_format(fn, true, true, false, true));
  EXPECT_EQ("/path/hello1.png", fn);
}

TEST(FilenameFormatter, DetectFrame)
{
  std::string fn;

  fn = "/path/hello.png";
  EXPECT_EQ("/path/hello{frame1}.png", replace_frame_number_with_frame_format(fn));

  fn = "/path/hello0.png";
  EXPECT_EQ("/path/hello{frame0}.png", replace_frame_number_with_frame_format(fn));

  fn = "/path/hello1.png";
  EXPECT_EQ("/path/hello{frame1}.png", replace_frame_number_with_frame_format(fn));

  fn = "/path/hello002.png";
  EXPECT_EQ("/path/hello{frame002}.png", replace_frame_number_with_frame_format(fn));
}
