// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/filters/color_curve_editor.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/context.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/ui/color_button.h"
#include "base/bind.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/color_curve.h"
#include "filters/color_curve_filter.h"
#include "ui/ui.h"

namespace app {

using namespace filters;

static std::unique_ptr<ColorCurve> the_curve;

class ColorCurveWindow : public FilterWindow {
public:
  ColorCurveWindow(ColorCurveFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Color Curve", "ColorCurve", &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
    , m_editor(filter.getCurve(), gfx::Rect(0, 0, 256, 256))
  {
    m_view.attachToView(&m_editor);
    m_view.setExpansive(true);
    m_view.setMinSize(gfx::Size(128, 64));

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
  ui::View m_view;
  ColorCurveEditor m_editor;
};

class ColorCurveCommand : public Command {
public:
  ColorCurveCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ColorCurveCommand::ColorCurveCommand()
  : Command(CommandId::ColorCurve(), CmdRecordableFlag)
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

    the_curve.reset(new ColorCurve(ColorCurve::Linear));
    the_curve->addPoint(gfx::Point(0, 0));
    the_curve->addPoint(gfx::Point(255, 255));
  }

  ColorCurveFilter filter;
  filter.setCurve(the_curve.get());

  FilterManagerImpl filterMgr(context, &filter);
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

Command* CommandFactory::createColorCurveCommand()
{
  return new ColorCurveCommand;
}

} // namespace app
