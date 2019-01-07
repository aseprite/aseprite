// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class GotoLayerCommand : public Command {
public:
  GotoLayerCommand(int offset,
                   const char* id,
                   CommandFlags flags)
    : Command(id, flags),
      m_offset(offset) {
  }

protected:

  bool onEnabled(Context* context) override {
    return (current_editor &&
            current_editor->document());
  }

  void onExecute(Context* context) override {
    Site site = current_editor->getSite();

    Layer* layer = site.layer();
    if (!layer)
      return;

    if (m_offset > 0) {
      int i = m_offset;
      while (i-- > 0) {
        layer = layer->getNextBrowsable();
        if (!layer)
          layer = site.sprite()->firstBrowsableLayer();
      }
    }
    else if (m_offset < 0) {
      int i = m_offset;
      while (i++ < 0) {
        layer = layer->getPreviousBrowsable();
        if (!layer)
          layer = site.sprite()->root()->lastLayer();
      }
    }

    site.layer(layer);

    // Flash the current layer
    current_editor->setLayer(site.layer());
    current_editor->flashCurrentLayer();

    updateStatusBar(site);
  }

  void updateStatusBar(Site& site) {
    if (site.layer() != NULL)
      StatusBar::instance()->setStatusText(
        1000, "%s '%s' selected",
        (site.layer()->isGroup() ? "Group": "Layer"),
        site.layer()->name().c_str());
  }

private:
  int m_offset;
};

class GotoPreviousLayerCommand : public GotoLayerCommand {
public:
  GotoPreviousLayerCommand()
    : GotoLayerCommand(-1, "GotoPreviousLayer",
                       CmdUIOnlyFlag) {
  }
};

class GotoNextLayerCommand : public GotoLayerCommand {
public:
  GotoNextLayerCommand()
    : GotoLayerCommand(+1, "GotoNextLayer",
                       CmdUIOnlyFlag) {
  }
};

Command* CommandFactory::createGotoPreviousLayerCommand()
{
  return new GotoPreviousLayerCommand;
}

Command* CommandFactory::createGotoNextLayerCommand()
{
  return new GotoNextLayerCommand;
}

} // namespace app
