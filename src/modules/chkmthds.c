/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include "jinete/widget.h"

#include "core/app.h"
#include "dialogs/filmedit.h"
#include "modules/chkmthds.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/hash.h"
#include "util/misc.h"
#include "util/recscr.h"

#endif

static HashTable *table;

static bool method_always (JWidget menuitem);
static bool method_has_sprite (JWidget menuitem);
static bool method_has_layer (JWidget menuitem);
static bool method_has_layerimage (JWidget menuitem);
static bool method_has_image (JWidget menuitem);
static bool method_can_undo (JWidget menuitem);
static bool method_can_redo (JWidget menuitem);
static bool method_has_path (JWidget menuitem);
static bool method_has_mask (JWidget menuitem);
static bool method_has_imagemask (JWidget menuitem);
static bool method_has_clipboard (JWidget menuitem);
static bool method_menu_bar (JWidget menuitem);
static bool method_status_bar (JWidget menuitem);
static bool method_color_bar (JWidget menuitem);
static bool method_tool_bar (JWidget menuitem);
static bool method_tool_marker (JWidget menuitem);
static bool method_tool_dots (JWidget menuitem);
static bool method_tool_pencil (JWidget menuitem);
static bool method_tool_brush (JWidget menuitem);
static bool method_tool_floodfill (JWidget menuitem);
static bool method_tool_spray (JWidget menuitem);
static bool method_tool_line (JWidget menuitem);
static bool method_tool_rectangle (JWidget menuitem);
static bool method_tool_ellipse (JWidget menuitem);
static bool method_is_rec (JWidget menuitem);
static bool method_is_movingframe (JWidget menuitem);

int init_module_check_methods (void)
{
  table = hash_new (16);

  hash_insert (table, "always", method_always);
  hash_insert (table, "has_sprite", method_has_sprite);
  hash_insert (table, "has_layer", method_has_layer);
  hash_insert (table, "has_layerimage", method_has_layerimage);
  hash_insert (table, "has_image", method_has_image);
  hash_insert (table, "can_undo", method_can_undo);
  hash_insert (table, "can_redo", method_can_redo);
  hash_insert (table, "has_path", method_has_path);
  hash_insert (table, "has_mask", method_has_mask);
  hash_insert (table, "has_imagemask", method_has_imagemask);
  hash_insert (table, "has_clipboard", method_has_clipboard);

  hash_insert (table, "menu_bar", method_menu_bar);
  hash_insert (table, "status_bar", method_status_bar);
  hash_insert (table, "color_bar", method_color_bar);
  hash_insert (table, "tool_bar", method_tool_bar);

  hash_insert (table, "tool_marker", method_tool_marker);
  hash_insert (table, "tool_dots", method_tool_dots);
  hash_insert (table, "tool_pencil", method_tool_pencil);
  hash_insert (table, "tool_brush", method_tool_brush);
  hash_insert (table, "tool_floodfill", method_tool_floodfill);
  hash_insert (table, "tool_spray", method_tool_spray);
  hash_insert (table, "tool_line", method_tool_line);
  hash_insert (table, "tool_rectangle", method_tool_rectangle);
  hash_insert (table, "tool_ellipse", method_tool_ellipse);

  hash_insert (table, "is_rec", method_is_rec);
  hash_insert (table, "is_movingframe", method_is_movingframe);

  return 0;
}

void exit_module_check_methods(void)
{
  hash_free(table, NULL);
}

CheckMethod get_check_method(const char *name)
{
  return (CheckMethod)hash_lookup(table, name);
}

static bool method_always(JWidget menuitem)
{
  return TRUE;
}

static bool method_has_sprite(JWidget menuitem)
{
  return (current_sprite) ? TRUE: FALSE;
}

static bool method_has_layer(JWidget menuitem)
{
  return (current_sprite &&
	  current_sprite->layer) ? TRUE: FALSE;
}

static bool method_has_layerimage(JWidget menuitem)
{
  return (current_sprite &&
	  current_sprite->layer &&
	  layer_is_image(current_sprite->layer));
}

static bool method_has_image(JWidget menuitem)
{
  if ((!current_sprite) ||
      (!current_sprite->layer) ||
      (!current_sprite->layer->readable) ||
      (!current_sprite->layer->writeable))
    return FALSE;
  else
    return GetImage () ? TRUE: FALSE;
}

static bool method_can_undo(JWidget menuitem)
{
  return current_sprite && undo_can_undo(current_sprite->undo);
}

static bool method_can_redo(JWidget menuitem)
{
  return current_sprite && undo_can_redo(current_sprite->undo);
}

static bool method_has_path(JWidget menuitem)
{
  if (!current_sprite)
    return FALSE;
  else
    return (current_sprite->path) ? TRUE: FALSE;
}

static bool method_has_mask(JWidget menuitem)
{
  if (!current_sprite)
    return FALSE;
  else
    return (current_sprite->mask &&
	    current_sprite->mask->bitmap) ? TRUE: FALSE;
}

static bool method_has_imagemask(JWidget menuitem)
{
  if ((!current_sprite) ||
      (!current_sprite->layer) ||
      (!current_sprite->layer->readable) ||
      (!current_sprite->layer->writeable) ||
      (!current_sprite->mask) ||
      (!current_sprite->mask->bitmap))
    return FALSE;
  else
    return GetImage() ? TRUE: FALSE;
}

static bool method_has_clipboard(JWidget menuitem)
{
  Sprite *sprite = current_sprite;
  Sprite *clipboard = get_clipboard_sprite();

  return (sprite &&
	  clipboard &&
	  (clipboard != sprite));
}

static bool method_menu_bar(JWidget menuitem)
{
  if (jwidget_is_visible(app_get_menu_bar()))
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_status_bar(JWidget menuitem)
{
  if (jwidget_is_visible(app_get_status_bar()))
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_color_bar(JWidget menuitem)
{
  if (jwidget_is_visible(app_get_color_bar()))
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_tool_bar(JWidget menuitem)
{
  if (jwidget_is_visible(app_get_tool_bar()))
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_tool_marker(JWidget menuitem)
{
  if (current_tool == &ase_tool_marker)
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_tool_dots(JWidget menuitem)
{
  if (current_tool == &ase_tool_dots)
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_tool_pencil(JWidget menuitem)
{
  if (current_tool == &ase_tool_pencil)
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_tool_brush(JWidget menuitem)
{
  if (current_tool == &ase_tool_brush)
    jwidget_select(menuitem);
  else
    jwidget_deselect(menuitem);

  return TRUE;
}

static bool method_tool_floodfill (JWidget menuitem)
{
  if (current_tool == &ase_tool_floodfill)
    jwidget_select (menuitem);
  else
    jwidget_deselect (menuitem);

  return TRUE;
}

static bool method_tool_spray (JWidget menuitem)
{
  if (current_tool == &ase_tool_spray)
    jwidget_select (menuitem);
  else
    jwidget_deselect (menuitem);

  return TRUE;
}

static bool method_tool_line (JWidget menuitem)
{
  if (current_tool == &ase_tool_line)
    jwidget_select (menuitem);
  else
    jwidget_deselect (menuitem);

  return TRUE;
}

static bool method_tool_rectangle (JWidget menuitem)
{
  if (current_tool == &ase_tool_rectangle)
    jwidget_select (menuitem);
  else
    jwidget_deselect (menuitem);

  return TRUE;
}

static bool method_tool_ellipse (JWidget menuitem)
{
  if (current_tool == &ase_tool_ellipse)
    jwidget_select (menuitem);
  else
    jwidget_deselect (menuitem);

  return TRUE;
}

static bool method_is_rec (JWidget menuitem)
{
  if (is_rec_screen ())
    jwidget_select (menuitem);
  else
    jwidget_deselect (menuitem);

  return TRUE;
}

static bool method_is_movingframe (JWidget menuitem)
{
  return is_movingframe ();
}
