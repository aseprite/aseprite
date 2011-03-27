// ASEPRITE Undo Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "undo/undo_history.h"

#include "raster/palette.h"
#include "undo/undoers_stack.h"
#include "undoers/add_cel.h"
#include "undoers/add_image.h"
#include "undoers/add_layer.h"
#include "undoers/add_palette.h"
#include "undoers/close_group.h"
#include "undoers/dirty_area.h"
#include "undoers/flip_image.h"
#include "undoers/image_area.h"
#include "undoers/move_layer.h"
#include "undoers/open_group.h"
#include "undoers/remap_palette.h"
#include "undoers/remove_cel.h"
#include "undoers/remove_image.h"
#include "undoers/remove_layer.h"
#include "undoers/remove_palette.h"
#include "undoers/replace_image.h"
#include "undoers/set_current_frame.h"
#include "undoers/set_current_layer.h"
#include "undoers/set_frame_duration.h"
#include "undoers/set_layer_name.h"
#include "undoers/set_mask.h"
#include "undoers/set_palette_colors.h"
#include "undoers/set_sprite_imgtype.h"
#include "undoers/set_sprite_size.h"
#include "undoers/set_total_frames.h"

using namespace undo;

void UndoHistory::undo_open()
{
  pushUndoer(new undoers::OpenGroup());
}

void UndoHistory::undo_close()
{
  pushUndoer(new undoers::CloseGroup());
}

void UndoHistory::undo_image(Image* image, int x, int y, int w, int h)
{
  pushUndoer(new undoers::ImageArea(getObjects(), image, x, y, w, h));
}

void UndoHistory::undo_flip(Image* image, int x1, int y1, int x2, int y2, bool horz)
{
  pushUndoer(new undoers::FlipImage(getObjects(), image, x1, y1, x2-x1+1, y2-y1+1,
				    (horz ? undoers::FlipImage::FlipHorizontal:
					    undoers::FlipImage::FlipVertical)));
}

void UndoHistory::undo_dirty(Image* image, Dirty* dirty)
{
  pushUndoer(new undoers::DirtyArea(getObjects(), image, dirty));
}

void UndoHistory::undo_add_image(Stock* stock, int imageIndex)
{
  pushUndoer(new undoers::AddImage(getObjects(), stock, imageIndex));
}

void UndoHistory::undo_remove_image(Stock *stock, int imageIndex)
{
  pushUndoer(new undoers::RemoveImage(getObjects(), stock, imageIndex));
}

void UndoHistory::undo_replace_image(Stock *stock, int imageIndex)
{
  pushUndoer(new undoers::ReplaceImage(getObjects(), stock, imageIndex));
}

void UndoHistory::undo_add_cel(Layer* layer, Cel* cel)
{
  pushUndoer(new undoers::AddCel(getObjects(), layer, cel));
}

void UndoHistory::undo_remove_cel(Layer* layer, Cel* cel)
{
  pushUndoer(new undoers::RemoveCel(getObjects(), layer, cel));
}

void UndoHistory::undo_set_layer_name(Layer* layer)
{
  pushUndoer(new undoers::SetLayerName(getObjects(), layer));
}

void UndoHistory::undo_add_layer(Layer* folder, Layer* layer)
{
  pushUndoer(new undoers::AddLayer(getObjects(), folder, layer));
}

void UndoHistory::undo_remove_layer(Layer* layer)
{
  pushUndoer(new undoers::RemoveLayer(getObjects(), layer));
}

void UndoHistory::undo_move_layer(Layer* layer)
{
  pushUndoer(new undoers::MoveLayer(getObjects(), layer));
}

void UndoHistory::undo_set_layer(Sprite* sprite)
{
  pushUndoer(new undoers::SetCurrentLayer(getObjects(), sprite));
}

void UndoHistory::undo_add_palette(Sprite* sprite, Palette* palette)
{
  pushUndoer(new undoers::AddPalette(getObjects(), sprite, palette->getFrame()));
}

void UndoHistory::undo_remove_palette(Sprite* sprite, Palette* palette)
{
  pushUndoer(new undoers::RemovePalette(getObjects(), sprite, palette->getFrame()));
}

void UndoHistory::undo_set_palette_colors(Sprite* sprite, Palette* palette, int from, int to)
{
  pushUndoer(new undoers::SetPaletteColors(getObjects(), sprite, palette, from, to));
}

void UndoHistory::undo_remap_palette(Sprite* sprite, int frameFrom, int frameTo, const std::vector<uint8_t>& mapping)
{
  pushUndoer(new undoers::RemapPalette(getObjects(), sprite, frameFrom, frameTo, mapping));
}

void UndoHistory::undo_set_mask(Document* document)
{
  pushUndoer(new undoers::SetMask(getObjects(), document));
}

void UndoHistory::undo_set_imgtype(Sprite* sprite)
{
  pushUndoer(new undoers::SetSpriteImgType(getObjects(), sprite));
}

void UndoHistory::undo_set_size(Sprite* sprite)
{
  pushUndoer(new undoers::SetSpriteSize(getObjects(), sprite));
}

void UndoHistory::undo_set_frame(Sprite* sprite)
{
  pushUndoer(new undoers::SetCurrentFrame(getObjects(), sprite));
}

void UndoHistory::undo_set_frames(Sprite* sprite)
{
  pushUndoer(new undoers::SetTotalFrames(getObjects(), sprite));
}

void UndoHistory::undo_set_frlen(Sprite* sprite, int frame)
{
  pushUndoer(new undoers::SetFrameDuration(getObjects(), sprite, frame));
}
