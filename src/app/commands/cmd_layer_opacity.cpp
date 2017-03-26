// Aseprite
// Copyright (C) 2016-2017  David Capello
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
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/timeline/timeline.h"
#include "base/convert_to.h"
#include "doc/layer.h"

#include <string>

namespace app {

class LayerOpacityCommand : public Command {
public:
  LayerOpacityCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  int m_opacity;
};

LayerOpacityCommand::LayerOpacityCommand()
  : Command("LayerOpacity",
            "Layer Opacity",
            CmdUIOnlyFlag)
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
    Transaction transaction(writer.context(), "Set Layer Opacity");

    // TODO the range of selected frames should be in doc::Site.
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
        transaction.execute(
          new cmd::SetLayerOpacity(static_cast<LayerImage*>(layer), m_opacity));
    }

    transaction.commit();
  }

  update_screen_for_document(writer.document());
}

std::string LayerOpacityCommand::onGetFriendlyName() const
{
  std::string text = "Set Layer Opacity to ";
  text += base::convert_to<std::string>(m_opacity);
  text += " (";
  text += base::convert_to<std::string>(int(100.0 * m_opacity / 255.0));
  text += "%)";
  return text;
}

Command* CommandFactory::createLayerOpacityCommand()
{
  return new LayerOpacityCommand;
}

} // namespace app
