// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/configure_timeline_popup.h"

#include "app/app.h"
#include "app/context.h"
#include "app/document.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/settings/settings.h"
#include "app/commands/commands.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/message.h"
#include "ui/slider.h"
#include "ui/theme.h"

namespace app {

using namespace ui;

ConfigureTimelinePopup::ConfigureTimelinePopup()
  : PopupWindow("Timeline Settings", kCloseOnClickInOtherWindow)
  , m_lockUpdates(false)
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  addChild(app::load_widget<Box>("timeline_conf.xml", "mainbox"));

  app::finder(this)
    >> "merge" >> m_merge
    >> "tint" >> m_tint
    >> "opacity" >> m_opacity
    >> "opacity_step" >> m_opacityStep
    >> "reset_onionskin" >> m_resetOnionskin
    >> "loop_section" >> m_setLoopSection
    >> "reset_loop_section" >> m_resetLoopSection
    >> "normal" >> m_normalDir
    >> "reverse" >> m_reverseDir
    >> "pingpong" >> m_pingPongDir;

  m_merge->Click.connect(Bind<void>(&ConfigureTimelinePopup::onChangeType, this));
  m_tint->Click.connect(Bind<void>(&ConfigureTimelinePopup::onChangeType, this));
  m_opacity->Change.connect(Bind<void>(&ConfigureTimelinePopup::onOpacity, this));
  m_opacityStep->Change.connect(Bind<void>(&ConfigureTimelinePopup::onOpacityStep, this));
  m_resetOnionskin->Click.connect(Bind<void>(&ConfigureTimelinePopup::onResetOnionskin, this));
  m_setLoopSection->Click.connect(Bind<void>(&ConfigureTimelinePopup::onSetLoopSection, this));
  m_resetLoopSection->Click.connect(Bind<void>(&ConfigureTimelinePopup::onResetLoopSection, this));
  m_normalDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, app::gen::AniDir::FORWARD));
  m_reverseDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, app::gen::AniDir::REVERSE));
  m_pingPongDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, app::gen::AniDir::PING_PONG));
}

DocumentPreferences& ConfigureTimelinePopup::docPref()
{
  return App::instance()->preferences().document(
    UIContext::instance()->activeDocument());
}

void ConfigureTimelinePopup::updateWidgetsFromCurrentSettings()
{
  DocumentPreferences& docPref = this->docPref();
  base::ScopedValue<bool> lockUpdates(m_lockUpdates, true, false);

  switch (docPref.onionskin.type()) {
    case app::gen::OnionskinType::MERGE:
      m_merge->setSelected(true);
      break;
    case app::gen::OnionskinType::RED_BLUE_TINT:
      m_tint->setSelected(true);
      break;
  }
  m_opacity->setValue(docPref.onionskin.opacityBase());
  m_opacityStep->setValue(docPref.onionskin.opacityStep());

  switch (docPref.onionskin.type()) {
    case app::gen::OnionskinType::MERGE:
      m_merge->setSelected(true);
      break;
    case app::gen::OnionskinType::RED_BLUE_TINT:
      m_tint->setSelected(true);
      break;
  }

  switch (docPref.loop.aniDir()) {
    case app::gen::AniDir::FORWARD:
      m_normalDir->setSelected(true);
      break;
    case app::gen::AniDir::REVERSE:
      m_reverseDir->setSelected(true);
      break;
    case app::gen::AniDir::PING_PONG:
      m_pingPongDir->setSelected(true);
      break;
  }
}

bool ConfigureTimelinePopup::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      updateWidgetsFromCurrentSettings();
      break;

    }
  }
  return PopupWindow::onProcessMessage(msg);
}

void ConfigureTimelinePopup::onChangeType()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.type(m_merge->isSelected() ?
    app::gen::OnionskinType::MERGE:
    app::gen::OnionskinType::RED_BLUE_TINT);
}

void ConfigureTimelinePopup::onOpacity()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.opacityBase(m_opacity->getValue());
}

void ConfigureTimelinePopup::onOpacityStep()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.opacityStep(m_opacityStep->getValue());
}

void ConfigureTimelinePopup::onResetOnionskin()
{
  DocumentPreferences& docPref = this->docPref();

  docPref.onionskin.type(docPref.onionskin.type.defaultValue());
  docPref.onionskin.opacityBase(docPref.onionskin.opacityBase.defaultValue());
  docPref.onionskin.opacityStep(docPref.onionskin.opacityStep.defaultValue());

  updateWidgetsFromCurrentSettings();
}

void ConfigureTimelinePopup::onSetLoopSection()
{
  UIContext::instance()->executeCommand(CommandId::SetLoopSection);
}

void ConfigureTimelinePopup::onResetLoopSection()
{
  docPref().loop.visible(false);
}

void ConfigureTimelinePopup::onAniDir(app::gen::AniDir aniDir)
{
  docPref().loop.aniDir(aniDir);
}

} // namespace app
