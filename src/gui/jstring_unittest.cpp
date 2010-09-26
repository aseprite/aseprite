/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "gui/jstring.h"

TEST(JString, has_extension)
{
  EXPECT_TRUE (jstring("hi.png").has_extension("png"));
  EXPECT_FALSE(jstring("hi.png").has_extension("pngg"));
  EXPECT_FALSE(jstring("hi.png").has_extension("ppng"));
  EXPECT_TRUE (jstring("hi.jpeg").has_extension("jpg,jpeg"));
  EXPECT_TRUE (jstring("hi.jpg").has_extension("jpg,jpeg"));
  EXPECT_FALSE(jstring("hi.ase").has_extension("jpg,jpeg"));
  EXPECT_TRUE (jstring("hi.ase").has_extension("jpg,jpeg,ase"));
  EXPECT_TRUE (jstring("hi.ase").has_extension("ase,jpg,jpeg"));
}
