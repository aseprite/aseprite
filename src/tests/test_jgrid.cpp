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

#define TEST_GUI
#include "tests/test.h"

/**
 * Tests two widgets in a row, each one of 10x10 pixels.
 */
static void test_grid_2x1()
{
  JWidget grid = jgrid_new(2, false);
  JWidget w1 = jwidget_new(JI_WIDGET);
  JWidget w2 = jwidget_new(JI_WIDGET);
  int req_w, req_h;

  jwidget_set_min_size(w1, 10, 10);
  jwidget_set_min_size(w2, 10, 10);

  jgrid_add_child(grid, w1, 1, 1, 0);
  jgrid_add_child(grid, w2, 1, 1, 0);

  /* test request-size */
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 20 && req_h == 10);

  /* test child-spacing */
  grid->child_spacing = 2;
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 22 && req_h == 10);

  /* test borders */
  grid->border_width.l = 3;
  grid->border_width.b = 3;
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 25 && req_h == 13);

  grid->border_width.r = 5;
  grid->border_width.t = 2;
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 30 && req_h == 15);

  jwidget_free(grid);
}

static void test_grid_2x1_expand2nd()
{
  JWidget grid = jgrid_new(2, false);
  JWidget w1 = jwidget_new(JI_WIDGET);
  JWidget w2 = jwidget_new(JI_WIDGET);
  JRect rect;
  int req_w, req_h;

  jwidget_set_min_size(w1, 20, 20);
  jwidget_set_min_size(w2, 10, 10);

  jgrid_add_child(grid, w1, 1, 1, 0);
  jgrid_add_child(grid, w2, 1, 1, JI_HORIZONTAL | JI_TOP);

  /* test request size */
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 30 && req_h == 20);

  /* test layout */
  rect = jrect_new(0, 0, 40, 20);
  jwidget_set_rect(grid, rect);
  jrect_free(rect);

  ASSERT(w1->rc->x1 == 0);
  ASSERT(w1->rc->y1 == 0);
  ASSERT(jrect_w(w1->rc) == 20);
  ASSERT(jrect_h(w1->rc) == 20);

  ASSERT(w2->rc->x1 == 20);
  ASSERT(w2->rc->y1 == 0);
  ASSERT(jrect_w(w2->rc) == 20);
  ASSERT(jrect_h(w2->rc) == 10);

  jwidget_free(grid);
}

static void test_grid_2x1_samewidth()
{
  JWidget grid = jgrid_new(2, true);
  JWidget w1 = jwidget_new(JI_WIDGET);
  JWidget w2 = jwidget_new(JI_WIDGET);
  JRect rect;
  int req_w, req_h;

  jwidget_set_min_size(w1, 20, 20);
  jwidget_set_min_size(w2, 10, 10);

  jgrid_add_child(grid, w1, 1, 1, 0);
  jgrid_add_child(grid, w2, 1, 1, 0);

  /* test request size */
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 40 && req_h == 20);

  /* test layout */
  rect = jrect_new(0, 0, 60, 20);
  jwidget_set_rect(grid, rect);
  jrect_free(rect);

  ASSERT(w1->rc->x1 == 0);
  ASSERT(w2->rc->x1 == 30);
  ASSERT(jrect_w(w1->rc) == 30);
  ASSERT(jrect_w(w2->rc) == 30);

  jwidget_free(grid);
}

/**
 * Tests the next layout (a grid of 3x3 cells):
 * 
 * <pre>
 *        2 (separator)
 *       _|_
 *      /   \
 *   10 2 0 2 10
 *  +---+-------+
 *  | 1 | 2     | 10
 *  +---+---+---+ --- 2 (separator)
 *  | 3     | 4 | 4
 *  |       |   | --- 2 (separator)
 *  |       |   | 4
 *  +-------+---+
 *
 *  1.align = 0
 *  2.align = JI_HORIZONTAL
 *  3.align = JI_HORIZONTAL | JI_VERTICAL
 *  4.align = JI_VERTICAL
 * </pre>
 *
 * When expand the grid:
 *
 * <pre>
 *  +---+----------------------------+
 *  | 1 | 2       		     |
 *  +---+------------------------+---+
 *  | 3                	       	 | 4 |
 *  |            		 |   |
 *  |            		 |   |
 *  |            		 |   |
 *  |				 |   |
 *  |				 |   |
 *  |				 |   |
 *  +----------------------------+---+
 * </pre>
 */
static void test_grid_3x3_intrincate()
{
  JWidget grid = jgrid_new(3, false);
  JWidget w1 = jwidget_new(JI_WIDGET);
  JWidget w2 = jwidget_new(JI_WIDGET);
  JWidget w3 = jwidget_new(JI_WIDGET);
  JWidget w4 = jwidget_new(JI_WIDGET);
  JRect rect;
  int req_w, req_h;

  jwidget_set_min_size(w1, 10, 10);
  jwidget_set_min_size(w2, 10, 10);
  jwidget_set_min_size(w3, 10, 10);
  jwidget_set_min_size(w4, 10, 10);

  jgrid_add_child(grid, w1, 1, 1, 0);
  jgrid_add_child(grid, w2, 2, 1, JI_HORIZONTAL);
  jgrid_add_child(grid, w3, 2, 2, JI_HORIZONTAL | JI_VERTICAL);
  jgrid_add_child(grid, w4, 1, 2, JI_VERTICAL);

  /* test request size */
  grid->child_spacing = 2;
  jwidget_request_size(grid, &req_w, &req_h);
  ASSERT(req_w == 22 && req_h == 22);

  /* test layout */
  rect = jrect_new(0, 0, 100, 100);
  jwidget_set_rect(grid, rect);
  jrect_free(rect);

  ASSERT(w1->rc->x1 == 0  && w1->rc->y1 == 0);
  ASSERT(w2->rc->x1 == 12 && w2->rc->y1 == 0);
  ASSERT(w3->rc->x1 == 0  && w3->rc->y1 == 12);
  ASSERT(w4->rc->x1 == 90 && w4->rc->y1 == 12);

  ASSERT(jrect_w(w1->rc) == 10 && jrect_h(w1->rc) == 10);
  ASSERT(jrect_w(w2->rc) == 88 && jrect_h(w2->rc) == 10);
  ASSERT(jrect_w(w3->rc) == 88 && jrect_h(w3->rc) == 88);
  ASSERT(jrect_w(w4->rc) == 10 && jrect_h(w4->rc) == 88);

  jwidget_free(grid);
}

int main(int argc, char *argv[])
{
  JWidget manager = test_init_gui();

  test_grid_2x1();
  test_grid_2x1_expand2nd();
  test_grid_2x1_samewidth();
  test_grid_3x3_intrincate();

  return test_exit_gui(manager);
}

END_OF_MAIN();
