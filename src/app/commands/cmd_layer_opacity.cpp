// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/timeline/timeline.h"
#include "doc/layer.h"
#include "fmt/format.h"

#include <string>

namespace app {

class LayerOpacityCommand : public Command {
public:
  LayerOpacityCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  int m_opacity;
};

LayerOpacityCommand::LayerOpacityCommand()
  : Command(CommandId::LayerOpacity(), CmdUIOnlyFlag)
{
  m_opacity = 255;
}

void LayerOpacityCommand::onLoadParams(const Params& params)
{
  m_opacity = params.get_as<int>("opacity");
  m_opacity = MID(0, m_opacity, 255);
}

bool LayerOpacityCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

void LayerOpacityCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Layer* layer = writer.layer();
  if (!layer ||
      !layer->isImage() ||
      static_cast<LayerImage*>(layer)->opacity() == m_opacity)
    return;

  {
    Tx tx(writer.context(), "Set Layer Opacity");

    // TODO the range of selected frames should be in app::Site.
    SelectedLayers selLayers;
    auto range = App::instance()->timeline()->range();
    if (range.enabled()) {
      selLayers = range.selectedLayers();
    }
    else {
      selLayers.insert(writer.layer());
    }

    for (auto layer : selLayers) {
      if (layer->isImage())
        tx(new cmd::SetLayerOpacity(static_cast<LayerImage*>(layer), m_opacity));
    }

    tx.commit();
  }

  update_screen_for_document(writer.document());
}

std::string LayerOpacityCommand::onGetFriendlyName() const
{
  return fmt::format(getBaseFriendlyName(),
                     m_opacity,
                     int(100.0 * m_opacity / 255.0));
}

Command* CommandFactory::createLayerOpacityCommand()
{
  return new LayerOpacityCommand;
}

} // namespace app
