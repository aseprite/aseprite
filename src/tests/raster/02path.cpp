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

#include "config.h"

#include <allegro.h>
#include <math.h>

#include "jinete/jinete.h"

#include "raster/algo.h"
#include "raster/image.h"
#include "raster/path.h"

typedef struct BEZIER_NODE
{
  char tangent;
  float prev_x, x, next_x;
  float prev_y, y, next_y;
  struct BEZIER_NODE *prev;
  struct BEZIER_NODE *next;
} BEZIER_NODE;

typedef struct BEZIER_PATH
{
  BEZIER_NODE *first, *last;
} BEZIER_PATH;

BEZIER_PATH *create_bezier_path()
{
  BEZIER_PATH *path;

  path = jnew (BEZIER_PATH, 1);
  if (path)
    path->first = path->last = NULL;

  return path;
}

BEZIER_NODE *create_bezier_node(int tangent, float prev_x, float prev_y, float x, float y, float next_x, float next_y)
{
  BEZIER_NODE *node;

  node = jnew (BEZIER_NODE, 1);
  if (node) {
    node->tangent = tangent;
    node->prev_x = prev_x; node->x = x; node->next_x = next_x;
    node->prev_y = prev_y; node->y = y; node->next_y = next_y;
    node->prev = node->next = NULL;
  }

  return node;
}

void add_bezier_node(BEZIER_PATH *path, BEZIER_NODE *node)
{
  if (!path->first) {
    node->prev = NULL;
    path->first = node;
  }
  else {
    path->last->next = node;
    node->prev = path->last;
  }
  node->next = NULL;
  path->last = node;
}

#define DRAW_BEZIER_LINE             1
#define DRAW_BEZIER_NODES            2
#define DRAW_BEZIER_CONTROL_POINTS   4
#define DRAW_BEZIER_CONTROL_LINES    8

void draw_bezier_path(BITMAP *bmp, BEZIER_PATH *path, int flags, int color)
{
  BEZIER_NODE *node;
  int points[8];

  for (node=path->first; node; node=node->next) {
    /* draw the bezier spline */
    if ((flags & DRAW_BEZIER_LINE) && (node->next)) {
      points[0] = node->x;
      points[1] = node->y;
      points[2] = node->next_x;
      points[3] = node->next_y;
      points[4] = node->next->prev_x;
      points[5] = node->next->prev_y;
      points[6] = node->next->x;
      points[7] = node->next->y;
      spline(bmp, points, color);
    }

    /* draw the control points */
    if (node->prev) {
      if (flags & DRAW_BEZIER_CONTROL_LINES)
        line(bmp, node->prev_x, node->prev_y, node->x, node->y, color);

      if (flags & DRAW_BEZIER_CONTROL_POINTS)
        rectfill(bmp, node->prev_x-1, node->prev_y-1, node->prev_x, node->prev_y, color);
    }

    if (node->next) {
      if (flags & DRAW_BEZIER_CONTROL_LINES)
        line(bmp, node->x, node->y, node->next_x, node->next_y, color);

      if (flags & DRAW_BEZIER_CONTROL_POINTS)
        rectfill(bmp, node->next_x-1, node->next_y-1, node->next_x, node->next_y, color);
    }

    /* draw the node */
    if (flags & DRAW_BEZIER_NODES)
      rectfill(bmp, node->x-1, node->y-1, node->x+1, node->y+1, color);

    if (node == path->last)
      break;
  }
}

typedef struct Point { int x, y; } Point;

static void add_shape_seg(int x1, int y1, int x2, int y2, JList shape)
{
  Point *point = jnew(Point, 1);
  point->x = x2;
  point->y = y2;
  jlist_append(shape, point);
}

void draw_filled_bezier_path(BITMAP *bmp, BEZIER_PATH *path, int color)
{
  BEZIER_NODE *node;
  JList shape = jlist_new();
  JLink link;
  int c, vertices;
  int *points;

  for (node=path->first; node; node=node->next) {
    /* draw the bezier spline */
    if (node->next) {
      algo_spline(node->x,
		  node->y,
		  node->next_x,
		  node->next_y,
		  node->next->prev_x,
		  node->next->prev_y,
		  node->next->x,
		  node->next->y,
		  (void *)shape, (AlgoLine)add_shape_seg);
    }

    if (node == path->last)
      break;
  }

  vertices = jlist_length(shape);
  if (vertices > 0) {
    points = (int*)jmalloc(sizeof(int) * vertices * 2);

    c = 0;
    JI_LIST_FOR_EACH(shape, link) {
      points[c++] = ((Point *)link->data)->x;
      points[c++] = ((Point *)link->data)->y;
    }
    polygon(bmp, vertices, points, color);
    jfree(points);
  }

  JI_LIST_FOR_EACH(shape, link);
    jfree(link->data);

  jlist_free(shape);
}

void draw_art_filled_bezier_path(BITMAP *bmp, BEZIER_PATH *path)
{
  Image *image = image_new (IMAGE_RGB, SCREEN_W, SCREEN_H);
  Path *art_path = path_new (NULL);
  BEZIER_NODE *node;

  node = path->first;
  if (node) {
    path_moveto (art_path, node->x, node->y);
    for (; node; node=node->next) {
      if (node->next) {
	path_curveto (art_path,
		      node->next_x,
		      node->next_y,
		      node->next->prev_x,
		      node->next->prev_y,
		      node->next->x,
		      node->next->y);
      }
      if (node == path->last)
	break;
    }
    path_close (art_path);
  }

  image_clear (image, 0);
  path_fill (art_path, image, _rgba (255, 255, 255, 255));
  image_to_allegro (image, bmp, 0, 0);
  image_free (image);
  path_free (art_path);
}

#define GOT_BEZIER_POINT_NULL 0
#define GOT_BEZIER_POINT_PREV 1
#define GOT_BEZIER_POINT_NODE 2
#define GOT_BEZIER_POINT_NEXT 3

int get_bezier_point (BEZIER_PATH *path, float x, float y, BEZIER_NODE **the_node)
{
  BEZIER_NODE *node;

  for (node=path->first; node; node=node->next) {
    if (node->prev) {
      if ((x >= node->prev_x-1) &&
          (y >= node->prev_y-1) &&
          (x <= node->prev_x)   &&
          (y <= node->prev_y)) {
        *the_node = node;
        return GOT_BEZIER_POINT_PREV;
      }
    }

    if (node->next) {
      if ((x >= node->next_x-1) &&
          (y >= node->next_y-1) &&
          (x <= node->next_x)   &&
          (y <= node->next_y)) {
        *the_node = node;
        return GOT_BEZIER_POINT_NEXT;
      }
    }

    if ((x >= node->x-1) && (y >= node->y-1) &&
        (x <= node->x+1) && (y <= node->y+1)) {
      *the_node = node;
      return GOT_BEZIER_POINT_NODE;
    }

    if (node == path->last)
      break;
  }

  return GOT_BEZIER_POINT_NULL;
}

int main()
{
  BITMAP *virt, *background;
  BEZIER_PATH *path;
  int x[4], y[4];
  int level;
  int old_x;
  int old_y;

  allegro_init();
  install_timer();
  install_keyboard();
  install_mouse();

  set_gfx_mode(GFX_AUTODETECT, 320, 240, 0, 0);

  rgb_map = jnew (RGB_MAP, 1);
  create_rgb_table(rgb_map, _current_palette, NULL);

  path = create_bezier_path();

  virt = create_bitmap(SCREEN_W, SCREEN_H);
  background = create_bitmap(SCREEN_W, SCREEN_H);
  clear(background);

  level = 0;

  do {
    /* erase */
    if (key[KEY_E]) {
      while (key[KEY_E]);
      path = create_bezier_path();
    }

    /* close */
    if (key[KEY_C]) {
      while (key[KEY_C]);
      path->first->prev = path->last;
      path->last->next = path->first;
    }

    poll_mouse();
    if ((mouse_x != old_x) || (mouse_y != old_y)) {
      poll_mouse();
      switch (level) {
        case 0:
          x[0] = mouse_x;
          y[0] = mouse_y;
          level = 1;
          break;
        case 1:
          x[1] = mouse_x;
          y[1] = mouse_y;
          level = 2;
          break;
        case 2:
          x[2] = mouse_x;
          y[2] = mouse_y;
          level = 3;
          break;
        case 3:
          x[3] = mouse_x;
          y[3] = mouse_y;
          level = 4;
          break;
        case 4:
          if (mouse_b & 1) {
            BEZIER_NODE *node;
            node = create_bezier_node(true,
              x[1], y[1],
              x[3], y[3],
              x[3] + x[3] - x[2], y[3] + y[3] - y[2]);
            add_bezier_node(path, node);
          }
          x[0] = x[3];
          y[0] = y[3];
          x[1] = x[3] + x[3] - x[2];
          y[1] = y[3] + y[3] - y[2];
          level = 2;
          break;
      }
    }
    old_x = mouse_x;
    old_y = mouse_y;

    if (mouse_b & 2) {
      BEZIER_NODE *node;
      int ret;

      ret = get_bezier_point(path, mouse_x, mouse_y, &node);
      if (ret) {
	switch (ret) {
	  case GOT_BEZIER_POINT_NODE:
	    position_mouse (node->x, node->y);
	    break;
	  case GOT_BEZIER_POINT_PREV:
	    position_mouse (node->prev_x, node->prev_y);
	    break;
	  case GOT_BEZIER_POINT_NEXT:
	    position_mouse (node->next_x, node->next_y);
	    break;
	}

	while (mouse_b) {
          poll_mouse();
          if ((mouse_x != old_x) || (mouse_y != old_y)) {
            poll_mouse();

            switch (ret) {

              case GOT_BEZIER_POINT_NODE:
                {
                  node->x += mouse_x - old_x;
                  node->y += mouse_y - old_y;
                  node->prev_x += mouse_x - old_x;
                  node->prev_y += mouse_y - old_y;
                  node->next_x += mouse_x - old_x;
                  node->next_y += mouse_y - old_y;
                }
                break;

              case GOT_BEZIER_POINT_PREV:
                node->prev_x = mouse_x;
                node->prev_y = mouse_y;
                if (node->tangent) {
                  node->next_x = node->x + node->x - node->prev_x;
                  node->next_y = node->y + node->y - node->prev_y;
                }
                break;

              case GOT_BEZIER_POINT_NEXT:
                node->next_x = mouse_x;
                node->next_y = mouse_y;
                if (node->tangent) {
                  node->prev_x = node->x + node->x - node->next_x;
                  node->prev_y = node->y + node->y - node->next_y;
                }
                break;
            }

            blit(background, virt, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

            draw_bezier_path(virt, path, DRAW_BEZIER_CONTROL_LINES, makecol(32, 32, 32));
            draw_bezier_path(virt, path, DRAW_BEZIER_LINE, makecol(255, 0, 0));
            draw_bezier_path(virt, path, DRAW_BEZIER_CONTROL_POINTS, makecol(0, 128, 255));
            draw_bezier_path(virt, path, DRAW_BEZIER_NODES, makecol(255, 255, 255));

            blit(virt, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
          }
          old_x = mouse_x;
          old_y = mouse_y;

          if (key[KEY_S])
            node->tangent = false;

          if (key[KEY_T])
            node->tangent = true;
        }
      }
    }

    /* draw */
    blit(background, virt, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

    if (!key[KEY_LSHIFT])
      draw_bezier_path(virt, path, DRAW_BEZIER_CONTROL_LINES, makecol(32, 32, 32));

    draw_bezier_path(virt, path, DRAW_BEZIER_LINE, makecol(255, 0, 0));

    if (!key[KEY_LSHIFT]) {
      draw_bezier_path(virt, path, DRAW_BEZIER_CONTROL_POINTS, makecol(0, 128, 255));
      draw_bezier_path(virt, path, DRAW_BEZIER_NODES, makecol(255, 255, 255));
    }

    /* filled */
    if (key[KEY_F]) {
      clear (screen);
      draw_filled_bezier_path (screen, path, makecol(128, 128, 128));
      draw_bezier_path (screen, path, DRAW_BEZIER_LINE, makecol(0, 0, 255));
      while (key[KEY_F]);
    }

    /* art-filled */
    if (key[KEY_A]) {
      clear (screen);
      draw_art_filled_bezier_path (screen, path);
      while (key[KEY_A]);
    }

    hline(virt, mouse_x-4, mouse_y, mouse_x+4, makecol(255, 255, 255));
    vline(virt, mouse_x, mouse_y-4, mouse_y+4, makecol(255, 255, 255));
    blit(virt, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
  } while (!key[KEY_ESC]);

  destroy_bitmap(virt);
  destroy_bitmap(background);

  allegro_exit();
  return 0;
}

END_OF_MAIN();

