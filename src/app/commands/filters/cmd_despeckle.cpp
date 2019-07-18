// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/commands/filters/filter_worker.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/pref/preferences.h"
#include "base/clamp.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/median_filter.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/widget.h"
#include "ui/window.h"

#include "despeckle.xml.h"

#include <stdio.h>

namespace app {

using namespace filters;

struct DespeckleParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<filters::Target> channels { this, 0, "channels" };
  Param<int> width { this, 3, "width" };
  Param<int> height { this, 3, "height" };
  Param<filters::TiledMode> tiledMode { this, filters::TiledMode::NONE, "tiledMode" };
};

#ifdef ENABLE_UI

static const char* ConfigSection = "Despeckle";

class DespeckleWindow : public FilterWindow {
public:
  DespeckleWindow(MedianFilter& filter, FilterManagerImpl& filterMgr)
    : FilterWindow("Median Blur", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithTiledCheckBox,
                   filter.getTiledMode())
    , m_filter(filter)
    , m_controlsWidget(new gen::Despeckle)
    , m_widthEntry(m_controlsWidget->width())
    , m_heightEntry(m_controlsWidget->height())
  {
    getContainer()->addChild(m_controlsWidget.get());

    m_widthEntry->setTextf("%d", m_filter.getWidth());
    m_heightEntry->setTextf("%d", m_filter.getHeight());

    m_widthEntry->Change.connect(&DespeckleWindow::onSizeChange, this);
    m_heightEntry->Change.connect(&DespeckleWindow::onSizeChange, this);
  }

private:
  void onSizeChange() {
    gfx::Size newSize(m_widthEntry->textInt(),
                      m_heightEntry->textInt());

    // Avoid negative numbers
    newSize.w = base::clamp(newSize.w, 1, 100);
    newSize.h = base::clamp(newSize.h, 1, 100);

    m_filter.setSize(newSize.w, newSize.h);
    restartPreview();
  }

  void setupTiledMode(TiledMode tiledMode) override {
    m_filter.setTiledMode(tiledMode);
  }

  MedianFilter& m_filter;
  std::unique_ptr<gen::Despeckle> m_controlsWidget;
  ExprEntry* m_widthEntry;
  ExprEntry* m_heightEntry;
};

#endif  // ENABLE_UI

class DespeckleCommand : public CommandWithNewParams<DespeckleParams> {
public:
  DespeckleCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DespeckleCommand::DespeckleCommand()
  : CommandWithNewParams<DespeckleParams>(CommandId::Despeckle(), CmdRecordableFlag)
{
}

bool DespeckleCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void DespeckleCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif

  MedianFilter filter;
  filter.setSize(3, 3);         // Default size

  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(TARGET_RED_CHANNEL |
                      TARGET_GREEN_CHANNEL |
                      TARGET_BLUE_CHANNEL |
                      TARGET_GRAY_CHANNEL);

#ifdef ENABLE_UI
  if (ui) {
    DocumentPreferences& docPref = Preferences::instance()
      .document(context->activeDocument());
    filter.setTiledMode((filters::TiledMode)docPref.tiled.mode());
    filter.setSize(get_config_int(ConfigSection, "Width", 3),
                   get_config_int(ConfigSection, "Height", 3));
  }
#endif

  if (params().width.isSet()) filter.setSize(params().width(), filter.getHeight());
  if (params().height.isSet()) filter.setSize(filter.getWidth(), params().height());
  if (params().channels.isSet()) filterMgr.setTarget(params().channels());
  if (params().tiledMode.isSet()) filter.setTiledMode(params().tiledMode());

#ifdef ENABLE_UI
  if (ui) {
    DespeckleWindow window(filter, filterMgr);
    if (window.doModal()) {
      set_config_int(ConfigSection, "Width", filter.getWidth());
      set_config_int(ConfigSection, "Height", filter.getHeight());
    }
  }
  else
#endif // ENABLE_UI
  {
    start_filter_worker(&filterMgr);
  }
}

Command* CommandFactory::createDespeckleCommand()
{
  return new DespeckleCommand;
}

} // namespace app
