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

#ifndef UTIL_RENDER_H_INCLUDED
#define UTIL_RENDER_H_INCLUDED

class Image;
class Layer;
class Sprite;

class RenderEngine
{
public:
  static void setPreviewImage(Layer* layer, Image* drawable);

  static Image* renderSprite(Sprite* sprite,
			     int source_x, int source_y,
			     int width, int height,
			     int frpos, int zoom);

private:
  static void renderLayer(Sprite* sprite, Layer* layer, Image* image,
			  int source_x, int source_y,
			  int frame, int zoom,
			  void (*zoomed_func)(Image*, Image*, int, int, int, int, int),
			  bool render_background,
			  bool render_transparent);
};

#endif
