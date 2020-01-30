// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/deselect_mask.h"
#include "app/cmd/set_mask.h"
#include "app/cmd/trim_cel.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/doc_range.h"
#include "app/doc_range_ops.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/clipboard_native.h"
#include "app/util/new_image_from_mask.h"
#include "app/util/range_utils.h"
#include "clip/clip.h"
#include "doc/doc.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"

#include <memory>
#include <stdexcept>

namespace app {

namespace {

  class ClipboardRange : public DocsObserver {
  public:
    ClipboardRange() : m_doc(nullptr) {
    }

    ~ClipboardRange() {
      ASSERT(!m_doc);
    }

    void observeUIContext() {
      UIContext::instance()->documents().add_observer(this);
    }

    void unobserveUIContext() {
      UIContext::instance()->documents().remove_observer(this);
    }

    bool valid() {
      return (m_doc != nullptr);
    }

    void invalidate() {
      m_doc = nullptr;
    }

    void setRange(Doc* doc, const DocRange& range) {
      m_doc = doc;
      m_range = range;
    }

    Doc* document() const { return m_doc; }
    DocRange range() const { return m_range; }

    // DocsObserver impl
    void onRemoveDocument(Doc* doc) override {
      if (doc == m_doc)
        invalidate();
    }

  private:
    Doc* m_doc;
    DocRange m_range;
  };

}

namespace clipboard {

using namespace doc;

static std::shared_ptr<Palette> clipboard_palette;
static PalettePicks clipboard_picks;
static ImageRef clipboard_image;
static std::shared_ptr<Mask> clipboard_mask;
static ClipboardRange clipboard_range;

static ClipboardManager* g_instance = nullptr;

static bool use_native_clipboard()
{
  return Preferences::instance().experimental.useNativeClipboard();
}

ClipboardManager* ClipboardManager::instance()
{
  return g_instance;
}

ClipboardManager::ClipboardManager()
{
  ASSERT(!g_instance);
  g_instance = this;

  register_native_clipboard_formats();

  clipboard_range.observeUIContext();
}

ClipboardManager::~ClipboardManager()
{
  clipboard_range.invalidate();
  clipboard_range.unobserveUIContext();

  // Clean the whole clipboard
  clipboard_palette.reset();
  clipboard_image.reset();
  clipboard_mask.reset();

  ASSERT(g_instance == this);
  g_instance = nullptr;
}

void ClipboardManager::setClipboardText(const std::string& text)
{
  if (use_native_clipboard()) {
    clip::set_text(text);
  }
  else {
    m_text = text;
  }
}

bool ClipboardManager::getClipboardText(std::string& text)
{
  if (use_native_clipboard()) {
    return clip::get_text(text);
  }
  else {
    text = m_text;
    return true;
  }
}

static void set_clipboard_image(Image* image,
                                Mask* mask,
                                Palette* palette,
                                bool set_system_clipboard,
                                bool image_source_is_transparent)
{
  clipboard_palette.reset(palette);
  clipboard_picks.clear();
  clipboard_image.reset(image);
  clipboard_mask.reset(mask);

  // Copy image to the native clipboard
  if (set_system_clipboard) {
    color_t oldMask;
    if (image) {
      oldMask = image->maskColor();
      if (!image_source_is_transparent)
        image->setMaskColor(-1);
    }

    if (use_native_clipboard())
      set_native_clipboard_bitmap(image, mask, palette);

    if (image && !image_source_is_transparent)
      image->setMaskColor(oldMask);
  }

  clipboard_range.invalidate();
}

static bool copy_from_document(const Site& site, bool merged = false)
{
  const Doc* document = static_cast<const Doc*>(site.document());
  ASSERT(document);

  const Mask* mask = document->mask();
  Image* image = new_image_from_mask(site, mask,
                                     Preferences::instance().experimental.newBlend(),
                                     merged);
  if (!image)
    return false;

  const Palette* pal = document->sprite()->palette(site.frame());
  set_clipboard_image(
    image,
    (mask ? new Mask(*mask): nullptr),
    (pal ? new Palette(*pal): nullptr),
    true,
    site.layer() && !site.layer()->isBackground());

  return true;
}

ClipboardFormat get_current_format()
{
  // Check if the native clipboard has an image
  if (use_native_clipboard() &&
      has_native_clipboard_bitmap())
    return ClipboardImage;
  else if (clipboard_image)
    return ClipboardImage;
  else if (clipboard_range.valid())
    return ClipboardDocRange;
  else if (clipboard_palette && clipboard_picks.picks())
    return ClipboardPaletteEntries;
  else
    return ClipboardNone;
}

void get_document_range_info(Doc** document, DocRange* range)
{
  if (clipboard_range.valid()) {
    *document = clipboard_range.document();
    *range = clipboard_range.range();
  }
  else {
    *document = NULL;
  }
}

void clear_mask_from_cels(Tx& tx,
                          Doc* doc,
                          const CelList& cels,
                          const bool deselectMask)
{
  for (Cel* cel : cels) {
    ObjectId celId = cel->id();

    tx(new cmd::ClearMask(cel));

    // Get cel again just in case the cmd::ClearMask() called cmd::ClearCel()
    cel = doc::get<Cel>(celId);
    if (cel &&
        cel->layer()->isTransparent()) {
      tx(new cmd::TrimCel(cel));
    }
  }

  if (deselectMask)
    tx(new cmd::DeselectMask(doc));
}

void clear_content()
{
  set_clipboard_image(nullptr, nullptr, nullptr, true, false);
}

void cut(ContextWriter& writer)
{
  ASSERT(writer.document() != NULL);
  ASSERT(writer.sprite() != NULL);
  ASSERT(writer.layer() != NULL);

  if (!copy_from_document(*writer.site())) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
  else {
    {
      Tx tx(writer.context(), "Cut");
      Site site = writer.context()->activeSite();
      CelList cels;
      if (site.range().enabled()) {
        cels = get_unlocked_unique_cels(site.sprite(), site.range());
      }
      else if (site.cel()) {
        cels.push_back(site.cel());
      }
      clear_mask_from_cels(tx,
                           writer.document(),
                           cels,
                           true); // Deselect mask
      tx.commit();
    }
    writer.document()->generateMaskBoundaries();
    update_screen_for_document(writer.document());
  }
}

void copy(const ContextReader& reader)
{
  ASSERT(reader.document() != NULL);

  if (!copy_from_document(*reader.site())) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
    return;
  }
}

void copy_merged(const ContextReader& reader)
{
  ASSERT(reader.document() != NULL);

  copy_from_document(*reader.site(), true);
}

void copy_range(const ContextReader& reader, const DocRange& range)
{
  ASSERT(reader.document() != NULL);

  ContextWriter writer(reader);

  clear_content();
  clipboard_range.setRange(writer.document(), range);

  // TODO Replace this with a signal, because here the timeline
  // depends on the clipboard and the clipboard on the timeline.
  App::instance()->timeline()->activateClipboardRange();
}

void copy_image(const Image* image, const Mask* mask, const Palette* pal)
{
  set_clipboard_image(
    Image::createCopy(image),
    (mask ? new Mask(*mask): nullptr),
    (pal ? new Palette(*pal): nullptr),
    true, false);
}

void copy_palette(const Palette* palette, const doc::PalettePicks& picks)
{
  if (!picks.picks())
    return;                     // Do nothing case

  set_clipboard_image(nullptr,
                      nullptr,
                      new Palette(*palette),
                      true, false);
  clipboard_picks = picks;
}

void paste(Context* ctx, const bool interactive)
{
  Site site = ctx->activeSite();
  Doc* dstDoc = site.document();
  if (!dstDoc)
    return;

  Sprite* dstSpr = site.sprite();
  if (!dstSpr)
    return;

  switch (get_current_format()) {

    case clipboard::ClipboardImage: {
      // Get the image from the native clipboard.
      if (!get_image(nullptr))
        return;

      ASSERT(clipboard_image);

      Palette* dst_palette = dstSpr->palette(site.frame());

      // Source image (clipboard or a converted copy to the destination 'imgtype')
      ImageRef src_image;
      if (clipboard_image->pixelFormat() == dstSpr->pixelFormat() &&
        // Indexed images can be copied directly only if both images
        // have the same palette.
        (clipboard_image->pixelFormat() != IMAGE_INDEXED ||
          clipboard_palette->countDiff(dst_palette, NULL, NULL) == 0)) {
        src_image = clipboard_image;
      }
      else {
        RgbMap* dst_rgbmap = dstSpr->rgbMap(site.frame());

        src_image.reset(
          render::convert_pixel_format(
            clipboard_image.get(), NULL, dstSpr->pixelFormat(),
            render::Dithering(),
            dst_rgbmap, clipboard_palette.get(),
            false,
            0));
      }

      if (current_editor && interactive) {
        // TODO we don't support pasting in multiple cels at the
        //      moment, so we clear the range here (same as in
        //      PasteTextCommand::onExecute())
        App::instance()->timeline()->clearAndInvalidateRange();

        // Change to MovingPixelsState
        current_editor->pasteImage(src_image.get(),
                                   clipboard_mask.get());
      }
      else {
        // Non-interactive version (just copy the image to the cel)
        Layer* dstLayer = site.layer();
        ASSERT(dstLayer);
        if (!dstLayer || !dstLayer->isImage())
          return;

        Tx tx(ctx, "Paste Image");
        DocApi api = dstDoc->getApi(tx);
        Cel* dstCel = api.addCel(
          static_cast<LayerImage*>(dstLayer), site.frame(),
          ImageRef(Image::createCopy(src_image.get())));

        // Adjust bounds
        if (dstCel) {
          if (clipboard_mask) {
            if (dstLayer->isReference()) {
              dstCel->setBounds(dstSpr->bounds());

              Mask emptyMask;
              tx(new cmd::SetMask(dstDoc, &emptyMask));
            }
            else {
              dstCel->setBounds(clipboard_mask->bounds());
              tx(new cmd::SetMask(dstDoc, clipboard_mask.get()));
            }
          }
        }

        tx.commit();
      }
      break;
    }

    case clipboard::ClipboardDocRange: {
      DocRange srcRange = clipboard_range.range();
      Doc* srcDoc = clipboard_range.document();
      Sprite* srcSpr = srcDoc->sprite();

      switch (srcRange.type()) {

        case DocRange::kCels: {
          Layer* dstLayer = site.layer();
          ASSERT(dstLayer);
          if (!dstLayer)
            return;

          frame_t dstFrameFirst = site.frame();

          DocRange dstRange;
          dstRange.startRange(dstLayer, dstFrameFirst, DocRange::kCels);
          for (layer_t i=1; i<srcRange.layers(); ++i) {
            dstLayer = dstLayer->getPreviousBrowsable();
            if (dstLayer == nullptr)
              break;
          }
          dstRange.endRange(dstLayer, dstFrameFirst+srcRange.frames()-1);

          // We can use a document range op (copy_range) to copy/paste
          // cels in the same document.
          if (srcDoc == dstDoc) {
            // This is the app::copy_range (not clipboard::copy_range()).
            if (srcRange.layers() == dstRange.layers())
              app::copy_range(srcDoc, srcRange, dstRange, kDocRangeBefore);
            if (current_editor)
              current_editor->invalidate(); // TODO check if this is necessary
            return;
          }

          Tx tx(ctx, "Paste Cels");
          DocApi api = dstDoc->getApi(tx);

          // Add extra frames if needed
          while (dstFrameFirst+srcRange.frames() > dstSpr->totalFrames())
            api.addFrame(dstSpr, dstSpr->totalFrames());

          auto srcLayers = srcRange.selectedLayers().toLayerList();
          auto dstLayers = dstRange.selectedLayers().toLayerList();

          auto srcIt = srcLayers.begin();
          auto dstIt = dstLayers.begin();
          auto srcEnd = srcLayers.end();
          auto dstEnd = dstLayers.end();

          for (; srcIt != srcEnd && dstIt != dstEnd; ++srcIt, ++dstIt) {
            auto srcLayer = *srcIt;
            auto dstLayer = *dstIt;

            if (!srcLayer->isImage() ||
                !dstLayer->isImage())
              continue;

            frame_t dstFrame = dstFrameFirst;
            for (frame_t srcFrame : srcRange.selectedFrames()) {
              Cel* srcCel = srcLayer->cel(srcFrame);

              if (srcCel && srcCel->image()) {
                api.copyCel(
                  static_cast<LayerImage*>(srcLayer), srcFrame,
                  static_cast<LayerImage*>(dstLayer), dstFrame);
              }
              else {
                if (Cel* dstCel = dstLayer->cel(dstFrame))
                  api.clearCel(dstCel);
              }

              ++dstFrame;
            }
          }

          tx.commit();
          if (current_editor)
            current_editor->invalidate(); // TODO check if this is necessary
          break;
        }

        case DocRange::kFrames: {
          frame_t dstFrame = site.frame();

          // We use a DocRange operation to copy frames inside
          // the same sprite.
          if (srcSpr == dstSpr) {
            DocRange dstRange;
            dstRange.startRange(nullptr, dstFrame, DocRange::kFrames);
            dstRange.endRange(nullptr, dstFrame);
            app::copy_range(srcDoc, srcRange, dstRange, kDocRangeBefore);
            break;
          }

          Tx tx(ctx, "Paste Frames");
          DocApi api = dstDoc->getApi(tx);

          auto srcLayers = srcSpr->allBrowsableLayers();
          auto dstLayers = dstSpr->allBrowsableLayers();

          for (frame_t srcFrame : srcRange.selectedFrames()) {
            api.addEmptyFrame(dstSpr, dstFrame);
            api.setFrameDuration(dstSpr, dstFrame, srcSpr->frameDuration(srcFrame));

            auto srcIt = srcLayers.begin();
            auto dstIt = dstLayers.begin();
            auto srcEnd = srcLayers.end();
            auto dstEnd = dstLayers.end();

            for (; srcIt != srcEnd && dstIt != dstEnd; ++srcIt, ++dstIt) {
              auto srcLayer = *srcIt;
              auto dstLayer = *dstIt;

              if (!srcLayer->isImage() ||
                  !dstLayer->isImage())
                continue;

              Cel* cel = static_cast<LayerImage*>(srcLayer)->cel(srcFrame);
              if (cel && cel->image()) {
                api.copyCel(
                  static_cast<LayerImage*>(srcLayer), srcFrame,
                  static_cast<LayerImage*>(dstLayer), dstFrame);
              }
            }

            ++dstFrame;
          }

          tx.commit();
          if (current_editor)
            current_editor->invalidate(); // TODO check if this is necessary
          break;
        }

        case DocRange::kLayers: {
          if (srcDoc->colorMode() != dstDoc->colorMode())
            throw std::runtime_error("You cannot copy layers of document with different color modes");

          Tx tx(ctx, "Paste Layers");
          DocApi api = dstDoc->getApi(tx);

          // Remove children if their parent is selected so we only
          // copy the parent.
          SelectedLayers srcLayersSet = srcRange.selectedLayers();
          srcLayersSet.removeChildrenIfParentIsSelected();
          LayerList srcLayers = srcLayersSet.toLayerList();

          // Expand frames of dstDoc if it's needed.
          frame_t maxFrame = 0;
          for (Layer* srcLayer : srcLayers) {
            if (!srcLayer->isImage())
              continue;

            Cel* lastCel = static_cast<LayerImage*>(srcLayer)->getLastCel();
            if (lastCel && maxFrame < lastCel->frame())
              maxFrame = lastCel->frame();
          }
          while (dstSpr->totalFrames() < maxFrame+1)
            api.addEmptyFrame(dstSpr, dstSpr->totalFrames());

          for (Layer* srcLayer : srcLayers) {
            Layer* afterThis;
            if (srcLayer->isBackground() && !dstDoc->sprite()->backgroundLayer())
              afterThis = nullptr;
            else
              afterThis = dstSpr->root()->lastLayer();

            Layer* newLayer = nullptr;
            if (srcLayer->isImage())
              newLayer = new LayerImage(dstSpr);
            else if (srcLayer->isGroup())
              newLayer = new LayerGroup(dstSpr);
            else
              continue;

            api.addLayer(dstSpr->root(), newLayer, afterThis);

            srcDoc->copyLayerContent(srcLayer, dstDoc, newLayer);
          }

          tx.commit();
          if (current_editor)
            current_editor->invalidate(); // TODO check if this is necessary
          break;
        }
      }
      break;
    }

  }
}

ImageRef get_image(Palette* palette)
{
  // Get the image from the native clipboard.
  if (use_native_clipboard()) {
    Image* native_image = nullptr;
    Mask* native_mask = nullptr;
    Palette* native_palette = nullptr;
    get_native_clipboard_bitmap(&native_image, &native_mask, &native_palette);
    if (native_image)
      set_clipboard_image(native_image, native_mask, native_palette,
                          false, false);
  }
  if (clipboard_palette && palette)
    clipboard_palette->copyColorsTo(palette);
  return clipboard_image;
}

bool get_image_size(gfx::Size& size)
{
  if (use_native_clipboard() &&
      get_native_clipboard_bitmap_size(&size))
    return true;

  if (clipboard_image) {
    size.w = clipboard_image->width();
    size.h = clipboard_image->height();
    return true;
  }

  return false;
}

Palette* get_palette()
{
  if (clipboard::get_current_format() == ClipboardPaletteEntries) {
    ASSERT(clipboard_palette);
    return clipboard_palette.get();
  }
  else
    return nullptr;
}

const PalettePicks& get_palette_picks()
{
  return clipboard_picks;
}

}  // namespace clipboard

} // namespace app
