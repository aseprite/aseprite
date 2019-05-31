// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/task_widget.h"

#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "base/clamp.h"
#include "ui/scale.h"

#include <limits>

namespace app {

using namespace ui;
using namespace app::skin;

TaskWidget::TaskWidget(const Type type,
                       base::task::func_t&& func)
  : Box(HORIZONTAL | HOMOGENEOUS)
  , m_monitorTimer(25)
  , m_cancelButton("Cancel")
  , m_progressBar(0, 100, 0)
{
  if (int(type) & int(kCanCancel)) {
    addChild(&m_cancelButton);

    m_cancelButton.Click.connect(
      [this](Event&){
        m_task.cancel();
        m_cancelButton.setEnabled(false);
        m_progressBar.setEnabled(false);
      });
  }

  if (int(type) & int(kWithProgress)) {
    m_progressBar.setReadOnly(true);
    addChild(&m_progressBar);
  }

  m_monitorTimer.Tick.connect(
    [this]{
      if (m_task.completed()) {
        m_monitorTimer.stop();
        onComplete();
      }
      else if (m_progressBar.parent()) {
        float v = m_task.progress();
        if (v > 0.0f) {
          TRACEARGS("progressBar setValue",
                    int(base::clamp(v*100.0f, 0.0f, 100.0f)));
          m_progressBar.setValue(
            int(base::clamp(v*100.0f, 0.0f, 100.0f)));
        }
      }
    });
  m_monitorTimer.start();

  InitTheme.connect(
    [this]{
      auto theme = static_cast<SkinTheme*>(this->theme());
      setTransparent(true);
      setBgColor(gfx::ColorNone);
      m_cancelButton.setTransparent(true);
      m_cancelButton.setStyle(theme->styles.miniButton());
      m_cancelButton.setBgColor(gfx::ColorNone);
      m_progressBar.setTransparent(true);
      m_progressBar.setBgColor(gfx::ColorNone);
      setup_mini_font(&m_cancelButton);
      setup_mini_look(&m_progressBar);
      setMaxSize(gfx::Size(std::numeric_limits<int>::max(), textHeight()));
    });
  initTheme();

  m_task.run(std::move(func));
}

void TaskWidget::onComplete()
{
  // Do nothing
}

} // namespace app
