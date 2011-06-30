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

#include "app/color.h"
#include "base/bind.h"
#include "commands/command.h"
#include "commands/filters/filter_manager_impl.h"
#include "commands/filters/filter_window.h"
#include "document_wrappers.h"
#include "filters/invert_color_filter.h"
#include "gui/button.h"
#include "gui/frame.h"
#include "gui/label.h"
#include "gui/slider.h"
#include "gui/widget.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "widgets/color_button.h"

static const char* ConfigSection = "InvertColor";

//////////////////////////////////////////////////////////////////////
// Invert Color window

class InvertColorWindow : public FilterWindow
{
public:
  InvertColorWindow(InvertColorFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Invert Color", ConfigSection, &filterMgr,
		   WithChannelsSelector,
		   WithoutTiledCheckBox)
    , m_filter(filter)
  {
  }

private:
  InvertColorFilter& m_filter;
};

//////////////////////////////////////////////////////////////////////
// Invert Color command

class InvertColorCommand : public Command
{
public:
  InvertColorCommand();
  Command* clone() const { return new InvertColorCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

InvertColorCommand::InvertColorCommand()
  : Command("InvertColor",
	    "Invert Color",
	    CmdRecordableFlag)
{
}

bool InvertColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
			     ContextFlags::HasActiveSprite);
}

void InvertColorCommand::onExecute(Context* context)
{
  InvertColorFilter filter;
  FilterManagerImpl filterMgr(context->getActiveDocument(), &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
		      TARGET_GREEN_CHANNEL |
		      TARGET_BLUE_CHANNEL |
		      TARGET_GRAY_CHANNEL);

  InvertColorWindow window(filter, filterMgr);
  window.doModal();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createInvertColorCommand()
{
  return new InvertColorCommand;
}
