// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
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
#include "app/pref/preferences.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/timeline/timeline.h"
#include "app/util/render_text.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "ui/manager.h"

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
  PasteTextWindow(const FontInfo& fontInfo,
                  const app::Color& color) {
    fontFace()->setInfo(fontInfo);
    fontColor()->setColor(color);
  }

  FontInfo fontInfo() const {
    return fontFace()->info();
  }

};

void PasteTextCommand::onExecute(Context* ctx)
{
  auto editor = Editor::activeEditor();
  if (editor == nullptr)
    return;

  Preferences& pref = Preferences::instance();
  FontInfo fontInfo = FontInfo::getFromPreferences();
  PasteTextWindow window(fontInfo, pref.colorBar.fgColor());

  window.userText()->setText(last_text_used);

  window.openWindowInForeground();
  if (window.closer() != window.ok())
    return;

  last_text_used = window.userText()->text();

  fontInfo = window.fontInfo();
  fontInfo.updatePreferences();

  try {
    std::string text = window.userText()->text();
    app::Color color = window.fontColor()->getColor();

    doc::ImageRef image = render_text(
      fontInfo, text,
      gfx::rgba(color.getRed(),
                color.getGreen(),
                color.getBlue(),
                color.getAlpha()));
    if (image) {
      Sprite* sprite = editor->sprite();
      if (image->pixelFormat() != sprite->pixelFormat()) {
        RgbMap* rgbmap = sprite->rgbMap(editor->frame());
        image.reset(
          render::convert_pixel_format(
            image.get(), NULL, sprite->pixelFormat(),
            render::Dithering(),
            rgbmap, sprite->palette(editor->frame()),
            false,
            sprite->transparentColor()));
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
