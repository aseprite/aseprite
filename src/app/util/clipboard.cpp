/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_location.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/settings.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_parts.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/undo_transaction.h"
#include "app/undoers/add_image.h"
#include "app/undoers/image_area.h"
#include "app/util/clipboard.h"
#include "app/util/misc.h"
#include "raster/raster.h"
#include "ui/ui.h"
#include "undo/undo_history.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#if defined ALLEGRO_WINDOWS
  #include <winalleg.h>

  #include "app/util/clipboard_win32.h"
#endif

namespace app {

using namespace raster;

static void set_clipboard(Image* image, Palette* palette, bool set_system_clipboard);
static bool copy_from_document(const DocumentLocation& location);

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

static bool copy_from_document(const DocumentLocation& location)
{
  const Document* document = location.document();

  ASSERT(document != NULL);
  ASSERT(document->isMaskVisible());

  Image* image = NewImageFromMask(location);
  if (!image)
    return false;

  clipboard_x = document->getMask()->getBounds().x;
  clipboard_y = document->getMask()->getBounds().y;

  const Palette* pal = document->sprite()->getPalette(location.frame());
  set_clipboard(image, pal ? new Palette(*pal): NULL, true);
  return true;
}

bool clipboard::can_paste()
{
#ifdef ALLEGRO_WINDOWS
  if (win32_clipboard_contains_bitmap())
    return true;
#endif
  return clipboard_image != NULL;
}

void clipboard::cut(ContextWriter& writer)
{
  ASSERT(writer.document() != NULL);
  ASSERT(writer.sprite() != NULL);
  ASSERT(writer.layer() != NULL);

  if (!copy_from_document(*writer.location())) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
  else {
    {
      UndoTransaction undoTransaction(writer.context(), "Cut");
      DocumentApi api(writer.document()->getApi());

      api.clearMask(writer.layer(), writer.cel(),
                    app_get_color_to_clear_layer(writer.layer()));
      api.deselectMask();

      undoTransaction.commit();
    }
    writer.document()->generateMaskBoundaries();
    update_screen_for_document(writer.document());
  }
}

void clipboard::copy(const ContextReader& reader)
{
  ASSERT(reader.document() != NULL);

  if (!copy_from_document(*reader.location())) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
}

void clipboard::copy_image(Image* image, Palette* pal, const gfx::Point& point)
{
  set_clipboard(Image::createCopy(image),
                pal ? new Palette(*pal): NULL, true);

  clipboard_x = point.x;
  clipboard_y = point.y;
}

void clipboard::paste()
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

  Sprite* dst_sprite = editor->getDocument()->sprite();
  if (clipboard_image == NULL)
    return;

  Palette* dst_palette = dst_sprite->getPalette(editor->getFrame());

  // Source image (clipboard or a converted copy to the destination 'imgtype')
  Image* src_image;
  if (clipboard_image->getPixelFormat() == dst_sprite->getPixelFormat() &&
      // Indexed images can be copied directly only if both images
      // have the same palette.
      (clipboard_image->getPixelFormat() != IMAGE_INDEXED ||
       clipboard_palette->countDiff(dst_palette, NULL, NULL) == 0)) {
    src_image = clipboard_image;
  }
  else {
    RgbMap* dst_rgbmap = dst_sprite->getRgbMap(editor->getFrame());

    src_image = quantization::convert_pixel_format(
      clipboard_image, NULL, dst_sprite->getPixelFormat(),
      DITHERING_NONE, dst_rgbmap, clipboard_palette,
      false);
  }

  // Change to MovingPixelsState
  editor->pasteImage(src_image, clipboard_x, clipboard_y);

  if (src_image != clipboard_image)
      delete src_image;
}

bool clipboard::get_image_size(gfx::Size& size)
{
#ifdef ALLEGRO_WINDOWS
  // Get the image from the clipboard.
  return get_win32_clipboard_bitmap_size(size);
#else
  if (clipboard_image != NULL) {
    size.w = clipboard_image->getWidth();
    size.h = clipboard_image->getHeight();
    return true;
  }
  else
    return false;
#endif
}

} // namespace app
