/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#ifndef EFFECT_CONVMATR_H
#define EFFECT_CONVMATR_H

#include "jinete/base.h"

struct Effect;

#define CONVMATR_R	1
#define CONVMATR_G	2
#define CONVMATR_B	4
#define CONVMATR_K	(CONVMATR_R | CONVMATR_G | CONVMATR_B)
#define CONVMATR_A	8

typedef struct ConvMatr      /* a convolution matrix */
{
  char *name;                /* name */
  int w, h;                  /* size of the matrix */
  int cx, cy;                /* centre of the filter */
  int *data;                 /* the matrix with the multiplication factors */
  int div;                   /* division factor */
  int bias;                  /* addition factor (for offset) */
  int default_target;        /* targets by default */
} ConvMatr;

ConvMatr *convmatr_new (int w, int h);
ConvMatr *convmatr_new_string (const char *format);
void convmatr_free (ConvMatr *convmatr);

void set_convmatr (ConvMatr *convmatr);
ConvMatr *get_convmatr (void);
ConvMatr *get_convmatr_by_name (const char *name);

void reload_matrices_stock (void);
void clean_matrices_stock (void);
JList get_convmatr_stock (void);

void init_convolution_matrix (void);
void exit_convolution_matrix (void);

void apply_convolution_matrix4 (struct Effect *effect);
void apply_convolution_matrix2 (struct Effect *effect);
void apply_convolution_matrix1 (struct Effect *effect);

#endif /* EFFECT_CONVMATR_H */
