/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "app/ui/configure_timeline_popup.h"

#include "app/app.h"
#include "app/context.h"
#include "app/document.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/settings/document_settings.h"
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
  m_normalDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, IDocumentSettings::AniDir_Normal));
  m_reverseDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, IDocumentSettings::AniDir_Reverse));
  m_pingPongDir->Click.connect(Bind<void>(&ConfigureTimelinePopup::onAniDir, this, IDocumentSettings::AniDir_PingPong));
}

IDocumentSettings* ConfigureTimelinePopup::docSettings()
{
  Context* context = UIContext::instance();
  Document* document = context->activeDocument();
  if (!document)
    return NULL;

  return context->settings()->getDocumentSettings(document);
}

void ConfigureTimelinePopup::updateWidgetsFromCurrentSettings()
{
  IDocumentSettings* docSet = docSettings();
  if (!docSet)
    return;

  base::ScopedValue<bool> lockUpdates(m_lockUpdates, true, false);

  switch (docSet->getOnionskinType()) {
    case IDocumentSettings::Onionskin_Merge:
      m_merge->setSelected(true);
      break;
    case IDocumentSettings::Onionskin_RedBlueTint:
      m_tint->setSelected(true);
      break;
  }
  m_opacity->setValue(docSet->getOnionskinOpacityBase());
  m_opacityStep->setValue(docSet->getOnionskinOpacityStep());

  switch (docSet->getOnionskinType()) {
    case IDocumentSettings::Onionskin_Merge:
      m_merge->setSelected(true);
      break;
    case IDocumentSettings::Onionskin_RedBlueTint:
      m_tint->setSelected(true);
      break;
  }

  switch (docSet->getAnimationDirection()) {
    case IDocumentSettings::AniDir_Normal:
      m_normalDir->setSelected(true);
      break;
    case IDocumentSettings::AniDir_Reverse:
      m_reverseDir->setSelected(true);
      break;
    case IDocumentSettings::AniDir_PingPong:
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

  IDocumentSettings* docSet = docSettings();
  if (docSet)
    docSet->setOnionskinType(m_merge->isSelected() ?
      IDocumentSettings::Onionskin_Merge:
      IDocumentSettings::Onionskin_RedBlueTint);
}

void ConfigureTimelinePopup::onOpacity()
{
  if (m_lockUpdates)
    return;

  IDocumentSettings* docSet = docSettings();
  if (docSet)
    docSet->setOnionskinOpacityBase(m_opacity->getValue());
}

void ConfigureTimelinePopup::onOpacityStep()
{
  if (m_lockUpdates)
    return;

  IDocumentSettings* docSet = docSettings();
  if (docSet)
    docSet->setOnionskinOpacityStep(m_opacityStep->getValue());
}

void ConfigureTimelinePopup::onResetOnionskin()
{
  IDocumentSettings* docSet = docSettings();
  if (docSet) {
    docSet->setDefaultOnionskinSettings();
    updateWidgetsFromCurrentSettings();
  }
}

void ConfigureTimelinePopup::onSetLoopSection()
{
  UIContext::instance()->executeCommand(CommandId::SetLoopSection);
}

void ConfigureTimelinePopup::onResetLoopSection()
{
  IDocumentSettings* docSet = docSettings();
  if (docSet)
    docSet->setLoopAnimation(false);
}

void ConfigureTimelinePopup::onAniDir(IDocumentSettings::AniDir aniDir)
{
  IDocumentSettings* docSet = docSettings();
  if (docSet)
    docSet->setAnimationDirection(aniDir);
}

} // namespace app
