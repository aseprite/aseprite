/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "app/util/autocrop.h"

namespace app {

using namespace doc;

bool get_shrink_rect(int *x1, int *y1, int *x2, int *y2,
                     Image *image, int refpixel)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,              \
                    v_begin, v_op, v_final, v_add, U, V, var)   \
  do {                                                          \
    for (u = u_begin; u u_op u_final; u u_add) {                \
      for (v = v_begin; v v_op v_final; v v_add) {              \
        if (image->getPixel(U, V) != refpixel)                  \
          break;                                                \
      }                                                         \
      if (v == v_final)                                         \
        var;                                                    \
      else                                                      \
        break;                                                  \
    }                                                           \
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->width()-1;
  *y2 = image->height()-1;

  SHRINK_SIDE(0, <, image->width(), ++,
              0, <, image->height(), ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->height(), ++,
              0, <, image->width(), ++, v, u, (*y1)++);

  SHRINK_SIDE(image->width()-1, >, 0, --,
              0, <, image->height(), ++, u, v, (*x2)--);

  SHRINK_SIDE(image->height()-1, >, 0, --,
              0, <, image->width(), ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return false;
  else
    return true;

#undef SHRINK_SIDE
}

bool get_shrink_rect2(int *x1, int *y1, int *x2, int *y2,
                      Image *image, Image *refimage)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,              \
                    v_begin, v_op, v_final, v_add, U, V, var)   \
  do {                                                          \
    for (u = u_begin; u u_op u_final; u u_add) {                \
      for (v = v_begin; v v_op v_final; v v_add) {              \
        if (image->getPixel(U, V) != refimage->getPixel(U, V))  \
          break;                                                \
      }                                                         \
      if (v == v_final)                                         \
        var;                                                    \
      else                                                      \
        break;                                                  \
    }                                                           \
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->width()-1;
  *y2 = image->height()-1;

  SHRINK_SIDE(0, <, image->width(), ++,
              0, <, image->height(), ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->height(), ++,
              0, <, image->width(), ++, v, u, (*y1)++);

  SHRINK_SIDE(image->width()-1, >, 0, --,
              0, <, image->height(), ++, u, v, (*x2)--);

  SHRINK_SIDE(image->height()-1, >, 0, --,
              0, <, image->width(), ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return false;
  else
    return true;

#undef SHRINK_SIDE
}

} // namespace app
