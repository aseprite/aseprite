// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
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
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/util/cel_ops.h"
#include "app/util/clipboard.h"
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

using namespace doc;

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

    bool valid() const {
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

// Data in the clipboard
struct Clipboard::Data {
  // Text used when the native clipboard is disabled
  std::string text;

  // RGB/Grayscale/Indexed image
  ImageRef image;

  // The palette of the image (or tileset) if it's indexed
  std::shared_ptr<Palette> palette;

  // In case we copy a tilemap information
  ImageRef tilemap;

  // Tileset for the tilemap or a set of tiles if we are copying tiles
  // in the color bar
  std::shared_ptr<Tileset> tileset;

  // Selected entries copied from the palette or the tileset
  PalettePicks picks;

  // Original selection used to copy the image
  std::shared_ptr<Mask> mask;

  // Selected set of layers/layers/cels
  ClipboardRange range;

  Data() {
    range.observeUIContext();
  }

  ~Data() {
    clear();
    range.unobserveUIContext();
  }

  void clear() {
    text.clear();
    image.reset();
    palette.reset();
    tilemap.reset();
    tileset.reset();
    picks.clear();
    mask.reset();
    range.invalidate();
  }

  ClipboardFormat format() const {
    if (image)
      return ClipboardFormat::Image;
    else if (tilemap)
      return ClipboardFormat::Tilemap;
    else if (range.valid())
      return ClipboardFormat::DocRange;
    else if (palette && picks.picks())
      return ClipboardFormat::PaletteEntries;
    else if (tileset && picks.picks())
      return ClipboardFormat::Tileset;
    else
      return ClipboardFormat::None;
  }
};

static bool use_native_clipboard()
{
  return Preferences::instance().experimental.useNativeClipboard();
}

static Clipboard* g_instance = nullptr;

Clipboard* Clipboard::instance()
{
  return g_instance;
}

Clipboard::Clipboard()
  : m_data(new Data)
{
  ASSERT(!g_instance);
  g_instance = this;

  registerNativeFormats();
}

Clipboard::~Clipboard()
{
  ASSERT(g_instance == this);
  g_instance = nullptr;
}

void Clipboard::setClipboardText(const std::string& text)
{
  if (use_native_clipboard()) {
    clip::set_text(text);
  }
  else {
    m_data->text = text;
  }
}

bool Clipboard::getClipboardText(std::string& text)
{
  if (use_native_clipboard()) {
    return clip::get_text(text);
  }
  else {
    text = m_data->text;
    return true;
  }
}

void Clipboard::setData(Image* image,
                        Mask* mask,
                        Palette* palette,
                        Tileset* tileset,
                        bool set_native_clipboard,
                        bool image_source_is_transparent)
{
  const bool isTilemap = (image && image->isTilemap());

  m_data->clear();
  m_data->palette.reset(palette);
  m_data->tileset.reset(tileset);
  m_data->mask.reset(mask);
  if (isTilemap)
    m_data->tilemap.reset(image);
  else
    m_data->image.reset(image);

  if (set_native_clipboard) {
    // Copy tilemap to the native clipboard
    if (isTilemap) {
      ASSERT(tileset);
      setNativeBitmap(image, mask, palette, tileset);
    }
    // Copy non-tilemap images to the native clipboard
    else {
      color_t oldMask = 0;
      if (image) {
        oldMask = image->maskColor();
        if (!image_source_is_transparent)
          image->setMaskColor(-1);
      }

      if (use_native_clipboard())
        setNativeBitmap(image, mask, palette);

      if (image && !image_source_is_transparent)
        image->setMaskColor(oldMask);
    }
  }
}

bool Clipboard::copyFromDocument(const Site& site, bool merged)
{
  ASSERT(site.document());
  const Doc* doc = static_cast<const Doc*>(site.document());
  const Mask* mask = doc->mask();
  const Palette* pal = doc->sprite()->palette(site.frame());

  if (!merged &&
      site.layer() &&
      site.layer()->isTilemap() &&
      site.tilemapMode() == TilemapMode::Tiles) {
    const Tileset* ts = static_cast<LayerTilemap*>(site.layer())->tileset();

    Image* image = new_tilemap_from_mask(site, mask);
    if (!image)
      return false;

    setData(
      image,
      (mask ? new Mask(*mask): nullptr),
      (pal ? new Palette(*pal): nullptr),
      Tileset::MakeCopyCopyingImages(ts),
      true,                       // set native clipboard
      site.layer() && !site.layer()->isBackground());

    return true;
  }

  Image* image = new_image_from_mask(site, mask,
                                     Preferences::instance().experimental.newBlend(),
                                     merged);
  if (!image)
    return false;

  setData(
    image,
    (mask ? new Mask(*mask): nullptr),
    (pal ? new Palette(*pal): nullptr),
    nullptr,
    true,                       // set native clipboard
    site.layer() && !site.layer()->isBackground());

  return true;
}

ClipboardFormat Clipboard::format() const
{
  // Check if the native clipboard has an image
  if (use_native_clipboard() && hasNativeBitmap()) {
    return ClipboardFormat::Image;
  }
  else {
    return m_data->format();
  }
}

void Clipboard::getDocumentRangeInfo(Doc** document, DocRange* range)
{
  if (m_data->range.valid()) {
    *document = m_data->range.document();
    *range = m_data->range.range();
  }
  else {
    *document = NULL;
  }
}

void Clipboard::clearMaskFromCels(Tx& tx,
                                  Doc* doc,
                                  const Site& site,
                                  const CelList& cels,
                                  const bool deselectMask)
{
  for (Cel* cel : cels) {
    ObjectId celId = cel->id();

    clear_mask_from_cel(
      tx, cel,
      site.tilemapMode(),
      site.tilesetMode());

    // Get cel again just in case the cmd::ClearMask() called cmd::ClearCel()
    cel = doc::get<Cel>(celId);
    if (site.shouldTrimCel(cel))
      tx(new cmd::TrimCel(cel));
  }

  if (deselectMask)
    tx(new cmd::DeselectMask(doc));
}

void Clipboard::clearContent()
{
  if (use_native_clipboard())
    clearNativeContent();
  m_data->clear();
}

void Clipboard::cut(ContextWriter& writer)
{
  ASSERT(writer.document() != NULL);
  ASSERT(writer.sprite() != NULL);
  ASSERT(writer.layer() != NULL);

  if (!copyFromDocument(*writer.site())) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
  else {
    // TODO This code is similar to DocView::onClear()
    {
      Tx tx(writer.context(), "Cut");
      Site site = writer.context()->activeSite();
      CelList cels;
      if (site.range().enabled()) {
        cels = get_unique_cels_to_edit_pixels(site.sprite(), site.range());
      }
      else if (site.cel()) {
        cels.push_back(site.cel());
      }
      clearMaskFromCels(tx,
                        writer.document(),
                        site,
                        cels,
                        true); // Deselect mask
      tx.commit();
    }
    writer.document()->generateMaskBoundaries();
    update_screen_for_document(writer.document());
  }
}

void Clipboard::copy(const ContextReader& reader)
{
  ASSERT(reader.document() != NULL);

  if (!copyFromDocument(*reader.site())) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
    return;
  }
}

void Clipboard::copyMerged(const ContextReader& reader)
{
  ASSERT(reader.document() != NULL);

  copyFromDocument(*reader.site(), true);
}

void Clipboard::copyRange(const ContextReader& reader, const DocRange& range)
{
  ASSERT(reader.document() != NULL);

  ContextWriter writer(reader);

  clearContent();
  m_data->range.setRange(writer.document(), range);

  // TODO Replace this with a signal, because here the timeline
  // depends on the clipboard and the clipboard on the timeline.
  App::instance()->timeline()->activateClipboardRange();
}

void Clipboard::copyImage(const Image* image,
                          const Mask* mask,
                          const Palette* pal)
{
  ASSERT(image->pixelFormat() != IMAGE_TILEMAP);
  setData(
    Image::createCopy(image),
    (mask ? new Mask(*mask): nullptr),
    (pal ? new Palette(*pal): nullptr),
    nullptr,
    true, false);
}

void Clipboard::copyTilemap(const Image* image,
                            const Mask* mask,
                            const Palette* pal,
                            const Tileset* tileset)
{
  ASSERT(image->pixelFormat() == IMAGE_TILEMAP);
  setData(
    Image::createCopy(image),
    (mask ? new Mask(*mask): nullptr),
    (pal ? new Palette(*pal): nullptr),
    Tileset::MakeCopyCopyingImages(tileset),
    true, false);
}

void Clipboard::copyPalette(const Palette* palette,
                            const PalettePicks& picks)
{
  if (!picks.picks())
    return;                     // Do nothing case

  setData(nullptr,
          nullptr,
          new Palette(*palette),
          nullptr,
          true,                 // set native clipboard
          false);
  m_data->picks = picks;
}

void Clipboard::paste(Context* ctx,
                      const bool interactive)
{
  Site site = ctx->activeSite();
  Doc* dstDoc = site.document();
  if (!dstDoc)
    return;

  Sprite* dstSpr = site.sprite();
  if (!dstSpr)
    return;

  auto editor = Editor::activeEditor();
  bool updateDstDoc = false;

  switch (format()) {

    case ClipboardFormat::Image: {
      // Get the image from the native clipboard.
      if (!getImage(nullptr))
        return;

      ASSERT(m_data->image);

      Palette* dst_palette = dstSpr->palette(site.frame());

      // Source image (clipboard or a converted copy to the destination 'imgtype')
      ImageRef src_image;
      if (// Copy image of the same pixel format
        (m_data->image->pixelFormat() == dstSpr->pixelFormat() &&
         // Indexed images can be copied directly only if both images
         // have the same palette.
         (m_data->image->pixelFormat() != IMAGE_INDEXED ||
          m_data->palette->countDiff(dst_palette, NULL, NULL) == 0))) {
        src_image = m_data->image;
      }
      else {
        RgbMap* dst_rgbmap = dstSpr->rgbMap(site.frame());

        src_image.reset(
          render::convert_pixel_format(
            m_data->image.get(), NULL, dstSpr->pixelFormat(),
            render::Dithering(),
            dst_rgbmap, m_data->palette.get(),
            false,
            0));
      }

      if (editor && interactive) {
        // TODO we don't support pasting in multiple cels at the
        //      moment, so we clear the range here (same as in
        //      PasteTextCommand::onExecute())
        App::instance()->timeline()->clearAndInvalidateRange();

        // Change to MovingPixelsState
        editor->pasteImage(src_image.get(),
                           m_data->mask.get());
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
          if (m_data->mask) {
            if (dstLayer->isReference()) {
              dstCel->setBounds(dstSpr->bounds());

              Mask emptyMask;
              tx(new cmd::SetMask(dstDoc, &emptyMask));
            }
            else {
              dstCel->setBounds(m_data->mask->bounds());
              tx(new cmd::SetMask(dstDoc, m_data->mask.get()));
            }
          }
        }

        tx.commit();
      }
      break;
    }

    case ClipboardFormat::Tilemap: {
      if (editor && interactive) {
        // TODO match both tilesets?
        // TODO add post-command parameters (issue #2324)

        // Change to MovingTilemapState
        editor->pasteImage(m_data->tilemap.get(),
                           m_data->mask.get());
      }
      else {
        // TODO non-interactive version (for scripts)
      }
      break;
    }

    case ClipboardFormat::DocRange: {
      DocRange srcRange = m_data->range.range();
      Doc* srcDoc = m_data->range.document();
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
            if (srcRange.layers() == dstRange.layers()) {
              app::copy_range(srcDoc, srcRange, dstRange, kDocRangeBefore);
              updateDstDoc = true;
            }
            break;
          }

          Tx tx(ctx, "Paste Cels");
          DocApi api = dstDoc->getApi(tx);

          // Add extra frames if needed
          while (dstFrameFirst+srcRange.frames() > dstSpr->totalFrames())
            api.addFrame(dstSpr, dstSpr->totalFrames());

          auto srcLayers = srcRange.selectedLayers().toBrowsableLayerList();
          auto dstLayers = dstRange.selectedLayers().toBrowsableLayerList();

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
          updateDstDoc = true;
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
            updateDstDoc = true;
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
          updateDstDoc = true;
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
          LayerList srcLayers = srcLayersSet.toBrowsableLayerList();

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
          updateDstDoc = true;
          break;
        }
      }
      break;
    }

  }

  // Update all editors/views showing this document
  if (updateDstDoc)
    dstDoc->notifyGeneralUpdate();
}

ImageRef Clipboard::getImage(Palette* palette)
{
  // Get the image from the native clipboard.
  if (use_native_clipboard()) {
    Image* native_image = nullptr;
    Mask* native_mask = nullptr;
    Palette* native_palette = nullptr;
    Tileset* native_tileset = nullptr;
    getNativeBitmap(&native_image,
                    &native_mask,
                    &native_palette,
                    &native_tileset);
    if (native_image) {
      setData(native_image,
              native_mask,
              native_palette,
              native_tileset,
              false, false);
    }
  }
  if (m_data->palette && palette)
    m_data->palette->copyColorsTo(palette);
  return m_data->image;
}

bool Clipboard::getImageSize(gfx::Size& size)
{
  if (use_native_clipboard() && getNativeBitmapSize(&size))
    return true;

  if (m_data->image) {
    size.w = m_data->image->width();
    size.h = m_data->image->height();
    return true;
  }

  return false;
}

Palette* Clipboard::getPalette()
{
  if (format() == ClipboardFormat::PaletteEntries) {
    ASSERT(m_data->palette);
    return m_data->palette.get();
  }
  else
    return nullptr;
}

const PalettePicks& Clipboard::getPalettePicks()
{
  return m_data->picks;
}

} // namespace app
