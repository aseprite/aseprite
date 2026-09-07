// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_grid_bounds.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/status_bar.h"
#include "doc/mask.h"
#include "ui/window.h"

#include "grid_settings.xml.h"

#include <algorithm>

namespace app {

using namespace ui;
using namespace gfx;

class SnapToGridCommand : public Command {
public:
  SnapToGridCommand() : Command(CommandId::SnapToGrid()) {}

protected:
  bool onChecked(Context* ctx) override
  {
    auto& docPref = Preferences::instance().document(ctx->activeDocument());
    return docPref.grid.snap();
  }

  void onExecute(Context* ctx) override
  {
    auto& docPref = Preferences::instance().document(ctx->activeDocument());
    bool newValue = !docPref.grid.snap();
    docPref.grid.snap(newValue);

    if (ctx->isUIAvailable())
      StatusBar::instance()->showSnapToGridWarning(newValue);
  }
};

class SelectionAsGridCommand : public Command {
public:
  SelectionAsGridCommand() : Command(CommandId::SelectionAsGrid()) {}

protected:
  bool onEnabled(Context* ctx) override
  {
    return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasVisibleMask);
  }

  void onExecute(Context* ctx) override
  {
    ContextWriter writer(ctx);
    Doc* doc = writer.document();
    const Mask* mask = doc->mask();
    gfx::Rect newGrid = mask->bounds();

    Tx tx(writer, friendlyName(), ModifyDocument);
    tx(new cmd::SetGridBounds(writer.sprite(), newGrid));
    tx.commit();

    auto& docPref = Preferences::instance().document(doc);
    docPref.grid.bounds(newGrid);
    if (!docPref.show.grid()) // Make grid visible
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

GridSettingsCommand::GridSettingsCommand() : Command(CommandId::GridSettings())
{
}

bool GridSettingsCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

void GridSettingsCommand::onExecute(Context* context)
{
  gen::GridSettings window;

  Site site = context->activeSite();
  Rect bounds = site.gridBounds();
  auto& docPref = Preferences::instance().document(site.document());
  gfx::Size subdiv = docPref.gridSubdivisions.subdivisions();

  window.gridX()->setTextf("%d", bounds.x);
  window.gridY()->setTextf("%d", bounds.y);
  window.gridW()->setTextf("%d", bounds.w);
  window.gridH()->setTextf("%d", bounds.h);
  window.subdivX()->setTextf("%d", subdiv.w);
  window.subdivY()->setTextf("%d", subdiv.h);
  window.gridW()->Leave.connect([&window] {
    // Prevent entering a width lesser than 1
    if (window.gridW()->textInt() <= 0)
      window.gridW()->setText("1");
  });
  window.gridH()->Leave.connect([&window] {
    // Prevent entering a height lesser than 1
    if (window.gridH()->textInt() <= 0)
      window.gridH()->setText("1");
  });
  window.subdivX()->Leave.connect([&window] {
    if (window.subdivX()->textInt() <= 0)
      window.subdivX()->setText("1");
  });
  window.subdivY()->Leave.connect([&window] {
    if (window.subdivY()->textInt() <= 0)
      window.subdivY()->setText("1");
  });

  auto applySettings = [&]() {
    Rect newBounds;
    newBounds.x = window.gridX()->textInt();
    newBounds.y = window.gridY()->textInt();
    newBounds.w = std::max(window.gridW()->textInt(), 1);
    newBounds.h = std::max(window.gridH()->textInt(), 1);

    int sx = std::max(window.subdivX()->textInt(), 1);
    int sy = std::max(window.subdivY()->textInt(), 1);
    docPref.gridSubdivisions.subdivisions(gfx::Size(sx, sy));

    ContextWriter writer(context);
    Tx tx(writer, friendlyName(), ModifyDocument);
    tx(new cmd::SetGridBounds(site.sprite(), newBounds));
    tx.commit();

    if (!docPref.show.grid())
      docPref.show.grid(true);
    if ((sx > 1 || sy > 1) && !docPref.show.gridSubdivisions())
      docPref.show.gridSubdivisions(true);
  };

  window.apply()->Click.connect([&] { applySettings(); });
  window.openWindowInForeground();

  if (window.closer() == window.ok())
    applySettings();
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
