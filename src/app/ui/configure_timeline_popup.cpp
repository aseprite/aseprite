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
#include "app/cmd/remove_frame_tag.h"
#include "app/cmd/set_frame_tag_anidir.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/loop_tag.h"
#include "app/settings/settings.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/scoped_value.h"
#include "doc/frame_tag.h"
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
  m_normalDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, doc::AniDir::FORWARD));
  m_reverseDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, doc::AniDir::REVERSE));
  m_pingPongDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, doc::AniDir::PING_PONG));
}

app::Document* ConfigureTimelinePopup::doc()
{
  return UIContext::instance()->activeDocument();
}

DocumentPreferences& ConfigureTimelinePopup::docPref()
{
  return App::instance()->preferences().document(doc());
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

  doc::AniDir aniDir = doc::AniDir::FORWARD;
  if (doc()) {
    if (doc::FrameTag* tag = get_loop_tag(doc()->sprite())) {
      aniDir = tag->aniDir();
    }
  }
  else {
    ASSERT(false && "We should have an active sprite at this moment");
  }

  switch (aniDir) {
    case doc::AniDir::FORWARD:
      m_normalDir->setSelected(true);
      break;
    case doc::AniDir::REVERSE:
      m_reverseDir->setSelected(true);
      break;
    case doc::AniDir::PING_PONG:
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
  ContextWriter writer(UIContext::instance());
  Transaction transaction(writer.context(), "Remove Loop");
  transaction.execute(new cmd::RemoveFrameTag(writer.sprite(),
      get_loop_tag(writer.sprite())));
  transaction.commit();
}

void ConfigureTimelinePopup::onAniDir(doc::AniDir aniDir)
{
  ContextWriter writer(UIContext::instance());
  doc::Sprite* sprite = writer.sprite();
  if (sprite) {
    ASSERT(false);
    return;
  }

  doc::FrameTag* loopTag = get_loop_tag(sprite);
  Transaction transaction(writer.context(), "Set Loop Direction");
  if (loopTag)
    transaction.execute(new cmd::SetFrameTagAniDir(loopTag, aniDir));
  else {
    loopTag = create_loop_tag(doc::frame_t(0), sprite->lastFrame());
    loopTag->setAniDir(aniDir);
    transaction.execute(new cmd::AddFrameTag(sprite, loopTag));
  }
  transaction.commit();
}

} // namespace app
