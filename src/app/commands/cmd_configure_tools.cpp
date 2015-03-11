// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/settings/settings.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "doc/context_observer.h"
#include "doc/mask.h"
#include "gfx/size.h"
#include "ui/ui.h"

#include "generated_tools_configuration.h"

namespace app {

using namespace gfx;
using namespace ui;
using namespace app::tools;

class ToolsConfigurationWindow : public app::gen::ToolsConfiguration,
                                 public doc::ContextObserver {
public:
  ToolsConfigurationWindow(Context* ctx) : m_ctx(ctx) {
    m_ctx->addObserver(this);

    // Slots
    this->Close.connect(Bind<void>(&ToolsConfigurationWindow::onWindowClose, this));
    tiled()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onTiledClick, this));
    tiledX()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onTiledXYClick, this, filters::TiledMode::X_AXIS, tiledX()));
    tiledY()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onTiledXYClick, this, filters::TiledMode::Y_AXIS, tiledY()));
    viewGrid()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onViewGridClick, this));
    pixelGrid()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onPixelGridClick, this));
    setGrid()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onSetGridClick, this));
    snapToGrid()->Click.connect(Bind<void>(&ToolsConfigurationWindow::onSnapToGridClick, this));

    remapWindow();
    centerWindow();
    load_window_pos(this, "ConfigureTool");

    onSetActiveDocument(m_ctx->activeDocument());
  }

  ~ToolsConfigurationWindow() {
    m_ctx->removeObserver(this);
  }

private:
  ISettings* settings() {
    ASSERT(m_ctx);
    return m_ctx->settings();
  }

  Preferences& preferences() {
    return App::instance()->preferences();
  }

  DocumentPreferences& docPref() {
    return preferences().document(m_ctx->activeDocument());
  }

  void onSetActiveDocument(doc::Document* document) override {
    DocumentPreferences& docPref = this->docPref();

    tiled()->setSelected(docPref.tiled.mode() != filters::TiledMode::NONE);
    tiledX()->setSelected(int(docPref.tiled.mode()) & int(filters::TiledMode::X_AXIS) ? true: false);
    tiledY()->setSelected(int(docPref.tiled.mode()) & int(filters::TiledMode::Y_AXIS) ? true: false);
    snapToGrid()->setSelected(docPref.grid.snap());
    viewGrid()->setSelected(docPref.grid.visible());
    pixelGrid()->setSelected(docPref.pixelGrid.visible());
  }

  void onWindowClose() {
    save_window_pos(this, "ConfigureTool");
  }

  void onTiledClick() {
    bool flag = tiled()->isSelected();

    docPref().tiled.mode(
      flag ? filters::TiledMode::BOTH:
             filters::TiledMode::NONE);

    tiledX()->setSelected(flag);
    tiledY()->setSelected(flag);
  }

  void onTiledXYClick(filters::TiledMode tiled_axis, CheckBox* checkbox) {
    int tiled_mode = int(docPref().tiled.mode());

    if (checkbox->isSelected())
      tiled_mode |= int(tiled_axis);
    else
      tiled_mode &= ~int(tiled_axis);

    checkbox->findSibling("tiled")->setSelected(
      tiled_mode != int(filters::TiledMode::NONE));

    docPref().tiled.mode(filters::TiledMode(tiled_mode));
  }

  void onViewGridClick() {
    docPref().grid.visible(viewGrid()->isSelected());
  }

  void onPixelGridClick() {
    docPref().pixelGrid.visible(pixelGrid()->isSelected());
  }

  void onSetGridClick() {
    try {
      // TODO use the same context as in ConfigureTools::onExecute
      const ContextReader reader(UIContext::instance());
      const Document* document = reader.document();

      if (document && document->isMaskVisible()) {
        const Mask* mask(document->mask());

        docPref().grid.bounds(mask->bounds());
      }
      else {
        Command* grid_settings_cmd =
          CommandsModule::instance()->getCommandByName(CommandId::GridSettings);

        UIContext::instance()->executeCommand(grid_settings_cmd);
      }
    }
    catch (LockedDocumentException& e) {
      Console::showException(e);
    }
  }

  void onSnapToGridClick() {
    docPref().grid.snap(snapToGrid()->isSelected());
  }

  Context* m_ctx;
};

class ConfigureTools : public Command {
public:
  ConfigureTools();
  Command* clone() const override { return new ConfigureTools(*this); }

protected:
  void onExecute(Context* context) override;
};

static ToolsConfigurationWindow* window;

ConfigureTools::ConfigureTools()
  : Command("ConfigureTools",
            "Configure Tools",
            CmdUIOnlyFlag)
{
}

// Slot for App::Exit signal
static void on_exit_delete_this_widget()
{
  ASSERT(window != NULL);
  delete window;
}

void ConfigureTools::onExecute(Context* context)
{
  if (!window) {
    window = new ToolsConfigurationWindow(context);

    App::instance()->Exit.connect(&on_exit_delete_this_widget);
  }
  // If the window is opened, close it
  else if (window->isVisible()) {
    window->closeWindow(NULL);
    return;
  }

  window->openWindow();
}

Command* CommandFactory::createConfigureToolsCommand()
{
  return new ConfigureTools;
}

} // namespace app
