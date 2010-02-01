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

#ifndef EFFECT_IMAGES_REF_H_INCLUDED
#define EFFECT_IMAGES_REF_H_INCLUDED

class Sprite;
class Image;
class Layer;
class Cel;

struct ImageRef
{
  Image* image;
  Layer* layer;
  Cel* cel;
  ImageRef* next;
};

ImageRef* images_ref_get_from_sprite(Sprite* sprite, int target, bool write);
void images_ref_free(ImageRef* image_ref);

#endif

