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

#ifndef UTIL_FUNCTIONS_H
#define UTIL_FUNCTIONS_H

class Sprite;
class Layer;
class Cel;

/*===================================================================*/
/* Sprite                                                            */
/*===================================================================*/

Sprite* NewSprite(int imgtype, int w, int h);
Sprite* LoadSprite(const char* filename);
void SaveSprite(const char* filename);

void SetSprite(Sprite* sprite); 

/*===================================================================*/
/* Layer                                                             */
/*===================================================================*/

Layer* NewLayerSet(Sprite* sprite);

char *GetUniqueLayerName(Sprite* sprite);

Layer* FlattenLayers(Sprite* sprite);

void BackgroundFromLayer(Sprite* sprite);
void LayerFromBackground(Sprite* sprite);

/* ======================================= */
/* Cel                                     */
/* ======================================= */

void RemoveCel(Layer* layer, Cel* cel);

#endif /* UTIL_FUNCTIONS_H */
