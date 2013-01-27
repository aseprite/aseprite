/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "app.h"
#include "console.h"
#include "document.h"
#include "document_wrappers.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/quantization.h"
#include "raster/rotate.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "settings/settings.h"
#include "skin/skin_parts.h"
#include "skin/skin_theme.h"
#include "ui/gui.h"
#include "ui_context.h"
#include "undo/undo_history.h"
#include "undo_transaction.h"
#include "undoers/add_image.h"
#include "undoers/image_area.h"
#include "util/clipboard.h"
#include "util/misc.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#if defined ALLEGRO_WINDOWS
  #include <winalleg.h>

  #include "util/clipboard_win32.h"
#endif

static void set_clipboard(Image* image, Palette* palette, bool set_system_clipboard);
static bool copy_from_document(const Document* document);

static bool first_time = true;

static Palette* clipboard_palette = NULL;
static Image* clipboard_image = NULL;
static int clipboard_x = 0;
static int clipboard_y = 0;

static void on_exit_delete_clipboard()
{
  delete clipboard_palette;
  delete clipboard_image;
}

static void set_clipboard(Image* image, Palette* palette, bool set_system_clipboard)
{
  if (first_time) {
    first_time = false;
    App::instance()->Exit.connect(&on_exit_delete_clipboard);
  }

  delete clipboard_palette;
  delete clipboard_image;

  clipboard_palette = palette;
  clipboard_image = image;

  // copy to the Windows clipboard
#ifdef ALLEGRO_WINDOWS
  if (set_system_clipboard)
    set_win32_clipboard_bitmap(image, palette);
#endif
}

static bool copy_from_document(const Document* document)
{
  ASSERT(document != NULL);
  ASSERT(document->isMaskVisible());

  Image* image = NewImageFromMask(document);
  if (!image)
    return false;

  clipboard_x = document->getMask()->getBounds().x;
  clipboard_y = document->getMask()->getBounds().y;

  const Palette* pal = document->getSprite()->getPalette(document->getSprite()->getCurrentFrame());
  set_clipboard(image, pal ? new Palette(*pal): NULL, true);
  return true;
}

bool util::clipboard::can_paste()
{
#ifdef ALLEGRO_WINDOWS
  if (win32_clipboard_contains_bitmap())
    return true;
#endif
  return clipboard_image != NULL;
}

void util::clipboard::cut(DocumentWriter& document)
{
  ASSERT(document != NULL);
  ASSERT(document->getSprite() != NULL);
  ASSERT(document->getSprite()->getCurrentLayer() != NULL);

  if (!copy_from_document(document)) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
  else {
    {
      Sprite* sprite = document->getSprite();

      UndoTransaction undoTransaction(document, "Cut");
      undoTransaction.clearMask(app_get_color_to_clear_layer(sprite->getCurrentLayer()));
      undoTransaction.deselectMask();
      undoTransaction.commit();
    }
    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

void util::clipboard::copy(const DocumentReader& document)
{
  ASSERT(document != NULL);

  if (!copy_from_document(document)) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
}

void util::clipboard::copy_image(Image* image, Palette* pal, const gfx::Point& point)
{
  set_clipboard(Image::createCopy(image),
                pal ? new Palette(*pal): NULL, true);

  clipboard_x = point.x;
  clipboard_y = point.y;
}

void util::clipboard::paste()
{
  Editor* editor = current_editor;
  if (editor == NULL)
    return;

#ifdef ALLEGRO_WINDOWS
  // Get the image from the clipboard.
  {
    Image* win32_image = NULL;
    Palette* win32_palette = NULL;
    get_win32_clipboard_bitmap(win32_image, win32_palette);
    if (win32_image != NULL)
      set_clipboard(win32_image, win32_palette, false);
  }
#endif

  Sprite* sprite = editor->getDocument()->getSprite();

  if (clipboard_image == NULL)
    return;

  // Source image (clipboard or a converted copy to the destination 'imgtype')
  Image* src_image;
  if (clipboard_image->getPixelFormat() == sprite->getPixelFormat())
    src_image = clipboard_image;
  else {
    RgbMap* rgbmap = sprite->getRgbMap();
    src_image = quantization::convert_pixel_format(clipboard_image,
                                                   sprite->getPixelFormat(), DITHERING_NONE,
                                                   rgbmap, sprite->getPalette(sprite->getCurrentFrame()),
                                                   false);
  }

  // Change to MovingPixelsState
  editor->pasteImage(src_image, clipboard_x, clipboard_y);

  if (src_image != clipboard_image)
      image_free(src_image);
}

bool util::clipboard::get_image_size(gfx::Size& size)
{
#ifdef ALLEGRO_WINDOWS
  // Get the image from the clipboard.
  return get_win32_clipboard_bitmap_size(size);
#else
  if (clipboard_image != NULL) {
    size.w = clipboard_image->w;
    size.h = clipboard_image->h;
    return true;
  }
  else
    return false;
#endif
}
