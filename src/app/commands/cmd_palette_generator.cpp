// Aseprite
// Copyright (C) 2026  Custom Build
//
// Palette Generator Command - Opens standalone palette generator dialog

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/palette_generator_popup.h"

namespace app {

class PaletteGeneratorCommand : public Command {
public:
    PaletteGeneratorCommand();

protected:
    bool onEnabled(Context* context) override;
    void onExecute(Context* context) override;
};

PaletteGeneratorCommand::PaletteGeneratorCommand()
    : Command(CommandId::PaletteGenerator())
{
}

bool PaletteGeneratorCommand::onEnabled(Context* context)
{
    return context->isUIAvailable();
}

void PaletteGeneratorCommand::onExecute(Context* context)
{
    PaletteGeneratorPopup popup;
    popup.openWindowInForeground();
}

Command* CommandFactory::createPaletteGeneratorCommand()
{
    return new PaletteGeneratorCommand;
}

} // namespace app
