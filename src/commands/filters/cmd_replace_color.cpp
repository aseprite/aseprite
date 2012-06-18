/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "base/bind.h"
#include "commands/command.h"
#include "commands/filters/filter_manager_impl.h"
#include "commands/filters/filter_window.h"
#include "document_wrappers.h"
#include "filters/replace_color_filter.h"
#include "ini_file.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/gui.h"
#include "widgets/color_bar.h"
#include "widgets/color_button.h"

static const char* ConfigSection = "ReplaceColor";

//////////////////////////////////////////////////////////////////////
// Wrapper for ReplaceColorFilter to handle colors in an easy way

class ReplaceColorFilterWrapper : public ReplaceColorFilter
{
public:
  ReplaceColorFilterWrapper(Layer* layer) : m_layer(layer) { }

  void setFrom(const Color& from) {
    m_from = from;
    ReplaceColorFilter::setFrom(color_utils::color_for_layer(from, m_layer));
  }
  void setTo(const Color& to) {
    m_to = to;
    ReplaceColorFilter::setTo(color_utils::color_for_layer(to, m_layer));
  }

  Color getFrom() const { return m_from; }
  Color getTo() const { return m_to; }

private:
  Layer* m_layer;
  Color m_from;
  Color m_to;
};

//////////////////////////////////////////////////////////////////////
// Replace Color window

class ReplaceColorWindow : public FilterWindow
{
public:
  ReplaceColorWindow(ReplaceColorFilterWrapper& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Replace Color", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
    , m_controlsWidget(app::load_widget<Widget>("replace_color.xml", "controls"))
    , m_fromButton(app::find_widget<ColorButton>(m_controlsWidget, "from"))
    , m_toButton(app::find_widget<ColorButton>(m_controlsWidget, "to"))
    , m_toleranceSlider(app::find_widget<ui::Slider>(m_controlsWidget, "tolerance"))
  {
    getContainer()->addChild(m_controlsWidget);

    m_fromButton->setColor(m_filter.getFrom());
    m_toButton->setColor(m_filter.getTo());
    m_toleranceSlider->setValue(m_filter.getTolerance());

    m_fromButton->Change.connect(&ReplaceColorWindow::onFromChange, this);
    m_toButton->Change.connect(&ReplaceColorWindow::onToChange, this);
    m_toleranceSlider->Change.connect(&ReplaceColorWindow::onToleranceChange, this);
  }

protected:
  void onFromChange(const Color& color)
  {
    m_filter.setFrom(color);
    restartPreview();
  }

  void onToChange(const Color& color)
  {
    m_filter.setTo(color);
    restartPreview();
  }

  void onToleranceChange()
  {
    m_filter.setTolerance(m_toleranceSlider->getValue());
    restartPreview();
  }

private:
  ReplaceColorFilterWrapper& m_filter;
  UniquePtr<ui::Widget> m_controlsWidget;
  ColorButton* m_fromButton;
  ColorButton* m_toButton;
  ui::Slider* m_toleranceSlider;
};

//////////////////////////////////////////////////////////////////////
// Replace Color command

class ReplaceColorCommand : public Command
{
public:
  ReplaceColorCommand();
  Command* clone() const { return new ReplaceColorCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ReplaceColorCommand::ReplaceColorCommand()
  : Command("ReplaceColor",
            "Replace Color",
            CmdRecordableFlag)
{
}

bool ReplaceColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ReplaceColorCommand::onExecute(Context* context)
{
  Document* document = context->getActiveDocument();
  Sprite* sprite(document->getSprite());

  ReplaceColorFilterWrapper filter(sprite->getCurrentLayer());
  filter.setFrom(get_config_color(ConfigSection, "Color1", app_get_colorbar()->getFgColor()));
  filter.setTo(get_config_color(ConfigSection, "Color2", app_get_colorbar()->getBgColor()));
  filter.setTolerance(get_config_int(ConfigSection, "Tolerance", 0));

  FilterManagerImpl filterMgr(document, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL |
                      TARGET_ALPHA_CHANNEL);

  ReplaceColorWindow window(filter, filterMgr);
  if (window.doModal()) {
    set_config_color(ConfigSection, "From", filter.getFrom());
    set_config_color(ConfigSection, "To", filter.getTo());
    set_config_int(ConfigSection, "Tolerance", filter.getTolerance());
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createReplaceColorCommand()
{
  return new ReplaceColorCommand;
}
