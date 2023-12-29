// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CLIPBOARD_H_INCLUDED
#define APP_UTIL_CLIPBOARD_H_INCLUDED
#pragma once

#include "doc/cel_list.h"
#include "doc/image_ref.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/base.h"
#include "ui/clipboard_delegate.h"

#include <memory>

namespace doc {
  class Image;
  class Mask;
  class Palette;
  class PalettePicks;
  class Tileset;
}

namespace app {
  class Context;
  class ContextReader;
  class ContextWriter;
  class Doc;
  class DocRange;
  class Site;
  class Tx;

  enum class ClipboardFormat {
    None,
    Image,
    DocRange,
    PaletteEntries,
    Tilemap,
    Tileset,
  };

  class Clipboard : public ui::ClipboardDelegate {
  public:
    static Clipboard* instance();

    Clipboard();
    ~Clipboard();

    ClipboardFormat format() const;
    void getDocumentRangeInfo(Doc** document, DocRange* range);

    void clearMaskFromCels(Tx& tx,
                           Doc* doc,
                           const Site& site,
                           const doc::CelList& cels,
                           const bool deselectMask);

    void clearContent();
    void cut(ContextWriter& context);
    void copy(const ContextReader& context);
    void copyMerged(const ContextReader& context);
    void copyRange(const ContextReader& context, const DocRange& range);
    void copyImage(const doc::Image* image,
                   const doc::Mask* mask,
                   const doc::Palette* palette);
    void copyTilemap(const doc::Image* image,
                     const doc::Mask* mask,
                     const doc::Palette* pal,
                     const doc::Tileset* tileset);
    void copyPalette(const doc::Palette* palette,
                     const doc::PalettePicks& picks);
    void paste(Context* ctx,
               const bool interactive);

    doc::ImageRef getImage(doc::Palette* palette);

    // Returns true and fills the specified "size"" with the image's
    // size in the clipboard, or return false in case that the clipboard
    // doesn't contain an image at all.
    bool getImageSize(gfx::Size& size);

    doc::Palette* getPalette();
    const doc::PalettePicks& getPalettePicks();

    // ui::ClipboardDelegate impl
    void setClipboardText(const std::string& text) override;
    bool getClipboardText(std::string& text) override;

  private:
    void setData(doc::Image* image,
                 doc::Mask* mask,
                 doc::Palette* palette,
                 doc::Tileset* tileset,
                 bool set_native_clipboard,
                 bool image_source_is_transparent);
    bool copyFromDocument(const Site& site, bool merged = false);

    // Native clipboard
    void clearNativeContent();
    void registerNativeFormats();
    bool hasNativeBitmap() const;
    bool setNativeBitmap(const doc::Image* image,
                         const doc::Mask* mask,
                         const doc::Palette* palette,
                         const doc::Tileset* tileset = nullptr);
    bool getNativeBitmap(doc::Image** image,
                         doc::Mask** mask,
                         doc::Palette** palette,
                         doc::Tileset** tileset);
    bool getNativeBitmapSize(gfx::Size* size);

    struct Data;
    std::unique_ptr<Data> m_data;
  };

} // namespace app

#endif
