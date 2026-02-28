// Aseprite
// Copyright (C) 2019-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/copy_region.h"
#include "app/cmd/patch_cel.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/new_params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/timeline/timeline.h"
#include "app/util/render_text.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "render/dithering.h"
#include "render/quantization.h"
#include "render/rasterize.h"
#include "render/render.h"
#include "ui/manager.h"

#include "paste_text.xml.h"

namespace app {

static std::string last_text_used;

struct PasteTextParams : public NewParams {
  Param<bool> ui{ this, true, "ui" };
  Param<app::Color> color{ this, app::Color::fromMask(), "color" };
  Param<std::string> text{ this, "", "text" };
  Param<std::string> fontName{ this, "Aseprite", "fontName" };
  Param<double> fontSize{ this, 6, "fontSize" };
  Param<int> x{ this, 0, "x" };
  Param<int> y{ this, 0, "y" };
};

class PasteTextCommand : public CommandWithNewParams<PasteTextParams> {
public:
  PasteTextCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

PasteTextCommand::PasteTextCommand() : CommandWithNewParams(CommandId::PasteText())
{
}

bool PasteTextCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::ActiveLayerIsEditable);
}

class PasteTextWindow : public app::gen::PasteText {
public:
  PasteTextWindow(const FontInfo& fontInfo, const app::Color& color)
  {
    fontFace()->setInfo(fontInfo, FontEntry::From::Init);
    fontColor()->setColor(color);
  }

  FontInfo fontInfo() const { return fontFace()->info(); }
};

void PasteTextCommand::onExecute(Context* ctx)
{
  const bool ui = params().ui() && ctx->isUIAvailable();

  FontInfo fontInfo = FontInfo::getFromPreferences();
  Preferences& pref = Preferences::instance();

  std::string text;
  app::Color color;
  ui::Paint paint;

  if (ui) {
    PasteTextWindow window(fontInfo, pref.colorBar.fgColor());

    window.userText()->setText(params().text().empty() ? last_text_used : params().text());

    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    text = window.userText()->text();
    last_text_used = text;
    color = window.fontColor()->getColor();
    paint = window.fontFace()->paint();

    fontInfo = window.fontInfo();
    fontInfo.updatePreferences();
  }
  else {
    text = params().text();
    color = params().color.isSet() ? params().color() : pref.colorBar.fgColor();

    FontInfo info(FontInfo::Type::Unknown, params().fontName(), params().fontSize());
    fontInfo = info;
  }

  try {
    paint.color(color_utils::color_for_ui(color));

    doc::ImageRef image = render_text(fontInfo, text, paint);
    if (!image)
      return;

    auto site = ctx->activeSite();
    Sprite* sprite = site.sprite();
    if (image->pixelFormat() != sprite->pixelFormat()) {
      RgbMap* rgbmap = sprite->rgbMap(site.frame());
      image.reset(render::convert_pixel_format(image.get(),
                                               NULL,
                                               sprite->pixelFormat(),
                                               render::Dithering(),
                                               rgbmap,
                                               sprite->palette(site.frame()),
                                               false,
                                               sprite->transparentColor()));
    }

    // TODO we don't support pasting text in multiple cels at the
    //      moment, so we clear the range here (same as in
    //      clipboard::paste())
    if (auto timeline = App::instance()->timeline())
      timeline->clearAndInvalidateRange();

    auto point = sprite->bounds().center() - gfx::Point(image->size().w / 2, image->size().h / 2);
    if (params().x.isSet())
      point.x = params().x();
    if (params().y.isSet())
      point.y = params().y();

    if (ui) {
      // TODO: Do we want to make this selectable result available when not using UI?
      Editor::activeEditor()->pasteImage(
        image.get(), nullptr, (params().x.isSet() || params().y.isSet()) ? &point : nullptr);
      return;
    }

    ContextWriter writer(ctx);
    Tx tx(writer, "Paste Text");
    ImageRef finalImage = image;
    if (writer.cel()->image()) {
      gfx::Rect celRect(point, image->size());
      ASSERT(!celRect.isEmpty() && celRect.x >= 0 && celRect.y >= 0);
      finalImage.reset(
        doc::crop_image(writer.cel()->image(), celRect, writer.cel()->image()->maskColor()));
      render::Render render;
      render.setNewBlend(pref.experimental.newBlend());
      render.setBgOptions(render::BgOptions::MakeTransparent());
      render.renderImage(finalImage.get(),
                         image.get(),
                         writer.palette(),
                         0,
                         0,
                         255,
                         doc::BlendMode::NORMAL);
    }

    tx(new cmd::CopyRegion(writer.cel()->image(),
                           finalImage.get(),
                           gfx::Region(finalImage->bounds()),
                           point));
    tx.commit();
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
