// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_palette.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui_context.h"
#include "base/unique_ptr.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "render/quantization.h"
#include "ui/manager.h"

namespace app {

class ColorQuantizationCommand : public Command {
public:
  ColorQuantizationCommand();
  Command* clone() const override { return new ColorQuantizationCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ColorQuantizationCommand::ColorQuantizationCommand()
  : Command("ColorQuantization",
            "Color Quantization",
            CmdRecordableFlag)
{
}

bool ColorQuantizationCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Sprite* sprite(writer.sprite());
  return (sprite && sprite->pixelFormat() == IMAGE_RGB);
}

void ColorQuantizationCommand::onExecute(Context* context)
{
  try {
    ContextWriter writer(UIContext::instance(), 500);
    Sprite* sprite = writer.sprite();
    frame_t frame = writer.frame();
    if (sprite) {
      PaletteView::SelectedEntries entries;
      ColorBar::instance()->getPaletteView()->getSelectedEntries(entries);

      // TODO convert SelectedEntries into a class, and put the
      // following lines in that class it is used several times (here
      // and in ColorBar)

      int n = 0;
      for (bool state : entries) {
        if (state)
          ++n;
      }

      if (n < 2) {
        n = 0;
        for (auto& state : entries) {
          state = true;
          ++n;
        }
      }

      Palette palette(frame, n);
      render::create_palette_from_rgb(sprite, frame, &palette);

      Palette newPalette(*get_current_palette());

      int i = 0, j = 0;
      for (bool state : entries) {
        if (state)
          newPalette.setEntry(i, palette.getEntry(j++));
        ++i;
      }

      Transaction transaction(writer.context(), "Color Quantization", ModifyDocument);
      transaction.execute(new cmd::SetPalette(sprite, frame, &newPalette));
      transaction.commit();

      set_current_palette(&newPalette, false);
      ui::Manager::getDefault()->invalidate();
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

Command* CommandFactory::createColorQuantizationCommand()
{
  return new ColorQuantizationCommand;
}

} // namespace app
