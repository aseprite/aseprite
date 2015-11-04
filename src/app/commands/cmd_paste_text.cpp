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
#include "app/console.h"
#include "app/context.h"
#include "app/file_selector.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/font_popup.h"
#include "app/util/freetype_utils.h"
#include "base/bind.h"
#include "base/path.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "render/quantization.h"

#include "paste_text.xml.h"

namespace app {

static std::string last_text_used;

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
    fontFace()->Click.connect(Bind<void>(&PasteTextWindow::onSelectFontFile, this));
    fontFace()->DropDownClick.connect(Bind<void>(&PasteTextWindow::onSelectSystemFont, this));
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
    fontFace()->mainButton()
      ->setTextf("Select Font: %s",
                 base::get_file_title(m_face).c_str());
  }

  void onSelectFontFile() {
    std::string face = show_file_selector(
      "Select a TrueType Font",
      m_face,
      "ttf",
      FileSelectorType::Open,
      nullptr);

    if (!face.empty()) {
      setFontFace(face);
    }
  }

  void setFontFace(const std::string& face) {
    m_face = face;
    ok()->setEnabled(true);
    updateFontFaceButton();
  }

  void onSelectSystemFont() {
    if (!m_fontPopup) {
      try {
        m_fontPopup.reset(new FontPopup());
        m_fontPopup->Load.connect(&PasteTextWindow::setFontFace, this);
        m_fontPopup->Close.connect(Bind<void>(&PasteTextWindow::onCloseFontPopup, this));
      }
      catch (const std::exception& ex) {
        Console::showException(ex);
        return;
      }
    }

    if (!m_fontPopup->isVisible()) {
      gfx::Rect bounds = fontFace()->getBounds();
      m_fontPopup->showPopup(
        gfx::Rect(bounds.x, bounds.y+bounds.h,
                  ui::display_w()/2, ui::display_h()/2));
    }
    else {
      m_fontPopup->closeWindow(NULL);
    }
  }

  void onCloseFontPopup() {
    fontFace()->dropDown()->requestFocus();
  }

  std::string m_face;
  base::UniquePtr<FontPopup> m_fontPopup;
};

void PasteTextCommand::onExecute(Context* ctx)
{
  Editor* editor = current_editor;
  if (editor == NULL)
    return;

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
  size = MID(1, size, 999);
  pref.textTool.fontFace(faceName);
  pref.textTool.fontSize(size);

  try {
    std::string text = window.userText()->getText();
    app::Color appColor = window.fontColor()->getColor();
    doc::color_t color = doc::rgba(appColor.getRed(),
                                   appColor.getGreen(),
                                   appColor.getBlue(),
                                   appColor.getAlpha());

    doc::ImageRef image(render_text(faceName, size, text, color));
    if (image) {
      Sprite* sprite = editor->sprite();
      if (image->pixelFormat() != sprite->pixelFormat()) {
        RgbMap* rgbmap = sprite->rgbMap(editor->frame());
        image.reset(
          render::convert_pixel_format(
            image.get(), NULL, sprite->pixelFormat(),
            DitheringMethod::NONE, rgbmap, sprite->palette(editor->frame()),
            false, 0));
      }

      editor->pasteImage(image.get());
    }
  }
  catch (const std::exception& ex) {
    ui::Alert::show(PACKAGE
                    "<<%s"
                    "||&OK", ex.what());
  }
}

Command* CommandFactory::createPasteTextCommand()
{
  return new PasteTextCommand;
}

} // namespace app
