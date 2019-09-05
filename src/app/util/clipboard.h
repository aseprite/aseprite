// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_CLIPBOARD_H_INCLUDED
#define APP_UTIL_CLIPBOARD_H_INCLUDED
#pragma once

#include "doc/cel_list.h"
#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/base.h"
#include "ui/clipboard_delegate.h"

namespace doc {
  class Image;
  class Mask;
  class Palette;
  class PalettePicks;
}

namespace app {
  class Context;
  class ContextReader;
  class ContextWriter;
  class Doc;
  class DocRange;
  class Tx;

  namespace clipboard {
    using namespace doc;

    enum ClipboardFormat {
      ClipboardNone,
      ClipboardImage,
      ClipboardDocRange,
      ClipboardPaletteEntries,
    };

    // TODO Horrible API: refactor it (maybe a merge with os::clipboard).

    class ClipboardManager : public ui::ClipboardDelegate {
    public:
      static ClipboardManager* instance();

      ClipboardManager();
      ~ClipboardManager();

      void setClipboardText(const std::string& text) override;
      bool getClipboardText(std::string& text) override;
    private:
      std::string m_text; // Text used when the native clipboard is disabled
    };

    ClipboardFormat get_current_format();
    void get_document_range_info(Doc** document, DocRange* range);

    void clear_mask_from_cels(Tx& tx,
                              Doc* doc,
                              const doc::CelList& cels,
                              const bool deselectMask);

    void clear_content();
    void cut(ContextWriter& context);
    void copy(const ContextReader& context);
    void copy_merged(const ContextReader& context);
    void copy_range(const ContextReader& context, const DocRange& range);
    void copy_image(const Image* image, const Mask* mask, const Palette* palette);
    void copy_palette(const Palette* palette, const PalettePicks& picks);
    void paste(Context* ctx, const bool interactive);

    ImageRef get_image(Palette* palette);

    // Returns true and fills the specified "size"" with the image's
    // size in the clipboard, or return false in case that the clipboard
    // doesn't contain an image at all.
    bool get_image_size(gfx::Size& size);

    Palette* get_palette();
    const PalettePicks& get_palette_picks();

  } // namespace clipboard
} // namespace app

#endif
