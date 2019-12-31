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
#include "app/i18n/strings.h"
#include "app/job.h"
#include "app/pref/preferences.h"
#include "app/sprite_job.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"

#include "palette_from_sprite.xml.h"

#include <algorithm>

namespace app {

#if ENABLE_UI
class PaletteFromSpriteWindow : public app::gen::PaletteFromSprite {
public:
  PaletteFromSpriteWindow(Site& site,
                          PalettePicks& entries,
                          Preferences& pref)
    : m_specificNumber(true)
    , m_colorLimit(256)
    , m_replacePalette(false)
    , m_replaceCurrentRange(false)
    , m_withAlpha(pref.quantization.withAlpha())
    , m_mapAlgorithm(pref.quantization.algorithm())

  {
    algorithm()->Change.connect(base::Bind<void>(&PaletteFromSpriteWindow::onChangeMapAlgorithm, this));
    algorithm()->setSelectedItemIndex((int)m_mapAlgorithm);

    if (site.document()->sprite()->pixelFormat() == PixelFormat::IMAGE_RGB) {
      if (m_mapAlgorithm == MapAlgorithm::OCTREE) {
        alphaChannel()->setSelected(false);
        alphaChannel()->setEnabled(false);
      }
      else {
        alphaChannel()->setEnabled(true);
        alphaChannel()->setSelected(m_withAlpha);
      }
    }
    else {
      algorithm()->setSelectedItemIndex((int)MapAlgorithm::RGB5A3);
      algorithm()->setEnabled(false);
    }

  }

  void onChangeMapAlgorithm() {
    MapAlgorithm selectedAlgo = (MapAlgorithm)algorithm()->getSelectedItemIndex();
    if (selectedAlgo == MapAlgorithm::OCTREE) {
      m_mapAlgorithm = MapAlgorithm::OCTREE;
      alphaChannel()->setSelected(false);
      alphaChannel()->setEnabled(false);
    }
    else {
      m_mapAlgorithm = MapAlgorithm::RGB5A3;
      alphaChannel()->setEnabled(true);
    }
  }

  bool m_specificNumber;
  int m_colorLimit = 256;
  bool m_replacePalette;
  bool m_replaceCurrentRange;
  bool m_withAlpha;
  MapAlgorithm m_mapAlgorithm;
};
#endif

struct ColorQuantizationParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<bool> withAlpha { this, true, "withAlpha" };
  Param<int> maxColors { this, 256, "maxColors" };
  Param<bool> useRange { this, false, "useRange" };
  Param<MapAlgorithm> algorithm { this, MapAlgorithm::DEFAULT, "algorithm" };
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
  MapAlgorithm algorithm = params().algorithm();
  bool createPal;

  Site site = ctx->activeSite();
  PalettePicks entries = site.selectedColors();

#ifdef ENABLE_UI
  if (ui) {
    auto& pref = Preferences::instance();
    PaletteFromSpriteWindow window(site, entries, pref);
    {
      ContextReader reader(ctx);
      const Palette* curPalette = site.sprite()->palette(site.frame());

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

    pref.quantization.withAlpha(window.alphaChannel()->isSelected());
    algorithm = MapAlgorithm(window.algorithm()->getSelectedItemIndex());
    pref.quantization.algorithm(algorithm);
    pref.quantization.save();

    if (window.closer() != window.ok())
      return;

    maxColors = window.ncolors()->textInt();

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
      [sprite, withAlpha, &tmpPalette, &job, newBlend, algorithm]{
        render::create_palette_from_sprite(
          sprite, 0, sprite->lastFrame(),
          withAlpha, &tmpPalette,
          &job,
          newBlend,
          algorithm);     // SpriteJob is a render::TaskDelegate
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
