// Aseprite
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/configure_timeline_popup.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/loop_tag.h"
#include "app/transaction.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "base/scoped_value.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/slider.h"
#include "ui/theme.h"

#include "timeline_conf.xml.h"

namespace app {

using namespace ui;

ConfigureTimelinePopup::ConfigureTimelinePopup()
  : PopupWindow("Timeline Settings", ClickBehavior::CloseOnClickInOtherWindow)
  , m_lockUpdates(false)
{
  // TODO we should add a new hot region to automatically close the
  //      popup if the mouse is moved outside or find other kind of
  //      dialog/window
  setHotRegion(gfx::Region(manager()->bounds())); // for the color selector

  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  m_box = new app::gen::TimelineConf();
  addChild(m_box);

  m_box->position()->ItemChange.connect([this]{ onChangePosition(); });
  m_box->firstFrame()->Change.connect([this]{ onChangeFirstFrame(); });
  m_box->merge()->Click.connect([this]{ onChangeType(); });
  m_box->tint()->Click.connect([this]{ onChangeType(); });
  m_box->opacity()->Change.connect([this]{ onOpacity(); });
  m_box->opacityStep()->Change.connect([this]{ onOpacityStep(); });
  m_box->resetOnionskin()->Click.connect([this]{ onResetOnionskin(); });
  m_box->loopTag()->Click.connect([this]{ onLoopTagChange(); });
  m_box->currentLayer()->Click.connect([this]{ onCurrentLayerChange(); });
  m_box->behind()->Click.connect([this]{ onPositionChange(); });
  m_box->infront()->Click.connect([this]{ onPositionChange(); });

  m_box->zoom()->Change.connect([this]{ onZoomChange(); });
  m_box->thumbEnabled()->Click.connect([this]{ onThumbEnabledChange(); });
  m_box->thumbOverlayEnabled()->Click.connect([this]{ onThumbOverlayEnabledChange(); });
  m_box->thumbOverlaySize()->Change.connect([this]{ onThumbOverlaySizeChange(); });

  const bool visibleThumb = docPref().thumbnails.enabled();
  m_box->thumbHSeparator()->setVisible(visibleThumb);
  m_box->thumbBox()->setVisible(visibleThumb);
}

Doc* ConfigureTimelinePopup::doc()
{
  return UIContext::instance()->activeDocument();
}

DocumentPreferences& ConfigureTimelinePopup::docPref()
{
  return Preferences::instance().document(doc());
}

void ConfigureTimelinePopup::updateWidgetsFromCurrentSettings()
{
  DocumentPreferences& docPref = this->docPref();
  base::ScopedValue lockUpdates(m_lockUpdates, true);

  auto position = Preferences::instance().general.timelinePosition();
  int selItem = 2;
  switch (position) {
    case gen::TimelinePosition::LEFT: selItem = 0; break;
    case gen::TimelinePosition::RIGHT: selItem = 1; break;
    case gen::TimelinePosition::BOTTOM: selItem = 2; break;
  }
  m_box->position()->setSelectedItem(selItem, false);

  m_box->firstFrame()->setTextf(
    "%d", docPref.timeline.firstFrame());

  switch (docPref.onionskin.type()) {
    case app::gen::OnionskinType::MERGE:
      m_box->merge()->setSelected(true);
      break;
    case app::gen::OnionskinType::RED_BLUE_TINT:
      m_box->tint()->setSelected(true);
      break;
  }
  m_box->opacity()->setValue(docPref.onionskin.opacityBase());
  m_box->opacityStep()->setValue(docPref.onionskin.opacityStep());
  m_box->loopTag()->setSelected(docPref.onionskin.loopTag());
  m_box->currentLayer()->setSelected(docPref.onionskin.currentLayer());

  switch (docPref.onionskin.type()) {
    case app::gen::OnionskinType::MERGE:
      m_box->merge()->setSelected(true);
      break;
    case app::gen::OnionskinType::RED_BLUE_TINT:
      m_box->tint()->setSelected(true);
      break;
  }

  switch (docPref.onionskin.position()) {
    case render::OnionskinPosition::BEHIND:
      m_box->behind()->setSelected(true);
      break;
    case render::OnionskinPosition::INFRONT:
      m_box->infront()->setSelected(true);
      break;
  }

  const bool visibleThumb = docPref.thumbnails.enabled();

  m_box->zoom()->setValue(int(docPref.thumbnails.zoom())); // TODO add a slider for floating points
  m_box->thumbEnabled()->setSelected(visibleThumb);
  m_box->thumbHSeparator()->setVisible(visibleThumb);
  m_box->thumbBox()->setVisible(visibleThumb);
  m_box->thumbOverlayEnabled()->setSelected(docPref.thumbnails.overlayEnabled());
  m_box->thumbOverlaySize()->setValue(docPref.thumbnails.overlaySize());

  expandWindow(sizeHint());
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

void ConfigureTimelinePopup::onChangePosition()
{
  gen::TimelinePosition newTimelinePos =
    gen::TimelinePosition::BOTTOM;

  int selITem = m_box->position()->selectedItem();
  switch (selITem) {
    case 0: newTimelinePos = gen::TimelinePosition::LEFT; break;
    case 1: newTimelinePos = gen::TimelinePosition::RIGHT; break;
    case 2: newTimelinePos = gen::TimelinePosition::BOTTOM; break;
  }
  Preferences::instance().general.timelinePosition(newTimelinePos);
}

void ConfigureTimelinePopup::onChangeFirstFrame()
{
  docPref().timeline.firstFrame(
    m_box->firstFrame()->textInt());
}

void ConfigureTimelinePopup::onChangeType()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.type(m_box->merge()->isSelected() ?
    app::gen::OnionskinType::MERGE:
    app::gen::OnionskinType::RED_BLUE_TINT);
}

void ConfigureTimelinePopup::onOpacity()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.opacityBase(m_box->opacity()->getValue());
}

void ConfigureTimelinePopup::onOpacityStep()
{
  if (m_lockUpdates)
    return;

  docPref().onionskin.opacityStep(m_box->opacityStep()->getValue());
}

void ConfigureTimelinePopup::onResetOnionskin()
{
  DocumentPreferences& docPref = this->docPref();

  docPref.onionskin.type(docPref.onionskin.type.defaultValue());
  docPref.onionskin.opacityBase(docPref.onionskin.opacityBase.defaultValue());
  docPref.onionskin.opacityStep(docPref.onionskin.opacityStep.defaultValue());
  docPref.onionskin.loopTag(docPref.onionskin.loopTag.defaultValue());
  docPref.onionskin.currentLayer(docPref.onionskin.currentLayer.defaultValue());
  docPref.onionskin.position(docPref.onionskin.position.defaultValue());

  updateWidgetsFromCurrentSettings();
}

void ConfigureTimelinePopup::onLoopTagChange()
{
  docPref().onionskin.loopTag(m_box->loopTag()->isSelected());
}

void ConfigureTimelinePopup::onCurrentLayerChange()
{
  docPref().onionskin.currentLayer(m_box->currentLayer()->isSelected());
}

void ConfigureTimelinePopup::onPositionChange()
{
  docPref().onionskin.position(m_box->behind()->isSelected() ?
                               render::OnionskinPosition::BEHIND:
                               render::OnionskinPosition::INFRONT);
}

void ConfigureTimelinePopup::onZoomChange()
{
  docPref().thumbnails.zoom(m_box->zoom()->getValue());
}

void ConfigureTimelinePopup::onThumbEnabledChange()
{
  docPref().thumbnails.enabled(m_box->thumbEnabled()->isSelected());
  updateWidgetsFromCurrentSettings();
}

void ConfigureTimelinePopup::onThumbOverlayEnabledChange()
{
  docPref().thumbnails.overlayEnabled(m_box->thumbOverlayEnabled()->isSelected());
}

void ConfigureTimelinePopup::onThumbOverlaySizeChange()
{
  docPref().thumbnails.overlaySize(m_box->thumbOverlaySize()->getValue());
}

} // namespace app
