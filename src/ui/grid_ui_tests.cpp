// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#define TEST_GUI
#include "tests/test.h"
#include "gfx/size.h"

using namespace gfx;
using namespace ui;

// Tests two widgets in a row, each one of 10x10 pixels.
TEST(JGrid, Simple2x1Grid)
{
  Grid* grid = new Grid(2, false);
  Widget* w1 = new Widget(kGenericWidget);
  Widget* w2 = new Widget(kGenericWidget);

  w1->setMinSize(gfx::Size(10, 10));
  w2->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 1, 1, 0);

  // Test request-size
  Size reqSize = grid->getPreferredSize();
  EXPECT_EQ(20, reqSize.w);
  EXPECT_EQ(10, reqSize.h);

  // Test child-spacing
  grid->child_spacing = 2;
  reqSize = grid->getPreferredSize();
  EXPECT_EQ(22, reqSize.w);
  EXPECT_EQ(10, reqSize.h);

  // Test borders
  grid->border_width.l = 3;
  grid->border_width.b = 3;
  reqSize = grid->getPreferredSize();
  EXPECT_EQ(25, reqSize.w);
  EXPECT_EQ(13, reqSize.h);

  grid->border_width.r = 5;
  grid->border_width.t = 2;
  reqSize = grid->getPreferredSize();
  EXPECT_EQ(30, reqSize.w);
  EXPECT_EQ(15, reqSize.h);

  delete grid;
}

TEST(JGrid, Expand2ndWidget)
{
  Grid* grid = new Grid(2, false);
  Widget* w1 = new Widget(kGenericWidget);
  Widget* w2 = new Widget(kGenericWidget);

  w1->setMinSize(gfx::Size(20, 20));
  w2->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 1, 1, JI_HORIZONTAL | JI_TOP);

  // Test request size
  Size reqSize = grid->getPreferredSize();
  EXPECT_EQ(30, reqSize.w);
  EXPECT_EQ(20, reqSize.h);

  // Test layout
  grid->setBounds(gfx::Rect(0, 0, 40, 20));

  EXPECT_EQ(0, w1->getBounds().x);
  EXPECT_EQ(0, w1->getBounds().y);
  EXPECT_EQ(20, w1->getBounds().w);
  EXPECT_EQ(20, w1->getBounds().h);

  EXPECT_EQ(20, w2->getBounds().x);
  EXPECT_EQ(0, w2->getBounds().y);
  EXPECT_EQ(20, w2->getBounds().w);
  EXPECT_EQ(10, w2->getBounds().h);

  delete grid;
}

TEST(JGrid, SameWidth2x1Grid)
{
  Grid* grid = new Grid(2, true);
  Widget* w1 = new Widget(kGenericWidget);
  Widget* w2 = new Widget(kGenericWidget);

  w1->setMinSize(gfx::Size(20, 20));
  w2->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 1, 1, 0);

  // Test request size
  Size reqSize = grid->getPreferredSize();
  EXPECT_EQ(40, reqSize.w);
  EXPECT_EQ(20, reqSize.h);

  // Test layout
  grid->setBounds(gfx::Rect(0, 0, 60, 20));

  EXPECT_EQ(0, w1->getBounds().x);
  EXPECT_EQ(30, w2->getBounds().x);
  EXPECT_EQ(30, w1->getBounds().w);
  EXPECT_EQ(30, w2->getBounds().w);

  delete grid;
}

// Tests the next layout (a grid of 3x3 cells):
//
//        2 (separator)
//       _|_
//      /   \
//   10 2 0 2 10
//  +---+-------+
//  | 1 | 2     | 10
//  +---+---+---+ --- 2 (separator)
//  | 3     | 4 | 4
//  |       |   | --- 2 (separator)
//  |       |   | 4
//  +-------+---+
//
//  1.align = 0
//  2.align = JI_HORIZONTAL
//  3.align = JI_HORIZONTAL | JI_VERTICAL
//  4.align = JI_VERTICAL
//
//
// When we expand the grid we get the following layout:
//
//  +---+----------------------------+
//  | 1 | 2                          |
//  +---+------------------------+---+
//  | 3                          | 4 |
//  |                            |   |
//  |                            |   |
//  |                            |   |
//  |                            |   |
//  |                            |   |
//  |                            |   |
//  +----------------------------+---+
//
TEST(JGrid, Intrincate3x3Grid)
{
  Grid* grid = new Grid(3, false);
  Widget* w1 = new Widget(kGenericWidget);
  Widget* w2 = new Widget(kGenericWidget);
  Widget* w3 = new Widget(kGenericWidget);
  Widget* w4 = new Widget(kGenericWidget);

  w1->setMinSize(gfx::Size(10, 10));
  w2->setMinSize(gfx::Size(10, 10));
  w3->setMinSize(gfx::Size(10, 10));
  w4->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 2, 1, JI_HORIZONTAL);
  grid->addChildInCell(w3, 2, 2, JI_HORIZONTAL | JI_VERTICAL);
  grid->addChildInCell(w4, 1, 2, JI_VERTICAL);

  // Test request size
  grid->child_spacing = 2;
  Size reqSize = grid->getPreferredSize();
  EXPECT_EQ(22, reqSize.w);
  EXPECT_EQ(22, reqSize.h);

  // Test layout
  grid->setBounds(gfx::Rect(0, 0, 100, 100));

  EXPECT_EQ(0, w1->getBounds().x);
  EXPECT_EQ(0, w1->getBounds().y);
  EXPECT_EQ(12, w2->getBounds().x);
  EXPECT_EQ(0, w2->getBounds().y);
  EXPECT_EQ(0, w3->getBounds().x);
  EXPECT_EQ(12, w3->getBounds().y);
  EXPECT_EQ(90, w4->getBounds().x);
  EXPECT_EQ(12, w4->getBounds().y);

  EXPECT_EQ(10, w1->getBounds().w);
  EXPECT_EQ(10, w1->getBounds().h);
  EXPECT_EQ(88, w2->getBounds().w);
  EXPECT_EQ(10, w2->getBounds().h);
  EXPECT_EQ(88, w3->getBounds().w);
  EXPECT_EQ(88, w3->getBounds().h);
  EXPECT_EQ(10, w4->getBounds().w);
  EXPECT_EQ(88, w4->getBounds().h);

  delete grid;
}
