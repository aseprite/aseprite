/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
#include "raster/layer.h"
#include "raster/sprite.h"

namespace app {

class GotoCommand : public Command {
public:
  GotoCommand(const char* short_name, const char* friendly_name, CommandFlags flags)
    : Command(short_name, friendly_name, flags) {
  }

protected:
  void updateStatusBar(DocumentLocation& location) {
    if (location.layer() != NULL)
      StatusBar::instance()
        ->setStatusText(1000, "Layer `%s' selected",
          location.layer()->getName().c_str());
  }
};

class GotoPreviousLayerCommand : public GotoCommand {
public:
  GotoPreviousLayerCommand();
  Command* clone() { return new GotoPreviousLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoPreviousLayerCommand::GotoPreviousLayerCommand()
 : GotoCommand("GotoPreviousLayer",
               "Goto Previous Layer",
               CmdUIOnlyFlag)
{
}

bool GotoPreviousLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void GotoPreviousLayerCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  DocumentLocation location = *reader.location();

  if (location.layerIndex() > 0)
    location.layerIndex(location.layerIndex().previous());
  else
    location.layerIndex(LayerIndex(sprite->countLayers()-1));

  // Flash the current layer
  ASSERT(current_editor != NULL && "Current editor cannot be null when we have a current sprite");
  current_editor->setLayer(location.layer());
  current_editor->flashCurrentLayer();

  updateStatusBar(location);
}

class GotoNextLayerCommand : public GotoCommand {
public:
  GotoNextLayerCommand();
  Command* clone() { return new GotoNextLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoNextLayerCommand::GotoNextLayerCommand()
  : GotoCommand("GotoNextLayer",
                "Goto Next Layer",
                CmdUIOnlyFlag)
{
}

bool GotoNextLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void GotoNextLayerCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();
  DocumentLocation location = *reader.location();

  if (location.layerIndex() < sprite->countLayers()-1)
    location.layerIndex(location.layerIndex().next());
  else
    location.layerIndex(LayerIndex(0));

  // Flash the current layer
  ASSERT(current_editor != NULL && "Current editor cannot be null when we have a current sprite");
  current_editor->setLayer(location.layer());
  current_editor->flashCurrentLayer();

  updateStatusBar(location);
}

Command* CommandFactory::createGotoPreviousLayerCommand()
{
  return new GotoPreviousLayerCommand;
}

Command* CommandFactory::createGotoNextLayerCommand()
{
  return new GotoNextLayerCommand;
}

} // namespace app
