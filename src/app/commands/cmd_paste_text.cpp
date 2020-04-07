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
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/file_selector.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/font_popup.h"
#include "app/ui/timeline/timeline.h"
#include "app/util/freetype_utils.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "ui/system.h"

#include "paste_text.xml.h"

namespace app {

static std::string last_text_used;

class PasteTextCommand : public Command {
public:
  PasteTextCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

PasteTextCommand::PasteTextCommand()
  : Command(CommandId::PasteText(), CmdUIOnlyFlag)
{
}

bool PasteTextCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::ActiveLayerIsEditable);
}

class PasteTextWindow : public app::gen::PasteText {
public:
  PasteTextWindow(const std::string& face, int size,
                  bool antialias,
                  const app::Color& color)
    : m_face(face) {
    ok()->setEnabled(!m_face.empty());
    if (!m_face.empty())
      updateFontFaceButton();

    fontSize()->setTextf("%d", size);
    fontFace()->Click.connect(base::Bind<void>(&PasteTextWindow::onSelectFontFile, this));
    fontFace()->DropDownClick.connect(base::Bind<void>(&PasteTextWindow::onSelectSystemFont, this));
    fontColor()->setColor(color);
    this->antialias()->setSelected(antialias);
  }

  std::string faceValue() const {
    return m_face;
  }

  int sizeValue() const {
    int size = fontSize()->textInt();
    size = base::clamp(size, 1, 5000);
    return size;
  }

private:
  void updateFontFaceButton() {
    fontFace()->mainButton()
      ->setTextf("Select Font: %s",
                 base::get_file_title(m_face).c_str());
  }

  void onSelectFontFile() {
    base::paths exts = { "ttf", "ttc", "otf", "dfont" };
    base::paths face;
    if (!show_file_selector(
          "Select a TrueType Font",
          m_face, exts,
          FileSelectorType::Open, face))
      return;

    ASSERT(!face.empty());
    setFontFace(face.front());
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
        m_fontPopup->Close.connect(base::Bind<void>(&PasteTextWindow::onCloseFontPopup, this));
      }
      catch (const std::exception& ex) {
        Console::showException(ex);
        return;
      }
    }

    if (!m_fontPopup->isVisible()) {
      gfx::Rect bounds = fontFace()->bounds();
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
  std::unique_ptr<FontPopup> m_fontPopup;
};

void PasteTextCommand::onExecute(Context* ctx)
{
  Editor* editor = current_editor;
  if (editor == NULL)
    return;

  Preferences& pref = Preferences::instance();
  PasteTextWindow window(pref.textTool.fontFace(),
                         pref.textTool.fontSize(),
                         pref.textTool.antialias(),
                         pref.colorBar.fgColor());

  window.userText()->setText(last_text_used);

  window.openWindowInForeground();
  if (window.closer() != window.ok())
    return;

  last_text_used = window.userText()->text();

  bool antialias = window.antialias()->isSelected();
  std::string faceName = window.faceValue();
  int size = window.sizeValue();
  size = base::clamp(size, 1, 999);
  pref.textTool.fontFace(faceName);
  pref.textTool.fontSize(size);
  pref.textTool.antialias(antialias);

  try {
    std::string text = window.userText()->text();
    app::Color appColor = window.fontColor()->getColor();
    doc::color_t color = doc::rgba(appColor.getRed(),
                                   appColor.getGreen(),
                                   appColor.getBlue(),
                                   appColor.getAlpha());

    doc::ImageRef image(render_text(faceName, size, text, color, antialias));
    if (image) {
      Sprite* sprite = editor->sprite();
      if (image->pixelFormat() != sprite->pixelFormat()) {
        RgbMap* rgbmap = sprite->rgbMap(editor->frame());
        image.reset(
          render::convert_pixel_format(
            image.get(), NULL, sprite->pixelFormat(),
            render::Dithering(),
            rgbmap, sprite->palette(editor->frame()),
            false, 0));
      }

      // TODO we don't support pasting text in multiple cels at the
      //      moment, so we clear the range here (same as in
      //      clipboard::paste())
      if (auto timeline = App::instance()->timeline())
        timeline->clearAndInvalidateRange();

      editor->pasteImage(image.get());
    }
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
}

Command* CommandFactory::createPasteTextCommand()
{
  return new PasteTextCommand;
}

} // namespace app
