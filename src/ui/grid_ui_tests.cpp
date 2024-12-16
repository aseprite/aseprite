// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#define TEST_GUI
#include "gfx/rect_io.h"
#include "gfx/size.h"
#include "tests/app_test.h"

using namespace gfx;
using namespace ui;

// Tests two widgets in a row, each one of 10x10 pixels.
TEST(Grid, Simple2x1Grid)
{
  Grid* grid = new Grid(2, false);
  Widget* w1 = new Widget;
  Widget* w2 = new Widget;

  w1->setMinSize(gfx::Size(10, 10));
  w2->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 1, 1, 0);

  // Test request-size
  Size reqSize = grid->sizeHint();
  EXPECT_EQ(20, reqSize.w);
  EXPECT_EQ(10, reqSize.h);

  // Test child-spacing
  grid->setChildSpacing(2);
  reqSize = grid->sizeHint();
  EXPECT_EQ(22, reqSize.w);
  EXPECT_EQ(10, reqSize.h);

  // Test borders
  grid->setBorder(gfx::Border(3, 0, 0, 3));
  reqSize = grid->sizeHint();
  EXPECT_EQ(25, reqSize.w);
  EXPECT_EQ(13, reqSize.h);

  grid->setBorder(gfx::Border(3, 2, 5, 3));
  reqSize = grid->sizeHint();
  EXPECT_EQ(30, reqSize.w);
  EXPECT_EQ(15, reqSize.h);

  delete grid;
}

TEST(Grid, Expand2ndWidget)
{
  Grid* grid = new Grid(2, false);
  Widget* w1 = new Widget;
  Widget* w2 = new Widget;

  w1->setMinSize(gfx::Size(20, 20));
  w2->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 1, 1, HORIZONTAL | TOP);

  // Test request size
  Size reqSize = grid->sizeHint();
  EXPECT_EQ(30, reqSize.w);
  EXPECT_EQ(20, reqSize.h);

  // Test layout
  grid->setBounds(gfx::Rect(0, 0, 40, 20));

  EXPECT_EQ(0, w1->bounds().x);
  EXPECT_EQ(0, w1->bounds().y);
  EXPECT_EQ(20, w1->bounds().w);
  EXPECT_EQ(20, w1->bounds().h);

  EXPECT_EQ(20, w2->bounds().x);
  EXPECT_EQ(0, w2->bounds().y);
  EXPECT_EQ(20, w2->bounds().w);
  EXPECT_EQ(10, w2->bounds().h);

  delete grid;
}

TEST(Grid, SameWidth2x1Grid)
{
  Grid* grid = new Grid(2, true);
  Widget* w1 = new Widget;
  Widget* w2 = new Widget;

  w1->setMinSize(gfx::Size(20, 20));
  w2->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 1, 1, 0);

  // Test request size
  Size reqSize = grid->sizeHint();
  EXPECT_EQ(40, reqSize.w);
  EXPECT_EQ(20, reqSize.h);

  // Test layout
  grid->setBounds(gfx::Rect(0, 0, 60, 20));

  EXPECT_EQ(0, w1->bounds().x);
  EXPECT_EQ(30, w2->bounds().x);
  EXPECT_EQ(30, w1->bounds().w);
  EXPECT_EQ(30, w2->bounds().w);

  delete grid;
}

// Tests the next layout (a grid of 3x3 cells):
//
//        2 (separator)
//       _|_
//      |   |
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
//  2.align = HORIZONTAL
//  3.align = HORIZONTAL | VERTICAL
//  4.align = VERTICAL
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
TEST(Grid, Intrincate2x2Grid)
{
  Grid* grid = new Grid(3, false);
  Widget* w1 = new Widget;
  Widget* w2 = new Widget;
  Widget* w3 = new Widget;
  Widget* w4 = new Widget;

  w1->setMinSize(gfx::Size(10, 10));
  w2->setMinSize(gfx::Size(10, 10));
  w3->setMinSize(gfx::Size(10, 10));
  w4->setMinSize(gfx::Size(10, 10));

  grid->addChildInCell(w1, 1, 1, 0);
  grid->addChildInCell(w2, 2, 1, HORIZONTAL);
  grid->addChildInCell(w3, 2, 2, HORIZONTAL | VERTICAL);
  grid->addChildInCell(w4, 1, 2, VERTICAL);

  // Test request size
  grid->setChildSpacing(2);
  Size reqSize = grid->sizeHint();
  EXPECT_EQ(22, reqSize.w);
  EXPECT_EQ(22, reqSize.h);

  // Test layout
  grid->setBounds(gfx::Rect(0, 0, 100, 100));

  EXPECT_EQ(0, w1->bounds().x);
  EXPECT_EQ(0, w1->bounds().y);
  EXPECT_EQ(12, w2->bounds().x);
  EXPECT_EQ(0, w2->bounds().y);
  EXPECT_EQ(0, w3->bounds().x);
  EXPECT_EQ(12, w3->bounds().y);
  EXPECT_EQ(90, w4->bounds().x);
  EXPECT_EQ(12, w4->bounds().y);

  EXPECT_EQ(10, w1->bounds().w);
  EXPECT_EQ(10, w1->bounds().h);
  EXPECT_EQ(88, w2->bounds().w);
  EXPECT_EQ(10, w2->bounds().h);
  EXPECT_EQ(88, w3->bounds().w);
  EXPECT_EQ(88, w3->bounds().h);
  EXPECT_EQ(10, w4->bounds().w);
  EXPECT_EQ(88, w4->bounds().h);

  delete grid;
}

TEST(Grid, FourColumns)
{
  Grid grid(4, false);
  grid.noBorderNoChildSpacing();

  Widget a;
  Widget b;
  Widget c;
  Widget d;
  Widget e;
  a.setMinSize(gfx::Size(10, 10));
  b.setMinSize(gfx::Size(10, 10));
  c.setMinSize(gfx::Size(10, 10));
  d.setMinSize(gfx::Size(10, 10));
  e.setMinSize(gfx::Size(10, 10));

  grid.addChildInCell(&a, 1, 1, HORIZONTAL | VERTICAL);
  grid.addChildInCell(&b, 1, 1, HORIZONTAL | VERTICAL);
  grid.addChildInCell(&c, 1, 1, HORIZONTAL | VERTICAL);
  grid.addChildInCell(&d, 1, 1, HORIZONTAL | VERTICAL);
  grid.addChildInCell(&e, 4, 1, HORIZONTAL | VERTICAL);

  // Test request size
  grid.setChildSpacing(0);
  EXPECT_EQ(gfx::Size(40, 20), grid.sizeHint());

  // Test layout
  grid.setBounds(gfx::Rect(0, 0, 40, 20));
  EXPECT_EQ(gfx::Rect(0, 0, 10, 10), a.bounds());
  EXPECT_EQ(gfx::Rect(10, 0, 10, 10), b.bounds());
  EXPECT_EQ(gfx::Rect(20, 0, 10, 10), c.bounds());
  EXPECT_EQ(gfx::Rect(30, 0, 10, 10), d.bounds());
  EXPECT_EQ(gfx::Rect(0, 10, 40, 10), e.bounds());
}

TEST(Grid, OneFixedReduceThree)
{
  Grid grid(1, false);
  grid.noBorderNoChildSpacing();

  Widget a;
  Widget b;
  Widget c;
  Widget d;
  a.setMinSize(gfx::Size(10, 10));
  b.setMinSize(gfx::Size(10, 10));
  c.setMinSize(gfx::Size(10, 10));
  d.setMinSize(gfx::Size(10, 10));

  grid.addChildInCell(&a, 1, 1, HORIZONTAL);
  grid.addChildInCell(&b, 1, 1, HORIZONTAL | VERTICAL);
  grid.addChildInCell(&c, 1, 1, HORIZONTAL | VERTICAL);
  grid.addChildInCell(&d, 1, 1, HORIZONTAL | VERTICAL);

  // Test request size
  grid.setChildSpacing(0);
  EXPECT_EQ(gfx::Size(10, 40), grid.sizeHint());

  // Test bigger layout
  grid.setBounds(gfx::Rect(0, 0, 10, 100));
  EXPECT_EQ(gfx::Rect(0, 0, 10, 10), a.bounds());
  EXPECT_EQ(gfx::Rect(0, 10, 10, 30), b.bounds());
  EXPECT_EQ(gfx::Rect(0, 40, 10, 30), c.bounds());
  EXPECT_EQ(gfx::Rect(0, 70, 10, 30), d.bounds());

  // Test perfect layout
  grid.setBounds(gfx::Rect(0, 0, 10, 40));
  EXPECT_EQ(gfx::Rect(0, 0, 10, 10), a.bounds());
  EXPECT_EQ(gfx::Rect(0, 10, 10, 10), b.bounds());
  EXPECT_EQ(gfx::Rect(0, 20, 10, 10), c.bounds());
  EXPECT_EQ(gfx::Rect(0, 30, 10, 10), d.bounds());

  // Test reduced layout
  grid.setBounds(gfx::Rect(0, 0, 10, 16));
  EXPECT_EQ(gfx::Rect(0, 0, 10, 10), a.bounds());
  EXPECT_EQ(gfx::Rect(0, 10, 10, 2), b.bounds());
  EXPECT_EQ(gfx::Rect(0, 12, 10, 2), c.bounds());
  EXPECT_EQ(gfx::Rect(0, 14, 10, 2), d.bounds());
}

TEST(Grid, ReduceThree)
{
  Grid grid(1, false);
  grid.noBorderNoChildSpacing();

  Widget a;
  Widget b;
  Widget c;
  a.setMinSize(gfx::Size(10, 10));
  b.setMinSize(gfx::Size(10, 10));
  c.setMinSize(gfx::Size(10, 10));

  grid.addChildInCell(&a, 1, 1, HORIZONTAL);
  grid.addChildInCell(&b, 1, 1, HORIZONTAL);
  grid.addChildInCell(&c, 1, 1, HORIZONTAL);

  // Test request size
  grid.setChildSpacing(0);
  EXPECT_EQ(gfx::Size(10, 30), grid.sizeHint());

  // Test bigger layout (as these widgets aren't expandible, they
  // should have height=10)
  grid.setBounds(gfx::Rect(0, 0, 10, 90));
  EXPECT_EQ(gfx::Rect(0, 0, 10, 10), a.bounds());
  EXPECT_EQ(gfx::Rect(0, 10, 10, 10), b.bounds());
  EXPECT_EQ(gfx::Rect(0, 20, 10, 10), c.bounds());

  // Test reduced layout
  grid.setBounds(gfx::Rect(0, 0, 10, 12));
  EXPECT_EQ(gfx::Rect(0, 0, 10, 4), a.bounds());
  EXPECT_EQ(gfx::Rect(0, 4, 10, 4), b.bounds());
  EXPECT_EQ(gfx::Rect(0, 8, 10, 4), c.bounds());
}

TEST(Grid, Alignment)
{
  Grid grid(1, false);
  grid.noBorderNoChildSpacing();

  Widget a;
  a.setMinSize(gfx::Size(10, 10));

  grid.addChildInCell(&a, 1, 1, CENTER | BOTTOM | HORIZONTAL | VERTICAL);

  Size reqSize = grid.sizeHint();
  EXPECT_EQ(10, reqSize.w);
  EXPECT_EQ(10, reqSize.h);

  grid.setBounds(gfx::Rect(10, 10, 40, 10));
  EXPECT_EQ(gfx::Rect(25, 10, 10, 10), a.bounds());

  grid.setBounds(gfx::Rect(10, 10, 60, 20));
  EXPECT_EQ(gfx::Rect(35, 20, 10, 10), a.bounds());
}
