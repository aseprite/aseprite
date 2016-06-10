// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "app/ui/timeline.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class GotoLayerCommand : public Command {
public:
  GotoLayerCommand(int offset,
                   const char* shortName,
                   const char* friendlyName,
                   CommandFlags flags)
    : Command(shortName, friendlyName, flags),
      m_offset(offset) {
  }

protected:

  bool onEnabled(Context* context) override {
    return (current_editor &&
            current_editor->document());
  }

  void onExecute(Context* context) override {
    Site site = current_editor->getSite();

    // TODO add support for layer groups

    Layer* layer = site.layer();
    if (!layer)
      return;

    const auto& layers = layer->parent()->layers();
    auto it = std::find(layers.begin(), layers.end(), layer);
    if (it == layers.end())
      return;

    if (m_offset > 0) {
      int i = m_offset;
      while (i-- > 0) {
        ++it;
        if (it == layers.end())
          it = layers.begin();
      }
    }
    else if (m_offset < 0) {
      int i = m_offset;
      while (i++ < 0) {
        if (it == layers.begin())
          it = layers.end();
        --it;
      }
    }

    site.layer(*it);

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
                       "Go to Previous Layer",
                       CmdUIOnlyFlag) {
  }
  Command* clone() const override {
    return new GotoPreviousLayerCommand(*this);
  }
};

class GotoNextLayerCommand : public GotoLayerCommand {
public:
  GotoNextLayerCommand()
    : GotoLayerCommand(+1, "GotoNextLayer",
                       "Go to Next Layer",
                       CmdUIOnlyFlag) {
  }
  Command* clone() const override {
    return new GotoNextLayerCommand(*this);
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
