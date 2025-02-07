// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/timeline/timeline.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/sprite.h"

#include <string>

namespace app {

class CelOpacityCommand : public Command {
public:
  CelOpacityCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  int m_opacity;
};

CelOpacityCommand::CelOpacityCommand() : Command(CommandId::CelOpacity(), CmdUIOnlyFlag)
{
  m_opacity = 255;
}

void CelOpacityCommand::onLoadParams(const Params& params)
{
  m_opacity = params.get_as<int>("opacity");
  m_opacity = std::clamp(m_opacity, 0, 255);
}

bool CelOpacityCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveCel);
}

void CelOpacityCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Layer* layer = writer.layer();
  Cel* cel = writer.cel();
  if (!cel || layer->isBackground() || !layer->isEditable() || cel->opacity() == m_opacity)
    return;

  {
    Tx tx(writer, "Set Cel Opacity");

    // TODO the range of selected cels should be in app::Site.
    DocRange range;

    if (context->isUIAvailable())
      range = App::instance()->timeline()->range();

    if (!range.enabled()) {
      range.startRange(layer, cel->frame(), DocRange::kCels);
      range.endRange(layer, cel->frame());
    }

    for (Cel* c : cel->sprite()->uniqueCels(range.selectedFrames())) {
      if (range.contains(c->layer())) {
        if (!c->layer()->isBackground() && c->layer()->isEditable() && m_opacity != c->opacity()) {
          tx(new cmd::SetCelOpacity(c, m_opacity));
        }
      }
    }

    tx.commit();
  }

  update_screen_for_document(writer.document());
}

std::string CelOpacityCommand::onGetFriendlyName() const
{
  return Strings::commands_CelOpacity(m_opacity, int(100.0 * m_opacity / 255.0));
}

Command* CommandFactory::createCelOpacityCommand()
{
  return new CelOpacityCommand;
}

} // namespace app
