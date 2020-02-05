// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
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
#include "app/commands/new_params.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "doc/palette.h"
#include "fmt/format.h"

#ifdef ENABLE_SCRIPTING
#include "app/script/luacpp.h"
#endif

namespace app {

enum class AddColorSource { Fg, Bg, Color };

template<>
void Param<AddColorSource>::fromString(const std::string& value)
{
  if (value == "fg" ||
      value == "foreground")
    setValue(AddColorSource::Fg);
  else if (value == "bg" ||
           value == "background")
    setValue(AddColorSource::Bg);
  else
    setValue(AddColorSource::Color);
}

#ifdef ENABLE_SCRIPTING
template<>
void Param<AddColorSource>::fromLua(lua_State* L, int index)
{
  fromString(lua_tostring(L, index));
}
#endif // ENABLE_SCRIPTING

struct AddColorParams : public NewParams {
  Param<AddColorSource> source { this, AddColorSource::Color, "source" };
  Param<app::Color> color { this, app::Color::fromMask(), "color" };
};

class AddColorCommand : public CommandWithNewParams<AddColorParams> {
public:
  AddColorCommand();
protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

AddColorCommand::AddColorCommand()
  : CommandWithNewParams<AddColorParams>(CommandId::AddColor(), CmdUIOnlyFlag)
{
}

bool AddColorCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void AddColorCommand::onExecute(Context* ctx)
{
  app::Color appColor;

  switch (params().source()) {
    case AddColorSource::Fg:
      appColor = Preferences::instance().colorBar.fgColor();
      break;
    case AddColorSource::Bg:
      appColor = Preferences::instance().colorBar.bgColor();
      break;
    case AddColorSource::Color:
      appColor = params().color();
      break;
  }

  Palette* pal = ctx->activeSite().palette();
  ASSERT(pal);
  if (!pal)
    return;

  try {
    std::unique_ptr<Palette> newPalette(new Palette(*pal));
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

    ContextWriter writer(ctx);
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
      tx(new cmd::SetPalette(sprite, frame, newPalette.get()));
      tx.commit();
    }
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

std::string AddColorCommand::onGetFriendlyName() const
{
  std::string source;
  switch (params().source()) {
    case AddColorSource::Fg: source = Strings::commands_AddColor_Foreground(); break;
    case AddColorSource::Bg: source = Strings::commands_AddColor_Background(); break;
    case AddColorSource::Color: source = Strings::commands_AddColor_Specific(); break;
  }
  return fmt::format(getBaseFriendlyName(), source);
}

Command* CommandFactory::createAddColorCommand()
{
  return new AddColorCommand;
}

} // namespace app
