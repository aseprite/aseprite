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
{
  setVisible(false);
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
  {
    std::scoped_lock lock(m_filterMgrMutex);
    if (m_timer.isRunning()) {
      ASSERT(m_filterMgr);
      m_filterMgr->end();
    }
    m_timer.stop();
  }
  if (m_filterTask.running()) {
    m_filterTask.cancel();
    m_filterTask.wait();
  }
}

void FilterPreview::restartPreview()
{
  std::scoped_lock lock(m_filterMgrMutex);

  // Restart filter, timer, and task if needed (there is no need to
  // restart the task if it's already running)
  if (m_timer.isRunning()) {
    ASSERT(m_filterMgr);
    m_filterMgr->end();
  }
  m_filterMgr->beginForPreview();
  m_timer.start();

  if (!m_filterTask.running()) {
    m_filterTask.run([this](base::task_token& token){
      onFilterTask(token);
    });
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
