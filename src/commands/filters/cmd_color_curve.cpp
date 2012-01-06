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

#include "app.h"
#include "app/color.h"
#include "base/bind.h"
#include "commands/command.h"
#include "commands/filters/color_curve_editor.h"
#include "commands/filters/filter_manager_impl.h"
#include "commands/filters/filter_window.h"
#include "document_wrappers.h"
#include "filters/color_curve.h"
#include "filters/color_curve_filter.h"
#include "gui/gui.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "widgets/color_button.h"

static ColorCurve* the_curve = NULL;

// Slot for App::Exit signal
static void on_exit_delete_curve()
{
  delete the_curve;
}

//////////////////////////////////////////////////////////////////////
// Color Curve window

class ColorCurveWindow : public FilterWindow
{
public:
  ColorCurveWindow(ColorCurveFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Color Curve", "ColorCurve", &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
    , m_editor(filter.getCurve(), 0, 0, 255, 255)
  {
    m_view.attachToView(&m_editor);
    jwidget_expansive(&m_view, true);
    jwidget_set_min_size(&m_view, 128, 64);

    getContainer()->addChild(&m_view);

    m_editor.CurveEditorChange.connect(&ColorCurveWindow::onCurveChange, this);
  }

protected:

  void onCurveChange()
  {
    // The color curve in the filter is the same refereced by the
    // editor. But anyway, we have to re-set the same curve in the
    // filter to regenerate the map used internally by the filter
    // (which is calculated inside setCurve() method).
    m_filter.setCurve(m_editor.getCurve());

    restartPreview();
  }

private:
  ColorCurveFilter& m_filter;
  View m_view;
  ColorCurveEditor m_editor;
};

//////////////////////////////////////////////////////////////////////
// Color Curve command

class ColorCurveCommand : public Command
{
public:
  ColorCurveCommand();
  Command* clone() const { return new ColorCurveCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ColorCurveCommand::ColorCurveCommand()
  : Command("ColorCurve",
            "Color Curve",
            CmdRecordableFlag)
{
}

bool ColorCurveCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ColorCurveCommand::onExecute(Context* context)
{
  // Default curve
  if (!the_curve) {
    // TODO load the curve?

    the_curve = new ColorCurve(ColorCurve::Linear);
    the_curve->addPoint(gfx::Point(0, 0));
    the_curve->addPoint(gfx::Point(255, 255));

    App::instance()->Exit.connect(&on_exit_delete_curve);
  }

  ColorCurveFilter filter;
  filter.setCurve(the_curve);

  FilterManagerImpl filterMgr(context->getActiveDocument(), &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL |
                      TARGET_ALPHA_CHANNEL);

  ColorCurveWindow window(filter, filterMgr);
  if (window.doModal()) {
    // TODO save the curve?
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createColorCurveCommand()
{
  return new ColorCurveCommand;
}
