/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#ifndef UTIL_MISC_H
#define UTIL_MISC_H

#include "core/color.h"
#include "widgets/editor.h"	/* for movement modes */

class Frame;
class Image;
class Layer;
class Sprite;
class Undo;

Image* GetImage(Sprite* sprite);
Image* GetImage2(Sprite* sprite, int *x, int *y, int *opacity);

void LoadPalette(const char* filename);

Layer* NewLayerFromMask(Sprite* src, Sprite* dst);

Image* GetLayerImage(Layer* layer, int *x, int *y, int frame);

int interactive_move_layer(int mode, bool use_undo, int (*callback)());

#endif /* UTIL_MISC_H */

