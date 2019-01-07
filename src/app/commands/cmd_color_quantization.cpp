// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_palette.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/job.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/sprite_job.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui_context.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"
#include "ui/manager.h"

#include "palette_from_sprite.xml.h"

namespace app {

class ColorQuantizationCommand : public Command {
public:
  ColorQuantizationCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ColorQuantizationCommand::ColorQuantizationCommand()
  : Command(CommandId::ColorQuantization(), CmdRecordableFlag)
{
}

bool ColorQuantizationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ColorQuantizationCommand::onExecute(Context* context)
{
  try {
    app::gen::PaletteFromSprite window;
    PalettePicks entries;

    Sprite* sprite;
    frame_t frame;
    Palette* curPalette;
    {
      ContextReader reader(context);
      Site site = context->activeSite();
      sprite = site.sprite();
      frame = site.frame();
      curPalette = sprite->palette(frame);

      window.newPalette()->setSelected(true);
      window.alphaChannel()->setSelected(
        App::instance()->preferences().quantization.withAlpha());
      window.ncolors()->setText("256");

      ColorBar::instance()->getPaletteView()->getSelectedEntries(entries);
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

    bool withAlpha = window.alphaChannel()->isSelected();
    App::instance()->preferences().quantization.withAlpha(withAlpha);

    bool createPal = false;
    if (window.newPalette()->isSelected()) {
      int n = window.ncolors()->textInt();
      n = MAX(1, n);
      entries = PalettePicks(n);
      entries.all();
      createPal = true;
    }
    else if (window.currentPalette()->isSelected()) {
      entries.all();
    }
    if (entries.picks() == 0)
      return;

    Palette tmpPalette(frame, entries.picks());

    ContextReader reader(context);
    SpriteJob job(reader, "Color Quantization");
    job.startJobWithCallback(
      [sprite, withAlpha, &tmpPalette, &job]{
        render::create_palette_from_sprite(
          sprite, 0, sprite->lastFrame(),
          withAlpha, &tmpPalette,
          &job);              // SpriteJob is a render::TaskDelegate
      });
    job.waitJob();
    if (job.isCanceled())
      return;

    std::unique_ptr<Palette> newPalette(
      new Palette(createPal ? tmpPalette:
                              *get_current_palette()));

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

    set_current_palette(newPalette.get(), false);
    ui::Manager::getDefault()->invalidate();
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
