// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/document.h"
#include "doc/mask.h"
#include "ui/window.h"

#include "grid_settings.xml.h"

namespace app {

using namespace ui;
using namespace gfx;

class SnapToGridCommand : public Command {
public:
  SnapToGridCommand()
    : Command(CommandId::SnapToGrid(), CmdUIOnlyFlag) {
  }

protected:
  bool onChecked(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.grid.snap();
  }

  void onExecute(Context* ctx) override {
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    bool newValue = !docPref.grid.snap();
    docPref.grid.snap(newValue);

    StatusBar::instance()->showSnapToGridWarning(newValue);
  }
};

class SelectionAsGridCommand : public Command {
public:
  SelectionAsGridCommand()
    : Command(CommandId::SelectionAsGrid(), CmdUIOnlyFlag) {
  }

protected:
  bool onEnabled(Context* ctx) override {
    return (ctx->activeDocument() &&
            ctx->activeDocument()->isMaskVisible());
  }

  void onExecute(Context* ctx) override {
    const ContextReader reader(ctx);
    const Doc* document = reader.document();
    const Mask* mask(document->mask());
    DocumentPreferences& docPref =
      Preferences::instance().document(ctx->activeDocument());

    docPref.grid.bounds(mask->bounds());

    // Make grid visible
    if (!docPref.show.grid())
      docPref.show.grid(true);
  }
};

class GridSettingsCommand : public Command {
public:
  GridSettingsCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

GridSettingsCommand::GridSettingsCommand()
  : Command(CommandId::GridSettings(), CmdUIOnlyFlag)
{
}

bool GridSettingsCommand::onEnabled(Context* context)
{
  return true;
}

void GridSettingsCommand::onExecute(Context* context)
{
  gen::GridSettings window;

  DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
  Rect bounds = docPref.grid.bounds();

  window.gridX()->setTextf("%d", bounds.x);
  window.gridY()->setTextf("%d", bounds.y);
  window.gridW()->setTextf("%d", bounds.w);
  window.gridH()->setTextf("%d", bounds.h);
  window.openWindowInForeground();

  if (window.closer() == window.ok()) {
    bounds.x = window.gridX()->textInt();
    bounds.y = window.gridY()->textInt();
    bounds.w = window.gridW()->textInt();
    bounds.h = window.gridH()->textInt();
    bounds.w = MAX(bounds.w, 1);
    bounds.h = MAX(bounds.h, 1);

    docPref.grid.bounds(bounds);

    // Make grid visible
    if (!docPref.show.grid())
      docPref.show.grid(true);
  }
}

Command* CommandFactory::createSnapToGridCommand()
{
  return new SnapToGridCommand;
}

Command* CommandFactory::createGridSettingsCommand()
{
  return new GridSettingsCommand;
}

Command* CommandFactory::createSelectionAsGridCommand()
{
  return new SelectionAsGridCommand;
}

} // namespace app
