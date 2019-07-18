// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/commands/filters/filter_worker.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/replace_color_filter.h"
#include "ui/ui.h"

namespace app {

struct ReplaceColorParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<filters::Target> channels { this, 0, "channels" };
  Param<app::Color> from { this, app::Color(), "from" };
  Param<app::Color> to { this, app::Color(), "to" };
  Param<int> tolerance { this, 0, "tolerance" };
};

// Wrapper for ReplaceColorFilter to handle colors in an easy way
class ReplaceColorFilterWrapper : public ReplaceColorFilter {
public:
  ReplaceColorFilterWrapper(Layer* layer) : m_layer(layer) { }

  void setFrom(const app::Color& from) {
    m_from = from;
    if (m_layer)
      ReplaceColorFilter::setFrom(color_utils::color_for_layer(from, m_layer));
  }
  void setTo(const app::Color& to) {
    m_to = to;
    if (m_layer)
      ReplaceColorFilter::setTo(color_utils::color_for_layer(to, m_layer));
  }

  app::Color getFrom() const { return m_from; }
  app::Color getTo() const { return m_to; }

private:
  Layer* m_layer;
  app::Color m_from;
  app::Color m_to;
};

#ifdef ENABLE_UI

static const char* ConfigSection = "ReplaceColor";

class ReplaceColorWindow : public FilterWindow {
public:
  ReplaceColorWindow(ReplaceColorFilterWrapper& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Replace Color", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithoutTiledCheckBox)
    , m_filter(filter)
    , m_controlsWidget(app::load_widget<Widget>("replace_color.xml", "controls"))
    , m_fromButton(app::find_widget<ColorButton>(m_controlsWidget.get(), "from"))
    , m_toButton(app::find_widget<ColorButton>(m_controlsWidget.get(), "to"))
    , m_toleranceSlider(app::find_widget<ui::Slider>(m_controlsWidget.get(), "tolerance"))
  {
    getContainer()->addChild(m_controlsWidget.get());

    m_fromButton->setColor(m_filter.getFrom());
    m_toButton->setColor(m_filter.getTo());
    m_toleranceSlider->setValue(m_filter.getTolerance());

    m_fromButton->Change.connect(&ReplaceColorWindow::onFromChange, this);
    m_toButton->Change.connect(&ReplaceColorWindow::onToChange, this);
    m_toleranceSlider->Change.connect(&ReplaceColorWindow::onToleranceChange, this);
  }

private:

  void onFromChange(const app::Color& color) {
    m_filter.setFrom(color);
    restartPreview();
  }

  void onToChange(const app::Color& color) {
    m_filter.setTo(color);
    restartPreview();
  }

  void onToleranceChange() {
    m_filter.setTolerance(m_toleranceSlider->getValue());
    restartPreview();
  }

  bool onProcessMessage(ui::Message* msg) override {
    switch (msg->type()) {
      case ui::kKeyDownMessage: {
        KeyboardShortcuts* keys = KeyboardShortcuts::instance();
        const KeyPtr key = keys->command(CommandId::SwitchColors());
        if (key && key->isPressed(msg, *keys)) {
          // Switch colors
          app::Color from = m_fromButton->getColor();
          app::Color to = m_toButton->getColor();
          m_fromButton->setColor(to);
          m_toButton->setColor(from);
        }
        break;
      }
    }
    return FilterWindow::onProcessMessage(msg);
  }

  ReplaceColorFilterWrapper& m_filter;
  std::unique_ptr<ui::Widget> m_controlsWidget;
  ColorButton* m_fromButton;
  ColorButton* m_toButton;
  ui::Slider* m_toleranceSlider;
};

#endif  // ENABLE_UI

class ReplaceColorCommand : public CommandWithNewParams<ReplaceColorParams> {
public:
  ReplaceColorCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ReplaceColorCommand::ReplaceColorCommand()
  : CommandWithNewParams<ReplaceColorParams>(CommandId::ReplaceColor(), CmdRecordableFlag)
{
}

bool ReplaceColorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ReplaceColorCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif
  Site site = context->activeSite();

  ReplaceColorFilterWrapper filter(site.layer());
  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(
    site.sprite()->pixelFormat() == IMAGE_INDEXED ?
    TARGET_INDEX_CHANNEL:
    TARGET_RED_CHANNEL |
    TARGET_GREEN_CHANNEL |
    TARGET_BLUE_CHANNEL |
    TARGET_GRAY_CHANNEL |
    TARGET_ALPHA_CHANNEL);

#ifdef ENABLE_UI
  if (ui) {
    filter.setFrom(get_config_color(ConfigSection, "Color1", Preferences::instance().colorBar.fgColor()));
    filter.setTo(get_config_color(ConfigSection, "Color2", Preferences::instance().colorBar.bgColor()));
    filter.setTolerance(get_config_int(ConfigSection, "Tolerance", 0));
  }
  else
#endif // ENABLE_UI
  {
    filter.setFrom(Preferences::instance().colorBar.fgColor());
    filter.setTo(Preferences::instance().colorBar.bgColor());
    filter.setTolerance(params().tolerance());
  }

  if (params().from.isSet()) filter.setFrom(params().from());
  if (params().to.isSet())  filter.setTo(params().to());
  if (params().tolerance.isSet()) filter.setTolerance(params().tolerance());
  if (params().channels.isSet()) filterMgr.setTarget(params().channels());

#ifdef ENABLE_UI
  if (ui) {
    ReplaceColorWindow window(filter, filterMgr);
    if (window.doModal()) {
      set_config_color(ConfigSection, "From", filter.getFrom());
      set_config_color(ConfigSection, "To", filter.getTo());
      set_config_int(ConfigSection, "Tolerance", filter.getTolerance());
    }
  }
  else
#endif // ENABLE_UI
  {
    start_filter_worker(&filterMgr);
  }
}

Command* CommandFactory::createReplaceColorCommand()
{
  return new ReplaceColorCommand;
}

} // namespace app
