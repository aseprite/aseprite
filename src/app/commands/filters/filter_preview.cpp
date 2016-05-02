// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_preview.h"

#include "app/commands/filters/filter_manager_impl.h"
#include "app/modules/editors.h"
#include "app/ui/editor/editor.h"
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

void FilterPreview::stop()
{
  if (m_timer.isRunning()) {
    ASSERT(m_filterMgr != NULL);

    m_filterMgr->end();
  }

  m_filterMgr = NULL;
  m_timer.stop();
}

void FilterPreview::restartPreview()
{
  m_filterMgr->beginForPreview();
  m_timer.start();
}

FilterManagerImpl* FilterPreview::getFilterManager() const
{
  return m_filterMgr;
}

bool FilterPreview::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      current_editor->renderEngine().setPreviewImage(
        m_filterMgr->layer(),
        m_filterMgr->frame(),
        m_filterMgr->destinationImage(),
        static_cast<doc::LayerImage*>(m_filterMgr->layer())->blendMode());
      break;

    case kCloseMessage:
      current_editor->renderEngine().removePreviewImage();

      // Stop the preview timer.
      m_timer.stop();
      break;

    case kTimerMessage:
      if (m_filterMgr) {
        if (m_filterMgr->applyStep())
          m_filterMgr->flush();
        else
          m_timer.stop();
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

} // namespace app
