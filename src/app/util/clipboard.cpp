// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/clear_mask.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_location.h"
#include "app/document_range.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/settings.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_parts.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/misc.h"
#include "doc/doc.h"
#include "render/quantization.h"

#ifdef _WIN32
  #include <windows.h>

  #include "app/util/clipboard_win32.h"
#endif

#include <stdexcept>

namespace app {

namespace {

  class ClipboardRange {
  public:
    ClipboardRange() : m_doc(NULL) {
    }

    bool valid() {
      return m_doc != NULL;
    }

    void invalidate() {
      m_doc = NULL;
    }

    void setRange(Document* doc, const DocumentRange& range) {
      m_doc = doc;
      m_range = range;
    }

    Document* document() const { return m_doc; }
    DocumentRange range() const { return m_range; }

  private:
    Document* m_doc;
    DocumentRange m_range;
  };

}

using namespace doc;

static void set_clipboard_image(Image* image, Palette* palette, bool set_system_clipboard);
static bool copy_from_document(const DocumentLocation& location);

static bool first_time = true;

static Palette* clipboard_palette = NULL;
static Image* clipboard_image = NULL;
static ClipboardRange clipboard_range;
static gfx::Point clipboard_pos(0, 0);

static void on_exit_delete_clipboard()
{
  delete clipboard_palette;
  delete clipboard_image;
}

static void set_clipboard_image(Image* image, Palette* palette, bool set_system_clipboard)
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
#ifdef _WIN32
  if (set_system_clipboard)
    set_win32_clipboard_bitmap(image, palette);
#endif

  clipboard_range.invalidate();
}

static bool copy_from_document(const DocumentLocation& location)
{
  const Document* document = location.document();

  ASSERT(document != NULL);
  ASSERT(document->isMaskVisible());

  Image* image = NewImageFromMask(location);
  if (!image)
    return false;

  clipboard_pos = document->mask()->bounds().getOrigin();

  const Palette* pal = document->sprite()->palette(location.frame());
  set_clipboard_image(image, pal ? new Palette(*pal): NULL, true);
  return true;
}

clipboard::ClipboardFormat clipboard::get_current_format()
{
#ifdef _WIN32
  if (win32_clipboard_contains_bitmap())
    return ClipboardImage;
#endif

  if (clipboard_image != NULL)
    return ClipboardImage;
  else if (clipboard_range.valid())
    return ClipboardDocumentRange;
  else
    return ClipboardNone;
}

void clipboard::get_document_range_info(Document** document, DocumentRange* range)
{
  if (clipboard_range.valid()) {
    *document = clipboard_range.document();
    *range = clipboard_range.range();
  }
  else {
    *document = NULL;
  }
}

void clipboard::clear_content()
{
  set_clipboard_image(NULL, NULL, true);
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
      Transaction transaction(writer.context(), "Cut");
      transaction.execute(new cmd::ClearMask(writer.cel()));
      transaction.execute(new cmd::DeselectMask(writer.document()));
      transaction.commit();
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
    return;
  }
}

void clipboard::copy_range(const ContextReader& reader, const DocumentRange& range)
{
  ASSERT(reader.document() != NULL);

  ContextWriter writer(reader);

  set_clipboard_image(NULL, NULL, true);
  clipboard_range.setRange(writer.document(), range);

  // TODO Replace this with a signal, because here the timeline
  // depends on the clipboard and the clipboard on the timeline.
  App::instance()->getMainWindow()
    ->getTimeline()->activateClipboardRange();
}

void clipboard::copy_image(Image* image, Palette* pal, const gfx::Point& point)
{
  set_clipboard_image(Image::createCopy(image),
    pal ? new Palette(*pal): NULL, true);

  clipboard_pos = point;
}

void clipboard::paste()
{
  Editor* editor = current_editor;
  if (editor == NULL)
    return;

  Document* dstDoc = editor->document();
  Sprite* dstSpr = dstDoc->sprite();

  switch (get_current_format()) {

    case clipboard::ClipboardImage: {
#ifdef _WIN32
      // Get the image from the clipboard.
      {
        Image* win32_image = NULL;
        Palette* win32_palette = NULL;
        get_win32_clipboard_bitmap(win32_image, win32_palette);
        if (win32_image != NULL)
          set_clipboard_image(win32_image, win32_palette, false);
      }
#endif

      if (clipboard_image == NULL)
        return;

      Palette* dst_palette = dstSpr->palette(editor->frame());

      // Source image (clipboard or a converted copy to the destination 'imgtype')
      Image* src_image;
      if (clipboard_image->pixelFormat() == dstSpr->pixelFormat() &&
        // Indexed images can be copied directly only if both images
        // have the same palette.
        (clipboard_image->pixelFormat() != IMAGE_INDEXED ||
          clipboard_palette->countDiff(dst_palette, NULL, NULL) == 0)) {
        src_image = clipboard_image;
      }
      else {
        RgbMap* dst_rgbmap = dstSpr->rgbMap(editor->frame());

        src_image = render::convert_pixel_format(
          clipboard_image, NULL, dstSpr->pixelFormat(),
          DitheringMethod::NONE, dst_rgbmap, clipboard_palette,
          false);
      }

      // Change to MovingPixelsState
      editor->pasteImage(src_image, clipboard_pos);

      if (src_image != clipboard_image)
        delete src_image;
      break;
    }

    case clipboard::ClipboardDocumentRange: {
      DocumentRange srcRange = clipboard_range.range();
      Document* srcDoc = clipboard_range.document();
      Sprite* srcSpr = srcDoc->sprite();
      std::vector<Layer*> srcLayers;
      std::vector<Layer*> dstLayers;
      srcSpr->getLayersList(srcLayers);
      dstSpr->getLayersList(dstLayers);

      switch (srcRange.type()) {

        case DocumentRange::kCels: {
          // Do nothing case: pasting in the same document.
          if (srcDoc == dstDoc)
            return;

          Transaction transaction(UIContext::instance(), "Paste Cels");
          DocumentApi api = dstDoc->getApi(transaction);

          frame_t dstFrame = editor->frame();
          for (frame_t frame = srcRange.frameBegin(); frame <= srcRange.frameEnd(); ++frame) {
            if (dstFrame == dstSpr->totalFrames())
              api.addFrame(dstSpr, dstFrame);

            for (LayerIndex
                   i = srcRange.layerEnd(),
                   j = dstSpr->layerToIndex(editor->layer());
                   i >= srcRange.layerBegin() &&
                   i >= LayerIndex(0) &&
                   j >= LayerIndex(0); --i, --j) {
              Cel* cel = srcLayers[i]->cel(frame);

              if (cel && cel->image()) {
                api.copyCel(
                  static_cast<LayerImage*>(srcLayers[i]), frame,
                  static_cast<LayerImage*>(dstLayers[j]), dstFrame);
              }
              else {
                Cel* dstCel = dstLayers[j]->cel(dstFrame);
                if (dstCel)
                  api.clearCel(dstCel);
              }
            }

            ++dstFrame;
          }

          transaction.commit();
          editor->invalidate();
          break;
        }

        case DocumentRange::kFrames: {
          Transaction transaction(UIContext::instance(), "Paste Frames");
          DocumentApi api = dstDoc->getApi(transaction);
          frame_t dstFrame = frame_t(editor->frame() + 1);

          for (frame_t frame = srcRange.frameBegin(); frame <= srcRange.frameEnd(); ++frame) {
            api.addFrame(dstSpr, dstFrame);

            for (LayerIndex
                   i = LayerIndex(srcLayers.size()-1),
                   j = LayerIndex(dstLayers.size()-1);
                   i >= LayerIndex(0) &&
                   j >= LayerIndex(0); --i, --j) {
              Cel* cel = static_cast<LayerImage*>(srcLayers[i])->cel(frame);
              if (cel && cel->image()) {
                api.copyCel(
                  static_cast<LayerImage*>(srcLayers[i]), frame,
                  static_cast<LayerImage*>(dstLayers[j]), dstFrame);
              }
            }

            ++dstFrame;
          }

          transaction.commit();
          editor->invalidate();
          break;
        }

        case DocumentRange::kLayers: {
          if (srcDoc->colorMode() != dstDoc->colorMode())
            throw std::runtime_error("You cannot copy layers of document with different color modes");

          Transaction transaction(UIContext::instance(), "Paste Layers");
          DocumentApi api = dstDoc->getApi(transaction);

          // Expand frames of dstDoc if it's needed.
          frame_t maxFrame(0);
          for (LayerIndex i = srcRange.layerBegin();
               i <= srcRange.layerEnd() &&
               i < LayerIndex(srcLayers.size()); ++i) {
            Cel* lastCel = static_cast<LayerImage*>(srcLayers[i])->getLastCel();
            if (lastCel && maxFrame < lastCel->frame())
              maxFrame = lastCel->frame();
          }
          while (dstSpr->totalFrames() < maxFrame+1)
            api.addEmptyFrame(dstSpr, dstSpr->totalFrames());

          for (LayerIndex i = srcRange.layerBegin(); i <= srcRange.layerEnd(); ++i) {
            Layer* afterThis;
            if (srcLayers[i]->isBackground() &&
                !dstDoc->sprite()->backgroundLayer()) {
              afterThis = nullptr;
            }
            else
              afterThis = dstSpr->folder()->getLastLayer();

            LayerImage* newLayer = new LayerImage(dstSpr);
            api.addLayer(dstSpr->folder(), newLayer, afterThis);

            srcDoc->copyLayerContent(
              srcLayers[i], dstDoc, newLayer);
          }

          transaction.commit();
          editor->invalidate();
          break;
        }

      }
      break;
    }

  }
}

bool clipboard::get_image_size(gfx::Size& size)
{
#ifdef _WIN32
  // Get the image from the clipboard.
  return get_win32_clipboard_bitmap_size(size);
#else
  if (clipboard_image != NULL) {
    size.w = clipboard_image->width();
    size.h = clipboard_image->height();
    return true;
  }
  else
    return false;
#endif
}

} // namespace app
