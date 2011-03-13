/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "config.h"

#include <stdio.h>

#include "base/bind.h"
#include "commands/command.h"
#include "commands/filters/filter_manager_impl.h"
#include "commands/filters/filter_window.h"
#include "core/cfg.h"
#include "filters/median_filter.h"
#include "gui/button.h"
#include "gui/entry.h"
#include "gui/frame.h"
#include "gui/grid.h"
#include "gui/widget.h"
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "settings/settings.h"
#include "sprite_wrappers.h"

static const char* ConfigSection = "Despeckle";

//////////////////////////////////////////////////////////////////////
// Despeckle window

class DespeckleWindow : public FilterWindow
{
public:
  DespeckleWindow(MedianFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Median Blur", ConfigSection, &filterMgr,
		   WithChannelsSelector,
		   WithTiledCheckBox,
		   filter.getTiledMode())
    , m_filter(filter)
    , m_controlsWidget(load_widget("despeckle.xml", "controls"))
  {
    get_widgets(m_controlsWidget,
		"width", &m_widthEntry,
		"height", &m_heightEntry, NULL);

    jwidget_add_child(getContainer(), m_controlsWidget);

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
  WidgetPtr m_controlsWidget;
  Entry* m_widthEntry;
  Entry* m_heightEntry;
};

//////////////////////////////////////////////////////////////////////
// Despeckle command

class DespeckleCommand : public Command
{
public:
  DespeckleCommand();
  Command* clone() const { return new DespeckleCommand(*this); }

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
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void DespeckleCommand::onExecute(Context* context)
{
  MedianFilter filter;
  filter.setTiledMode(context->getSettings()->getTiledMode());
  filter.setSize(get_config_int(ConfigSection, "Width", 3),
		 get_config_int(ConfigSection, "Height", 3));

  FilterManagerImpl filterMgr(context->getCurrentSprite(), &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
		      TARGET_GREEN_CHANNEL |
		      TARGET_BLUE_CHANNEL |
		      TARGET_GRAY_CHANNEL);

  DespeckleWindow window(filter, filterMgr);
  if (window.doModal()) {
    set_config_int(ConfigSection, "Width", filter.getWidth());
    set_config_int(ConfigSection, "Height", filter.getHeight());
    context->getSettings()->setTiledMode(filter.getTiledMode());
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createDespeckleCommand()
{
  return new DespeckleCommand;
}
