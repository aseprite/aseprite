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
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "gfx/size.h"
#include "raster/mask.h"
#include "ui/ui.h"

namespace app {

using namespace gfx;
using namespace ui;
using namespace app::tools;

static Window* window = NULL;

static bool window_close_hook(Widget* widget, void *data);

// Slot for App::Exit signal
static void on_exit_delete_this_widget()
{
  ASSERT(window != NULL);
  delete window;
}

class ConfigureTools : public Command {
public:
  ConfigureTools();
  Command* clone() const { return new ConfigureTools(*this); }

protected:
  void onExecute(Context* context);

private:
  CheckBox* m_tiled;
  CheckBox* m_tiledX;
  CheckBox* m_tiledY;
  CheckBox* m_pixelGrid;
  CheckBox* m_snapToGrid;
  CheckBox* m_viewGrid;
  ISettings* m_settings;
  IDocumentSettings* m_docSettings;

  void onWindowClose();
  void onTiledClick();
  void onTiledXYClick(int tiled_axis, CheckBox* checkbox);
  void onViewGridClick();
  void onPixelGridClick();
  void onSetGridClick();
  void onSnapToGridClick();
};

ConfigureTools::ConfigureTools()
  : Command("ConfigureTools",
            "Configure Tools",
            CmdUIOnlyFlag)
{
  m_settings = NULL;
  m_docSettings = NULL;
}

void ConfigureTools::onExecute(Context* context)
{
  m_settings = UIContext::instance()->getSettings();
  m_docSettings = m_settings->getDocumentSettings(NULL);

  Button* set_grid;
  bool first_time = false;

  if (!window) {
    window = app::load_widget<Window>("tools_configuration.xml", "configure_tool");
    first_time = true;
  }
  // If the window is opened, close it
  else if (window->isVisible()) {
    window->closeWindow(NULL);
    return;
  }

  try {
    m_tiled = app::find_widget<CheckBox>(window, "tiled");
    m_tiledX = app::find_widget<CheckBox>(window, "tiled_x");
    m_tiledY = app::find_widget<CheckBox>(window, "tiled_y");
    m_snapToGrid = app::find_widget<CheckBox>(window, "snap_to_grid");
    m_viewGrid = app::find_widget<CheckBox>(window, "view_grid");
    m_pixelGrid = app::find_widget<CheckBox>(window, "pixel_grid");
    set_grid = app::find_widget<Button>(window, "set_grid");
  }
  catch (...) {
    delete window;
    window = NULL;
    throw;
  }

  // Current settings
  Tool* current_tool = m_settings->getCurrentTool();
  IToolSettings* tool_settings = m_settings->getToolSettings(current_tool);

  if (m_docSettings->getTiledMode() != filters::TILED_NONE) {
    m_tiled->setSelected(true);
    if (m_docSettings->getTiledMode() & filters::TILED_X_AXIS) m_tiledX->setSelected(true);
    if (m_docSettings->getTiledMode() & filters::TILED_Y_AXIS) m_tiledY->setSelected(true);
  }

  if (m_docSettings->getSnapToGrid()) m_snapToGrid->setSelected(true);
  if (m_docSettings->getGridVisible()) m_viewGrid->setSelected(true);
  if (m_docSettings->getPixelGridVisible()) m_pixelGrid->setSelected(true);

  if (first_time) {
    // Slots
    window->Close.connect(Bind<void>(&ConfigureTools::onWindowClose, this));
    m_tiled->Click.connect(Bind<void>(&ConfigureTools::onTiledClick, this));
    m_tiledX->Click.connect(Bind<void>(&ConfigureTools::onTiledXYClick, this, filters::TILED_X_AXIS, m_tiledX));
    m_tiledY->Click.connect(Bind<void>(&ConfigureTools::onTiledXYClick, this, filters::TILED_Y_AXIS, m_tiledY));
    m_viewGrid->Click.connect(Bind<void>(&ConfigureTools::onViewGridClick, this));
    m_pixelGrid->Click.connect(Bind<void>(&ConfigureTools::onPixelGridClick, this));
    set_grid->Click.connect(Bind<void>(&ConfigureTools::onSetGridClick, this));
    m_snapToGrid->Click.connect(Bind<void>(&ConfigureTools::onSnapToGridClick, this));

    App::instance()->Exit.connect(&on_exit_delete_this_widget);
  }

  // Default position
  window->remapWindow();
  window->centerWindow();

  // Load window configuration
  load_window_pos(window, "ConfigureTool");

  window->openWindow();
}

void ConfigureTools::onWindowClose()
{
  save_window_pos(window, "ConfigureTool");
}

void ConfigureTools::onTiledClick()
{
  bool flag = m_tiled->isSelected();

  m_docSettings->setTiledMode(flag ? filters::TILED_BOTH:
                                     filters::TILED_NONE);

  m_tiledX->setSelected(flag);
  m_tiledY->setSelected(flag);
}

void ConfigureTools::onTiledXYClick(int tiled_axis, CheckBox* checkbox)
{
  int tiled_mode = m_docSettings->getTiledMode();

  if (checkbox->isSelected())
    tiled_mode |= tiled_axis;
  else
    tiled_mode &= ~tiled_axis;

  checkbox->findSibling("tiled")->setSelected(tiled_mode != filters::TILED_NONE);

  m_docSettings->setTiledMode((filters::TiledMode)tiled_mode);
}

void ConfigureTools::onSnapToGridClick()
{
  m_docSettings->setSnapToGrid(m_snapToGrid->isSelected());
}

void ConfigureTools::onViewGridClick()
{
  m_docSettings->setGridVisible(m_viewGrid->isSelected());
}

void ConfigureTools::onPixelGridClick()
{
  m_docSettings->setPixelGridVisible(m_pixelGrid->isSelected());
}

void ConfigureTools::onSetGridClick()
{
  try {
    // TODO use the same context as in ConfigureTools::onExecute
    const ContextReader reader(UIContext::instance());
    const Document* document = reader.document();

    if (document && document->isMaskVisible()) {
      const Mask* mask(document->getMask());

      m_docSettings->setGridBounds(mask->getBounds());
    }
    else {
      Command* grid_settings_cmd =
        CommandsModule::instance()->getCommandByName(CommandId::GridSettings);

      UIContext::instance()->executeCommand(grid_settings_cmd, NULL);
    }
  }
  catch (LockedDocumentException& e) {
    Console::showException(e);
  }
}

Command* CommandFactory::createConfigureToolsCommand()
{
  return new ConfigureTools;
}

} // namespace app
