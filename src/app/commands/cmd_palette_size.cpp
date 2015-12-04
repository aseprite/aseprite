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
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/modules/palettes.h"
#include "app/transaction.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include "palette_size.xml.h"

#include <climits>

namespace app {

class PaletteSizeCommand : public Command {
public:
  PaletteSizeCommand();
  Command* clone() const override { return new PaletteSizeCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  int m_size;
};

PaletteSizeCommand::PaletteSizeCommand()
  : Command("PaletteSize",
            "Palette Size",
            CmdRecordableFlag)
{
  m_size = 0;
}

void PaletteSizeCommand::onLoadParams(const Params& params)
{
  m_size = params.get_as<int>("size");
}

void PaletteSizeCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Sprite* sprite = writer.sprite();
  frame_t frame = writer.frame();
  Palette palette(*sprite->palette(frame));

  app::gen::PaletteSize window;
  window.colors()->setTextf("%d", palette.size());
  window.openWindowInForeground();
  if (window.closer() == window.ok()) {
    int ncolors = window.colors()->textInt();
    if (ncolors == palette.size())
      return;

    palette.resize(MID(1, ncolors, INT_MAX));

    Transaction transaction(context, "Palette Size", ModifyDocument);
    transaction.execute(new cmd::SetPalette(sprite, frame, &palette));
    transaction.commit();

    set_current_palette(&palette, false);
    ui::Manager::getDefault()->invalidate();
  }
}

Command* CommandFactory::createPaletteSizeCommand()
{
  return new PaletteSizeCommand;
}

} // namespace app
