/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "tests/test.h"

#include "app/file/split_filename.h"

using namespace app;

TEST(SplitFilename, Common)
{
  std::string left, right;
  int width;

  EXPECT_EQ(1, split_filename("C:\\test\\a1.png", left, right, width));
  EXPECT_EQ("C:\\test\\a", left);
  EXPECT_EQ(".png", right);
  EXPECT_EQ(1, width);

  EXPECT_EQ(0, split_filename("file00.png", left, right, width));
  EXPECT_EQ("file", left);
  EXPECT_EQ(".png", right);
  EXPECT_EQ(2, width);

  EXPECT_EQ(32, split_filename("sprite1-0032", left, right, width));
  EXPECT_EQ("sprite1-", left);
  EXPECT_EQ("", right);
  EXPECT_EQ(4, width);
}
