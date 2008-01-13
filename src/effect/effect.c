/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <stdlib.h>
#include <string.h>

#include "jinete/jmanager.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

#include "console/console.h"
#include "core/cfg.h"
#include "effect/colcurve.h"
#include "effect/convmatr.h"
#include "effect/effect.h"
#include "effect/invrtcol.h"
#include "effect/median.h"
#include "effect/replcol.h"
#include "modules/editors.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "widgets/editor.h"

#endif

typedef struct EffectData
{
  const char *name;
  void (*apply_4)(Effect *effect);
  void (*apply_2)(Effect *effect);
  void (*apply_1)(Effect *effect);
} EffectData;

#define FXDATA(name) \
  { #name, apply_##name##4, apply_##name##2, apply_##name##1 }

static EffectData effects_data[] = {
  FXDATA(color_curve),
  FXDATA(convolution_matrix),
  FXDATA(invert_color),
  FXDATA(median),
  FXDATA(replace_color),
  { NULL, NULL, NULL, NULL }
};

static EffectData *get_effect_data(const char *name);
static int effect_init(Effect *effect, Image *image, int offset_x, int offset_y);
static int effect_update_mask(Effect *effect, Mask *mask, Image *image);

int init_module_effect(void)
{
  init_convolution_matrix();
  return 0;
}

void exit_module_effect(void)
{
  exit_convolution_matrix();
}

Effect *effect_new(Sprite *sprite, const char *name)
{
  int offset_x, offset_y;
  EffectData *effect_data;
  Effect *effect;
  Image *image;
  void *apply;

  effect_data = get_effect_data(name);
  if (!effect_data)
    return NULL;

  apply = NULL;
  switch (sprite->imgtype) {
    case IMAGE_RGB:       apply = effect_data->apply_4; break;
    case IMAGE_GRAYSCALE: apply = effect_data->apply_2; break;
    case IMAGE_INDEXED:   apply = effect_data->apply_1; break;
  }
  if (!apply)
    return NULL;

  effect = jnew(Effect, 1);
  if (!effect)
    return NULL;

  effect->sprite = sprite;
  effect->src = NULL;
  effect->dst = NULL;
  effect->row = 0;
  effect->offset_x = 0;
  effect->offset_y = 0;
  effect->mask = NULL;
  effect->preview_mask = NULL;
  effect->mask_address = NULL;
  effect->apply = apply;

  effect_load_target(effect);

  image = GetImage2(sprite, &offset_x, &offset_y, NULL);
  if (image) {
    if (!effect_init(effect, image, offset_x, offset_y)) {
      effect_free(effect);
      return NULL;
    }
  }

  return effect;
}

void effect_free(Effect *effect)
{
  if (effect->preview_mask)
    mask_free(effect->preview_mask);

  if (effect->dst)
    image_free(effect->dst);

  jfree(effect);
}

void effect_load_target(Effect *effect)
{
  effect->target.r = get_config_bool("Target", "Red", TRUE);
  effect->target.g = get_config_bool("Target", "Green", TRUE);
  effect->target.b = get_config_bool("Target", "Blue", TRUE);
  effect->target.k = get_config_bool("Target", "Gray", TRUE);
  effect->target.a = get_config_bool("Target", "Alpha", TRUE);
  effect->target.index = get_config_bool("Target", "Index", FALSE);
}

void effect_set_target(Effect *effect, bool r, bool g, bool b, bool k, bool a, bool index)
{
  effect->target.r = r;
  effect->target.g = g;
  effect->target.b = b;
  effect->target.k = k;
  effect->target.a = a;
  effect->target.index = index;
}

void effect_set_target_rgb(Effect *effect, bool r, bool g, bool b, bool a)
{
  effect_set_target(effect, r, g, b, FALSE, a, FALSE);
}

void effect_set_target_grayscale(Effect *effect, bool k, bool a)
{
  effect_set_target(effect, FALSE, FALSE, FALSE, k, a, FALSE);
}

void effect_set_target_indexed(Effect *effect, bool r, bool g, bool b, bool index)
{
  effect_set_target(effect, r, g, b, FALSE, FALSE, index);
}

void effect_begin(Effect *effect)
{
  effect->row = 0;
  effect->mask = effect->sprite->mask;

  effect_update_mask(effect, effect->mask, effect->src);
}

void effect_begin_for_preview(Effect *effect)
{
  if (effect->preview_mask) {
    mask_free(effect->preview_mask);
    effect->preview_mask = NULL;
  }

  if ((effect->sprite->mask) && (effect->sprite->mask->bitmap))
    effect->preview_mask = mask_new_copy(effect->sprite->mask);
  else {
    effect->preview_mask = mask_new();
    mask_replace(effect->preview_mask,
		 effect->offset_x, effect->offset_y,
		 effect->src->w, effect->src->h);
  }

  effect->row = 0;
  effect->mask = effect->preview_mask;

  {
    JWidget editor = current_editor;
    JRect vp = jview_get_viewport_position(jwidget_get_view(editor));
    int x1, y1, x2, y2;
    int x, y, w, h;

    screen_to_editor(editor, vp->x1, vp->y1, &x1, &y1);
    screen_to_editor(editor, vp->x2-1, vp->y2-1, &x2, &y2);

    jrect_free(vp);

    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= effect->sprite->w) x2 = effect->sprite->w-1;
    if (y2 >= effect->sprite->h) y2 = effect->sprite->h-1;

    x = x1;
    y = y1;
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;

    if ((w < 1) || (h < 1)) {
      mask_free(effect->preview_mask);
      effect->preview_mask = NULL;
      effect->row = -1;
      return;
    }

    mask_intersect(effect->preview_mask, x, y, w, h);
  }

  if (!effect_update_mask(effect, effect->mask, effect->src)) {
    mask_free(effect->preview_mask);
    effect->preview_mask = NULL;
    effect->row = -1;
    return;
  }
}

bool effect_apply_step(Effect *effect)
{
  if ((effect->row >= 0) && (effect->row < effect->h)) {
    if ((effect->mask) && (effect->mask->bitmap)) {
      effect->d = div(effect->x-effect->mask->x+effect->offset_x, 8);
      effect->mask_address =
	((ase_uint8 **)effect->mask->bitmap->line)
	[effect->row+effect->y-effect->mask->y+effect->offset_y]+effect->d.quot;
    }
    else
      effect->mask_address = NULL;

    (*effect->apply)(effect);
    effect->row++;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

void effect_apply(Effect *effect)
{
  add_progress(effect->h);

  effect_begin(effect);
  while (effect_apply_step(effect))
    do_progress(effect->row);

  /* undo stuff */
  if (undo_is_enabled(effect->sprite->undo))
    undo_image(effect->sprite->undo, effect->src,
	       effect->x, effect->y, effect->w, effect->h);

  /* copy "dst" to "src" */
  image_copy(effect->src, effect->dst, 0, 0);

  del_progress();
}

void effect_flush(Effect *effect)
{
  if (effect->row >= 0) {
    JRegion reg1, reg2;
    struct jrect rect;
    JWidget editor;

    editor = current_editor;
    reg1 = jregion_new(NULL, 0);

    editor_to_screen(editor,
		     effect->x+effect->offset_x,
		     effect->y+effect->offset_y+effect->row-1,
		     &rect.x1, &rect.y1);
    rect.x2 = rect.x1 + (effect->w << editor_data(editor)->zoom);
    rect.y2 = rect.y1 + (1 << editor_data(editor)->zoom);

    reg2 = jregion_new(&rect, 1);
    jregion_union(reg1, reg1, reg2);
    jregion_free(reg2);

    reg2 = jwidget_get_drawable_region(editor, JI_GDR_CUTTOPWINDOWS);
    jregion_intersect(reg1, reg1, reg2);
    jregion_free(reg2);

    jwidget_redraw_region(editor, reg1);
    jregion_free(reg1);
  }
}

void effect_apply_to_image(Effect *effect, Image *image, int x, int y)
{
  if (effect_init(effect, image, x, y))
    effect_apply(effect);
}

void effect_apply_to_target(Effect *effect)
{
  int target = get_config_int("Target", "Images", 0);
  int n, n2, images = 0;
  Stock *stock;
  int *x, *y;

  stock = sprite_get_images(effect->sprite, target, TRUE, &x, &y);
  if (!stock)
    return;

  for (n=0; n<stock->nimage; n++)
    if (stock->image[n])
      images++;

  if (images > 0) {
    if (images > 1) {
      /* open undo */
      if (undo_is_enabled (effect->sprite->undo))
	undo_open(effect->sprite->undo);
    }

    add_progress(images);
    for (n=n2=0; n<stock->nimage; n++) {
      if (!stock->image[n])
	continue;

      do_progress(n2++);
      effect_apply_to_image(effect, stock->image[n], x[n], y[n]);
    }
    del_progress();

    if (images > 1) {
      /* close  */
      if (undo_is_enabled (effect->sprite->undo))
	undo_close(effect->sprite->undo);
    }

    jfree(x);
    jfree(y);
  }

  stock_free(stock);
}

static EffectData *get_effect_data(const char *name)
{
  int c;

  for (c=0; effects_data[c].name; c++) {
    if (strcmp (effects_data[c].name, name) == 0)
      return effects_data+c;
  }

  return NULL;
}

static int effect_init(Effect *effect, Image *image, int offset_x, int offset_y)
{
  effect->offset_x = offset_x;
  effect->offset_y = offset_y;

  if (!effect_update_mask (effect, effect->sprite->mask, image))
    return FALSE;

  if (effect->preview_mask) {
    mask_free(effect->preview_mask);
    effect->preview_mask = NULL;
  }

  if (effect->dst) {
    image_free(effect->dst);
    effect->dst = NULL;
  }

  effect->src = image;
  effect->dst = image_crop(image, 0, 0, image->w, image->h);
  effect->row = -1;
  effect->mask = NULL;
  effect->preview_mask = NULL;
  effect->mask_address = NULL;

  return TRUE;
}

static int effect_update_mask(Effect *effect, Mask *mask, Image *image)
{
  int x, y, w, h;

  if ((mask) && (mask->bitmap)) {
    x = mask->x - effect->offset_x;
    y = mask->y - effect->offset_y;
    w = mask->w;
    h = mask->h;

    if (x < 0) {
      w += x;
      x = 0;
    }

    if (y < 0) {
      h += y;
      y = 0;
    }

    if (x+w-1 >= image->w-1)
      w = image->w-x;

    if (y+h-1 >= image->h-1)
      h = image->h-y;
  }
  else {
    x = 0;
    y = 0;
    w = image->w;
    h = image->h;
  }

  if ((w < 1) || (h < 1)) {
    effect->x = 0;
    effect->y = 0;
    effect->w = 0;
    effect->h = 0;
    return FALSE;
  }
  else {
    effect->x = x;
    effect->y = y;
    effect->w = w;
    effect->h = h;
    return TRUE;
  }
}
