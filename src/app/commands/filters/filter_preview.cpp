// Aseprite
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
#include "base/bind.h"
#include "base/scoped_lock.h"
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
  , m_filterThread(nullptr)
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
    base::scoped_lock lock(m_filterMgrMutex);
    if (m_timer.isRunning()) {
      ASSERT(m_filterMgr);
      m_filterMgr->end();
    }
  }

  m_timer.stop();

  if (m_filterThread) {
    m_filterThread->join();
    m_filterThread.reset();
  }
}

void FilterPreview::restartPreview()
{
  stop();

  base::scoped_lock lock(m_filterMgrMutex);

  m_filterMgr->beginForPreview();
  m_filterIsDone = false;
  m_filterThread.reset(new base::thread(
    base::Bind<void>(&FilterPreview::onFilterThread, this)));

  m_timer.start();
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
      m_timer.stop();
      break;

    case kTimerMessage: {
      base::scoped_lock lock(m_filterMgrMutex);
      if (m_filterMgr) {
        m_filterMgr->flush();
        if (m_filterIsDone)
          m_timer.stop();
      }
      break;
    }
  }

  return Widget::onProcessMessage(msg);
}

// This is executed in other thread.
void FilterPreview::onFilterThread()
{
  while (!m_filterIsDone && m_timer.isRunning()) {
    {
      base::scoped_lock lock(m_filterMgrMutex);
      m_filterIsDone = !m_filterMgr->applyStep();
    }
    base::this_thread::yield();
  }
}

} // namespace app
