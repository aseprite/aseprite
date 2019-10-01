// Aseprite
// Copyright (C) 2019 Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_palette.h"
#include "app/commands/new_params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/job.h"
#include "app/pref/preferences.h"
#include "app/sprite_job.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui_context.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"

#include "palette_from_sprite.xml.h"

#include <algorithm>

namespace app {

struct ColorQuantizationParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<bool> withAlpha { this, true, "withAlpha" };
  Param<int> maxColors { this, 256, "maxColors" };
  Param<bool> useRange { this, false, "useRange" };
};

class ColorQuantizationCommand : public CommandWithNewParams<ColorQuantizationParams> {
public:
  ColorQuantizationCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ColorQuantizationCommand::ColorQuantizationCommand()
  : CommandWithNewParams<ColorQuantizationParams>(
      CommandId::ColorQuantization(),
      CmdRecordableFlag)
{
}

bool ColorQuantizationCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ColorQuantizationCommand::onExecute(Context* ctx)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && ctx->isUIAvailable());
#endif

  bool withAlpha = params().withAlpha();
  int maxColors = params().maxColors();
  bool createPal;

  Site site = ctx->activeSite();
  PalettePicks entries = site.selectedColors();

#ifdef ENABLE_UI
  if (ui) {
    app::gen::PaletteFromSprite window;
    {
      ContextReader reader(ctx);
      const Palette* curPalette = site.sprite()->palette(site.frame());

      if (!params().withAlpha.isSet())
        withAlpha = App::instance()->preferences().quantization.withAlpha();

      window.newPalette()->setSelected(true);
      window.alphaChannel()->setSelected(withAlpha);
      window.ncolors()->setTextf("%d", maxColors);

      if (entries.picks() > 1) {
        window.currentRange()->setTextf(
          "%s, %d color(s)",
          window.currentRange()->text().c_str(),
          entries.picks());
      }
      else
        window.currentRange()->setEnabled(false);

      window.currentPalette()->setTextf(
        "%s, %d color(s)",
        window.currentPalette()->text().c_str(),
        curPalette->size());
    }

    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    maxColors = window.ncolors()->textInt();
    withAlpha = window.alphaChannel()->isSelected();
    App::instance()->preferences().quantization.withAlpha(withAlpha);

    if (window.newPalette()->isSelected()) {
      createPal = true;
    }
    else {
      createPal = false;
      if (window.currentPalette()->isSelected())
        entries.all();
    }
  }
  else
#endif // ENABLE_UI
  {
    createPal = (!params().useRange());
  }

  if (createPal) {
    entries = PalettePicks(std::max(1, maxColors));
    entries.all();
  }
  if (entries.picks() == 0)
    return;

  try {
    ContextReader reader(ctx);
    Sprite* sprite = site.sprite();
    frame_t frame = site.frame();
    const Palette* curPalette = site.sprite()->palette(frame);
    Palette tmpPalette(frame, entries.picks());

    SpriteJob job(reader, "Color Quantization");
    const bool newBlend = Preferences::instance().experimental.newBlend();
    job.startJobWithCallback(
      [sprite, withAlpha, &tmpPalette, &job, newBlend]{
        render::create_palette_from_sprite(
          sprite, 0, sprite->lastFrame(),
          withAlpha, &tmpPalette,
          &job,
          newBlend);     // SpriteJob is a render::TaskDelegate
      });
    job.waitJob();
    if (job.isCanceled())
      return;

    std::unique_ptr<Palette> newPalette(
      new Palette(createPal ? tmpPalette:
                              *site.palette()));

    if (createPal) {
      entries = PalettePicks(newPalette->size());
      entries.all();
    }

    int i = 0, j = 0;
    for (bool state : entries) {
      if (state)
        newPalette->setEntry(i, tmpPalette.getEntry(j++));
      ++i;
    }

    if (*curPalette != *newPalette)
      job.tx()(new cmd::SetPalette(sprite, frame, newPalette.get()));
  }
  catch (const base::Exception& e) {
    Console::showException(e);
  }
}

Command* CommandFactory::createColorQuantizationCommand()
{
  return new ColorQuantizationCommand;
}

} // namespace app
