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
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/document.h"
#include "doc/mask.h"
#include "ui/window.h"

namespace app {

using namespace ui;
using namespace gfx;

class SnapToGridCommand : public Command {
public:
  SnapToGridCommand()
    : Command("SnapToGrid", CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new SnapToGridCommand(*this); }

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
    : Command("SelectionAsGrid", CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new SelectionAsGridCommand(*this); }

protected:
  bool onEnabled(Context* ctx) override {
    return (ctx->activeDocument() &&
            ctx->activeDocument()->isMaskVisible());
  }

  void onExecute(Context* ctx) override {
    const ContextReader reader(ctx);
    const Document* document = reader.document();
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
  Command* clone() const override { return new GridSettingsCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

GridSettingsCommand::GridSettingsCommand()
  : Command("GridSettings", CmdUIOnlyFlag)
{
}

bool GridSettingsCommand::onEnabled(Context* context)
{
  return true;
}

void GridSettingsCommand::onExecute(Context* context)
{
  base::UniquePtr<Window> window(app::load_widget<Window>("grid_settings.xml", "grid_settings"));
  Widget* button_ok = app::find_widget<Widget>(window, "ok");
  Widget* grid_x = app::find_widget<Widget>(window, "grid_x");
  Widget* grid_y = app::find_widget<Widget>(window, "grid_y");
  Widget* grid_w = app::find_widget<Widget>(window, "grid_w");
  Widget* grid_h = app::find_widget<Widget>(window, "grid_h");

  DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
  Rect bounds = docPref.grid.bounds();

  grid_x->setTextf("%d", bounds.x);
  grid_y->setTextf("%d", bounds.y);
  grid_w->setTextf("%d", bounds.w);
  grid_h->setTextf("%d", bounds.h);

  window->openWindowInForeground();

  if (window->closer() == button_ok) {
    bounds.x = grid_x->textInt();
    bounds.y = grid_y->textInt();
    bounds.w = grid_w->textInt();
    bounds.h = grid_h->textInt();
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
