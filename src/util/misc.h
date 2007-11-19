/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#include "widgets/editor.h"	/* for movement modes */

struct Frame;
struct Image;
struct Layer;
struct Sprite;
struct Undo;

struct Image *GetImage(void);
struct Image *GetImage2(struct Sprite *sprite, int *x, int *y, int *opacity);

void LoadPalette(const char *filename);

void ClearMask(void);
struct Layer *NewLayerFromMask(void);

struct Image *GetLayerImage(struct Layer *layer, int *x, int *y, int frame);

int interactive_move_layer(int mode, int use_undo, int (*callback) (void));

#endif /* UTIL_MISC_H */

