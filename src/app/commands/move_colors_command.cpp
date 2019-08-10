// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/cmd_set_palette.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/util/pal_ops.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"

namespace app {

using namespace ui;

struct MoveColorsParams : public NewParams {
  Param<int> before { this, 0, "before" };
};

class MoveColorsCommand : public CommandWithNewParams<MoveColorsParams> {
public:
  MoveColorsCommand(bool copy)
    : CommandWithNewParams<MoveColorsParams>(
        (copy ? CommandId::CopyColors():
                CommandId::MoveColors()), CmdRecordableFlag),
      m_copy(copy) { }

protected:
  bool onEnabled(Context* ctx) override {
    return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                           ContextFlags::HasSelectedColors);
  }

  void onExecute(Context* ctx) override {
    ContextWriter writer(ctx);
    Site site = ctx->activeSite();

    PalettePicks picks = site.selectedColors();
    if (picks.picks() == 0)
      return;                   // Do nothing

    ASSERT(writer.palette());
    if (!writer.palette())
      return;

    Tx tx(writer.context(), friendlyName(), ModifyDocument);
    const int beforeIndex = params().before();
    int currentEntry = picks.firstPick();

#ifdef ENABLE_UI
    if (ctx->isUIAvailable()) {
      auto& fgColor = Preferences::instance().colorBar.fgColor;
      if (fgColor().getType() == app::Color::IndexType)
        currentEntry = fgColor().getIndex();
    }
#endif

    doc::Palette palette(*writer.palette());
    doc::Palette newPalette(palette);
    move_or_copy_palette_colors(palette, newPalette, picks,
                                currentEntry,
                                beforeIndex,
                                m_copy);

    writer.document()->getApi(tx)
      .setPalette(writer.sprite(), writer.frame(), &newPalette);

    ctx->setSelectedColors(picks);

#ifdef ENABLE_UI
    if (ctx->isUIAvailable()) {
      auto& fgColor = Preferences::instance().colorBar.fgColor;
      if (fgColor().getType() == app::Color::IndexType)
        fgColor(Color::fromIndex(currentEntry));
    }
#endif

    tx.commit();
  }

private:
  bool m_copy;
};

Command* CommandFactory::createMoveColorsCommand()
{
  return new MoveColorsCommand(false);
}

Command* CommandFactory::createCopyColorsCommand()
{
  return new MoveColorsCommand(true);
}

} // namespace app
