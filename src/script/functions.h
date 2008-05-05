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

#ifndef SCRIPT_FUNCTIONS_H
#define SCRIPT_FUNCTIONS_H

struct Sprite;
struct Layer;
struct Cel;

/*===================================================================*/
/* Sprite                                                            */
/*===================================================================*/

struct Sprite *NewSprite(int imgtype, int w, int h);
struct Sprite *LoadSprite(const char *filename);
void SaveSprite(const char *filename);

void SetSprite(struct Sprite *sprite); 

void CropSprite(struct Sprite *sprite);

/*===================================================================*/
/* Layer                                                             */
/*===================================================================*/

struct Layer *NewLayer(struct Sprite *sprite);
struct Layer *NewLayerSet(struct Sprite *sprite);
void RemoveLayer(struct Sprite *sprite);

char *GetUniqueLayerName(struct Sprite *sprite);

struct Layer *FlattenLayers(struct Sprite *sprite);

void CropLayer(struct Layer *layer, int x, int y, int w, int h);

void BackgroundFromLayer(struct Sprite *sprite);
void LayerFromBackground(struct Sprite *sprite);

void MoveLayerAfter(struct Layer *layer, struct Layer *after_this);

/* ======================================= */
/* Frame                                   */
/* ======================================= */

void NewFrame(struct Sprite *sprite);
void RemoveFrame(struct Sprite *sprite, int frame);
void SetFrameLength(struct Sprite *sprite, int frame, int msecs);
void MoveFrameBefore(int frame, int before_frame);

/* ======================================= */
/* Cel                                     */
/* ======================================= */

void RemoveCel(struct Layer *layer, struct Cel *cel);

void CropCel(void);

#endif /* SCRIPT_FUNCTIONS_H */
