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

#ifndef EFFECT_CONVMATR_H_INCLUDED
#define EFFECT_CONVMATR_H_INCLUDED

#include "jinete/jbase.h"
#include "tiled_mode.h"

struct Effect;

typedef struct ConvMatr      /* a convolution matrix */
{
  char *name;                /* name */
  int w, h;                  /* size of the matrix */
  int cx, cy;                /* centre of the filter */
  int *data;                 /* the matrix with the multiplication factors */
  int div;                   /* division factor */
  int bias;                  /* addition factor (for offset) */
  int default_target;        /* targets by default (look at
				TARGET_RED_CHANNEL, etc. constants) */
} ConvMatr;

ConvMatr *convmatr_new(int w, int h);
ConvMatr *convmatr_new_string(const char *format);
void convmatr_free(ConvMatr *convmatr);

void set_convmatr(ConvMatr *convmatr, TiledMode tiled);
ConvMatr *get_convmatr();
ConvMatr *get_convmatr_by_name(const char *name);

void reload_matrices_stock();
void clean_matrices_stock();
JList get_convmatr_stock();

void init_convolution_matrix();
void exit_convolution_matrix();

void apply_convolution_matrix4(struct Effect *effect);
void apply_convolution_matrix2(struct Effect *effect);
void apply_convolution_matrix1(struct Effect *effect);

#endif
