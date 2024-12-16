// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class ClearCelCommand : public Command {
public:
  ClearCelCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ClearCelCommand::ClearCelCommand() : Command(CommandId::ClearCel(), CmdRecordableFlag)
{
}

bool ClearCelCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ClearCelCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  bool nonEditableLayers = false;
  {
    Tx tx(writer, "Clear Cel");

    const Site* site = writer.site();
    if (site->inTimeline() && !site->selectedLayers().empty() && !site->selectedFrames().empty()) {
      for (Layer* layer : site->selectedLayers()) {
        if (!layer->isImage())
          continue;

        if (!layer->isEditableHierarchy()) {
          nonEditableLayers = true;
          continue;
        }

        for (frame_t frame : site->selectedFrames().reversed()) {
          if (Cel* cel = layer->cel(frame))
            document->getApi(tx).clearCel(cel);
        }
      }
    }
    else if (writer.cel()) {
      if (writer.layer()->isEditableHierarchy())
        document->getApi(tx).clearCel(writer.cel());
      else
        nonEditableLayers = true;
    }

    tx.commit();
  }

  if (nonEditableLayers)
    StatusBar::instance()->showTip(1000, Strings::statusbar_tips_locked_layers());

  update_screen_for_document(document);
}

Command* CommandFactory::createClearCelCommand()
{
  return new ClearCelCommand;
}

} // namespace app
