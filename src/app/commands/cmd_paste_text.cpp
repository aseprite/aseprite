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
#include "app/commands/command.h"
#include "app/context.h"
#include "app/file_selector.h"
#include "app/pref/preferences.h"
#include "app/util/clipboard.h"
#include "base/bind.h"
#include "base/path.h"
#include "base/string.h"
#include "doc/blend_funcs.h"
#include "doc/blend_internals.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/primitives.h"

#include "freetype/ftglyph.h"
#include "ft2build.h"
#include FT_FREETYPE_H

#include "paste_text.xml.h"

namespace app {

static std::string last_text_used;

template<typename Iterator, typename Func>
static void for_each_glyph(FT_Face face, Iterator first, Iterator end, Func callback)
{
  bool use_kerning = (FT_HAS_KERNING(face) ? true: false);

  // Calculate size
  FT_UInt prev_glyph = 0;
  int x = 0;
  for (; first != end; ++first) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, *first);

    if (use_kerning && prev_glyph && glyph_index) {
      FT_Vector kerning;
      FT_Get_Kerning(face, prev_glyph, glyph_index,
                     FT_KERNING_DEFAULT, &kerning);
      x += kerning.x >> 6;
    }

    FT_Error err = FT_Load_Glyph(
      face, glyph_index,
      FT_LOAD_RENDER | FT_LOAD_NO_BITMAP);

    if (!err) {
      callback(x, face->glyph);
      x += face->glyph->advance.x >> 6;
    }

    prev_glyph = glyph_index;
  }
}

class PasteTextCommand : public Command {
public:
  PasteTextCommand();
  Command* clone() const override { return new PasteTextCommand(*this); }

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

PasteTextCommand::PasteTextCommand()
  : Command("PasteText",
            "Insert Text",
            CmdUIOnlyFlag)
{
}

bool PasteTextCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

class PasteTextWindow : public app::gen::PasteText {
public:
  PasteTextWindow(const std::string& face, int size,
                  const app::Color& color)
    : m_face(face) {
    ok()->setEnabled(!m_face.empty());
    if (!m_face.empty())
      updateFontFaceButton();

    fontSize()->setTextf("%d", size);
    fontFace()->Click.connect(Bind<void>(&PasteTextWindow::onSelectFont, this));
    fontColor()->setColor(color);
  }

  std::string faceValue() const {
    return m_face;
  }

  int sizeValue() const {
    int size = fontSize()->getTextInt();
    size = MID(1, size, 5000);
    return size;
  }

private:
  void updateFontFaceButton() {
    fontFace()->setTextf("Select Font: %s",
                         base::get_file_name(m_face).c_str());
  }

  void onSelectFont() {
    std::string face = show_file_selector(
      "Select a TrueType Font",
      m_face,
      "ttf",
      FileSelectorType::Open,
      nullptr);

    if (!face.empty()) {
      m_face = face;
      ok()->setEnabled(true);
      updateFontFaceButton();
    }
  }

  std::string m_face;
};

void PasteTextCommand::onExecute(Context* ctx)
{
  Preferences& pref = Preferences::instance();
  PasteTextWindow window(pref.textTool.fontFace(),
                         pref.textTool.fontSize(),
                         pref.colorBar.fgColor());

  window.userText()->setText(last_text_used);

  window.openWindowInForeground();
  if (window.getKiller() != window.ok())
    return;

  last_text_used = window.userText()->getText();

  std::string faceName = window.faceValue();
  int size = window.sizeValue();
  pref.textTool.fontFace(faceName);
  pref.textTool.fontSize(size);

  FT_Library ft;
  FT_Init_FreeType(&ft);

  FT_Open_Args args;
  memset(&args, 0, sizeof(args));
  args.flags = FT_OPEN_PATHNAME;
  args.pathname = (FT_String*)faceName.c_str();

  FT_Face face;
  FT_Error err = FT_Open_Face(ft, &args, 0, &face);
  if (!err) {
    std::string text = window.userText()->getText();
    app::Color appColor = window.fontColor()->getColor();
    doc::color_t color = doc::rgba(appColor.getRed(),
                                   appColor.getGreen(),
                                   appColor.getBlue(),
                                   appColor.getAlpha());

    // Set font size
    FT_Set_Pixel_Sizes(face, size, size);

    // Calculate text size
    base::utf8_iterator begin(text.begin()), end(text.end());
    gfx::Rect bounds(0, 0, 0, 0);
    for_each_glyph(
      face, begin, end,
      [&bounds](int x, FT_GlyphSlot glyph) {
        bounds |= gfx::Rect(x + glyph->bitmap_left,
                            -glyph->bitmap_top,
                            (int)glyph->bitmap.width,
                            (int)glyph->bitmap.rows);
      });

    // Render the image and copy it to the clipboard
    if (!bounds.isEmpty()) {
      doc::Image* image = doc::Image::create(IMAGE_RGB, bounds.w, bounds.h);
      doc::clear_image(image, 0);

      for_each_glyph(
        face, begin, end,
        [&bounds, &image, color](int x, FT_GlyphSlot glyph) {
          int t, yimg = - bounds.y - glyph->bitmap_top;
          for (int v=0; v<(int)glyph->bitmap.rows; ++v, ++yimg) {
            const uint8_t* p = glyph->bitmap.buffer + v*glyph->bitmap.pitch;
            int ximg = x - bounds.x + glyph->bitmap_left;
            for (int u=0; u<(int)glyph->bitmap.width; ++u, ++p, ++ximg) {
              int alpha = *p;
              doc::put_pixel(
                image, ximg, yimg,
                doc::rgba_blender_normal(
                  doc::get_pixel(image, ximg, yimg),
                  doc::rgba(doc::rgba_getr(color),
                            doc::rgba_getg(color),
                            doc::rgba_getb(color),
                            MUL_UN8(doc::rgba_geta(color), *p, t)), 255));
            }
          }
        });

      clipboard::copy_image(image, nullptr, nullptr);
      clipboard::paste();
    }
    else {
      ui::Alert::show(PACKAGE
                      "<<There is no text"
                      "||&OK");
    }

    FT_Done_Face(face);
  }
  else {
    ui::Alert::show(PACKAGE
                    "<<Error loading font face"
                    "||&OK");
  }

  FT_Done_FreeType(ft);
}

Command* CommandFactory::createPasteTextCommand()
{
  return new PasteTextCommand;
}

} // namespace app
