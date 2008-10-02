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

#ifndef RASTER_UNDOABLE_H
#define RASTER_UNDOABLE_H

class Layer;
class Sprite;

/**
 * Class with high-level set of routines to modify a sprite.
 *
 * In this class, all modifications in the sprite have an undo
 * counterpart (if the sprite's undo is enabled).
 */
class Undoable
{
  Sprite* sprite;
  bool committed;
  bool enabled_flag;

public:
  Undoable(Sprite* sprite, const char* label);
  virtual ~Undoable();

  inline Sprite* get_sprite() const { return sprite;  }
  inline bool is_enabled() const { return enabled_flag; }

  void commit();

  // for sprite
  void set_number_of_frames(int frames);
  void set_current_frame(int frame);
  void set_current_layer(Layer* layer);

  // for images in stock
  int add_image_in_stock(Image* image);
  void remove_image_from_stock(int image_index);

  // for layers
  Layer* new_layer();
  void move_layer_after(Layer *layer, Layer *after_this);

  // for frames
  void new_frame();
  void new_frame_for_layer(Layer* layer, int frame);
  void remove_frame(int frame);
  void remove_frame_of_layer(Layer* layer, int frame);
  void copy_previous_frame(Layer* layer, int frame);
  void set_frame_length(int frame, int msecs);
  void move_frame_before(int frame, int before_frame);
  void move_frame_before_layer(Layer* layer, int frame, int before_frame);

  // for cels
  void add_cel(Layer* layer, Cel* cel);
  void remove_cel(Layer* layer, Cel* cel);
  void set_cel_frame_position(Cel* cel, int frame);

};

#endif /* RASTER_UNDOABLE_H */
