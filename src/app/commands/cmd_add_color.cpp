// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_palette.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "doc/palette.h"
#include "fmt/format.h"
#include "ui/manager.h"

namespace app {

class AddColorCommand : public Command {
public:
  enum class Source { Fg, Bg, Color };

  AddColorCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;

  Source m_source;
  app::Color m_color;
};

AddColorCommand::AddColorCommand()
  : Command(CommandId::AddColor(), CmdUIOnlyFlag)
  , m_source(Source::Fg)
{
}

void AddColorCommand::onLoadParams(const Params& params)
{
  std::string source = params.get("source");
  if (source == "fg")
    m_source = Source::Fg;
  else if (source == "bg")
    m_source = Source::Bg;
  else
    m_source = Source::Color;

  if (m_source == Source::Color)
    m_color = app::Color::fromString(params.get("color"));
  else
    m_color = app::Color::fromMask();
}

bool AddColorCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void AddColorCommand::onExecute(Context* ctx)
{
  app::Color appColor;

  switch (m_source) {
    case Source::Fg:
      appColor = ColorBar::instance()->getFgColor();
      break;
    case Source::Bg:
      appColor = ColorBar::instance()->getBgColor();
      break;
    case Source::Color:
      appColor = m_color;
      break;
  }

  try {
    Palette* newPalette = get_current_palette(); // System current pal
    color_t color = doc::rgba(
      appColor.getRed(),
      appColor.getGreen(),
      appColor.getBlue(),
      appColor.getAlpha());
    int index = newPalette->findExactMatch(
      appColor.getRed(),
      appColor.getGreen(),
      appColor.getBlue(),
      appColor.getAlpha(), -1);

    // It should be -1, because the user has pressed the warning
    // button that is available only when the color isn't in the
    // palette.
    ASSERT(index < 0);
    if (index >= 0)
      return;

    ContextWriter writer(ctx, 500);
    Doc* document(writer.document());
    Sprite* sprite = writer.sprite();
    if (!document || !sprite) {
      ASSERT(false);
      return;
    }

    newPalette->addEntry(color);
    index = newPalette->size()-1;

    if (document) {
      frame_t frame = writer.frame();

      Tx tx(writer.context(), friendlyName(), ModifyDocument);
      tx(new cmd::SetPalette(sprite, frame, newPalette));
      tx.commit();
    }

    set_current_palette(newPalette, true);
    ui::Manager::getDefault()->invalidate();
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

std::string AddColorCommand::onGetFriendlyName() const
{
  std::string source;
  switch (m_source) {
    case Source::Fg: source = Strings::commands_AddColor_Foreground(); break;
    case Source::Bg: source = Strings::commands_AddColor_Background(); break;
    case Source::Color: source = Strings::commands_AddColor_Specific(); break;
  }
  return fmt::format(getBaseFriendlyName(), source);
}

Command* CommandFactory::createAddColorCommand()
{
  return new AddColorCommand;
}

} // namespace app
