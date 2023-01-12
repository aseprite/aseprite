// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
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
#include "app/ui/rgbmap_algorithm_selector.h"
#include "app/ui_context.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"
#include "ui/manager.h"

#include "palette_from_sprite.xml.h"

#include <algorithm>

namespace app {

struct ColorQuantizationParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<bool> withAlpha { this, true, "withAlpha" };
  Param<int> maxColors { this, 256, "maxColors" };
  Param<bool> useRange { this, false, "useRange" };
  Param<RgbMapAlgorithm> algorithm { this, RgbMapAlgorithm::DEFAULT, "algorithm" };
};

#if ENABLE_UI

class PaletteFromSpriteWindow : public app::gen::PaletteFromSprite {
public:
  PaletteFromSpriteWindow() {
    rgbmapAlgorithmPlaceholder()->addChild(&m_algoSelector);

    advancedCheck()->Click.connect(
      [this](){
        advanced()->setVisible(advancedCheck()->isSelected());
        expandWindow(sizeHint());
      });

  }

  doc::RgbMapAlgorithm algorithm() {
    return m_algoSelector.algorithm();
  }

  void algorithm(const doc::RgbMapAlgorithm mapAlgo) {
    m_algoSelector.algorithm(mapAlgo);
  }

private:
  RgbMapAlgorithmSelector m_algoSelector;
};

#endif

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

  auto& pref = Preferences::instance();
  bool withAlpha = params().withAlpha();
  int maxColors = params().maxColors();
  RgbMapAlgorithm algorithm = params().algorithm();
  bool createPal;

  Site site = ctx->activeSite();
  PalettePicks entries = site.selectedColors();

#ifdef ENABLE_UI
  if (ui) {
    PaletteFromSpriteWindow window;
    {
      ContextReader reader(ctx);
      const Palette* curPalette = site.sprite()->palette(site.frame());

      if (!params().algorithm.isSet())
        algorithm = pref.quantization.rgbmapAlgorithm();
      if (!params().withAlpha.isSet())
        withAlpha = pref.quantization.withAlpha();

      const bool advanced = pref.quantization.advanced();
      window.advancedCheck()->setSelected(advanced);
      window.advanced()->setVisible(advanced);

      window.algorithm(algorithm);
      window.newPalette()->setSelected(true);
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
    algorithm = window.algorithm();

    pref.quantization.withAlpha(withAlpha);
    pref.quantization.advanced(window.advancedCheck()->isSelected());

    if (window.newPalette()->isSelected()) {
      createPal = true;
    }
    else {
      createPal = false;
      if (window.currentPalette()->isSelected()) {
        entries = PalettePicks(site.palette()->size());
        entries.all();
      }
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
    const bool newBlend = pref.experimental.newBlend();
    job.startJobWithCallback(
      [sprite, withAlpha, &tmpPalette, &job, newBlend, algorithm]{
        render::create_palette_from_sprite(
          sprite, 0, sprite->lastFrame(),
          withAlpha, &tmpPalette,
          &job,                 // SpriteJob is a render::TaskDelegate
          newBlend,
          algorithm);
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
