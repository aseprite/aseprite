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

#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/context.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "base/bind.h"
#include "filters/median_filter.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <stdio.h>

namespace app {

using namespace filters;

static const char* ConfigSection = "Despeckle";

class DespeckleWindow : public FilterWindow {
public:
  DespeckleWindow(MedianFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Median Blur", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithTiledCheckBox,
                   filter.getTiledMode())
    , m_filter(filter)
    , m_controlsWidget(app::load_widget<ui::Widget>("despeckle.xml", "controls"))
    , m_widthEntry(app::find_widget<ui::Entry>(m_controlsWidget, "width"))
    , m_heightEntry(app::find_widget<ui::Entry>(m_controlsWidget, "height"))
  {
    getContainer()->addChild(m_controlsWidget);

    m_widthEntry->setTextf("%d", m_filter.getWidth());
    m_heightEntry->setTextf("%d", m_filter.getHeight());

    m_widthEntry->EntryChange.connect(&DespeckleWindow::onSizeChange, this);
    m_heightEntry->EntryChange.connect(&DespeckleWindow::onSizeChange, this);
  }

private:
  void onSizeChange()
  {
    m_filter.setSize(m_widthEntry->getTextInt(),
                     m_heightEntry->getTextInt());
    restartPreview();
  }

  void setupTiledMode(TiledMode tiledMode)
  {
    m_filter.setTiledMode(tiledMode);
  }

  MedianFilter& m_filter;
  base::UniquePtr<ui::Widget> m_controlsWidget;
  ui::Entry* m_widthEntry;
  ui::Entry* m_heightEntry;
};

//////////////////////////////////////////////////////////////////////
// Despeckle command

class DespeckleCommand : public Command
{
public:
  DespeckleCommand();
  Command* clone() const OVERRIDE { return new DespeckleCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

DespeckleCommand::DespeckleCommand()
  : Command("Despeckle",
            "Despeckle",
            CmdRecordableFlag)
{
}

bool DespeckleCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void DespeckleCommand::onExecute(Context* context)
{
  IDocumentSettings* docSettings = context->settings()->getDocumentSettings(context->activeDocument());

  MedianFilter filter;
  filter.setTiledMode(docSettings->getTiledMode());
  filter.setSize(get_config_int(ConfigSection, "Width", 3),
                 get_config_int(ConfigSection, "Height", 3));

  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL);

  DespeckleWindow window(filter, filterMgr);
  if (window.doModal()) {
    set_config_int(ConfigSection, "Width", filter.getWidth());
    set_config_int(ConfigSection, "Height", filter.getHeight());
  }
}

Command* CommandFactory::createDespeckleCommand()
{
  return new DespeckleCommand;
}

} // namespace app
