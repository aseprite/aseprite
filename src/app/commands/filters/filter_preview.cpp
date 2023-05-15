// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_preview.h"

#include "app/commands/filters/filter_manager_impl.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_render.h"
#include "base/thread.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/widget.h"

namespace app {

using namespace ui;
using namespace filters;

FilterPreview::FilterPreview(FilterManagerImpl* filterMgr)
  : Widget(kGenericWidget)
  , m_filterMgr(filterMgr)
  , m_timer(1, this)
  , m_restartPreviewTimer(10)
{
  setVisible(false);

  m_restartPreviewTimer.Tick.connect([this]{
    onDelayedStartPreview();
    m_restartPreviewTimer.stop();
  });
}

FilterPreview::~FilterPreview()
{
  stop();
}

void FilterPreview::setEnablePreview(bool state)
{
  if (state) {
    Editor::renderEngine().setPreviewImage(
      m_filterMgr->layer(),
      m_filterMgr->frame(),
      m_filterMgr->destinationImage(),
      nullptr,
      m_filterMgr->position(),
      static_cast<doc::LayerImage*>(m_filterMgr->layer())->blendMode());
  }
  else {
    Editor::renderEngine().removePreviewImage();
  }
}

void FilterPreview::stop()
{
  // Cancel the token (so the filter processing stops immediately)
  m_filterTask.cancel();

  // Stop the filter and timer to flush changes to the screen.
  {
    std::scoped_lock lock(m_filterMgrMutex);
    if (m_timer.isRunning()) {
      ASSERT(m_filterMgr);
      m_filterMgr->end();
    }
    m_timer.stop();
  }

  // Wait the filter task to end.
  if (m_filterTask.running())
    m_filterTask.wait();
}

void FilterPreview::restartPreview()
{
  stop();

  // Start the timer to re-launch the filter preview in the next 10
  // milliseconds. This is necessary to avoid restarting the preview
  // immediately when a lot of mouse events are received at the same
  // time, so we can group several events in one restart.
  //
  // E.g. this can happen when the user scrolls the preview or when
  // changes a filter parameter (e.g. tolerance) and the operating
  // system (e.g. Linux) creates a lot of mouse events/change filter
  // param events.
  m_restartPreviewTimer.start();
}

void FilterPreview::onDelayedStartPreview()
{
  // Start the filter for preview purposes and the timer to flush the
  // preview to the editor/display.
  m_filterMgr->beginForPreview();
  m_timer.start();

  if (!m_filterTask.running()) {
    // Re-start the task to apply the filter step by step
    m_filterTask.run([this](base::task_token& token) {
      m_filterMgr->setTaskToken(token);
      onFilterTask(token);
    });
  }
  else {
    // Is it possible to happen? We cancel the token, the task is
    // still running, we don't launch it again, and we are here.
    ASSERT(false);
  }
}

bool FilterPreview::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      setEnablePreview(true);
      break;

    case kCloseMessage:
      setEnablePreview(false);

      // Stop the preview timer.
      {
        std::scoped_lock lock(m_filterMgrMutex);
        m_timer.stop();
      }
      break;

    case kTimerMessage: {
      std::scoped_lock lock(m_filterMgrMutex);
      if (m_filterMgr) {
        m_filterMgr->flush();
        if (m_filterTask.completed())
          m_timer.stop();
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

// This is executed in other thread.
void FilterPreview::onFilterTask(base::task_token& token)
{
  while (!token.canceled()) {
    {
      std::scoped_lock lock(m_filterMgrMutex);
      if (!m_filterMgr->applyStep())
        token.cancel();
    }
    base::this_thread::yield();
  }
}

} // namespace app
