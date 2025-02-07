// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_palette.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "doc/palette.h"
#include "doc/sprite.h"

#include "palette_size.xml.h"

#include <limits>

namespace app {

class PaletteSizeCommand : public Command {
public:
  PaletteSizeCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  int m_size;
};

PaletteSizeCommand::PaletteSizeCommand() : Command(CommandId::PaletteSize(), CmdRecordableFlag)
{
  m_size = 0;
}

void PaletteSizeCommand::onLoadParams(const Params& params)
{
  m_size = params.get_as<int>("size");
}

bool PaletteSizeCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void PaletteSizeCommand::onExecute(Context* context)
{
  ContextReader reader(context);
  const frame_t frame = reader.frame();
  ASSERT(reader.palette());
  Palette palette(*reader.palette());
  int ncolors = (m_size != 0 ? m_size : palette.size());

  if (m_size == 0 && context->isUIAvailable()) {
    app::gen::PaletteSize window;
    window.colors()->setTextf("%d", ncolors);
    window.openWindowInForeground();
    if (window.closer() != window.ok())
      return;

    ncolors = window.colors()->textInt();
  }

  if (ncolors == palette.size())
    return;

  palette.resize(std::clamp(ncolors, 1, std::numeric_limits<int>::max()));

  ContextWriter writer(reader);
  Tx tx(writer, "Palette Size", ModifyDocument);
  tx(new cmd::SetPalette(writer.sprite(), frame, &palette));
  tx.commit();
}

Command* CommandFactory::createPaletteSizeCommand()
{
  return new PaletteSizeCommand;
}

} // namespace app
