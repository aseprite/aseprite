// Aseprite
// Copyright (c) 2023-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/incompat_file_window.h"

#include "base/trim_string.h"
#include "ui/fit_bounds.h"
#include "ui/manager.h"
#include "ui/scale.h"

namespace app {

void IncompatFileWindow::show(std::string incompatibilities)
{
  base::trim_string(incompatibilities, incompatibilities);
  if (!incompatibilities.empty()) {
    errors()->setText(incompatibilities);

    gfx::Size size(0, errors()->sizeHint().h);
    size.h = std::min(size.h, textHeight() * 16); // 16 lines as max height
    errorsView()->setSizeHint(errorsView()->sizeHint() + errorsView()->horizontalBar()->sizeHint() +
                              size);
  }
  else {
    errorsPlaceholder()->setVisible(false);
  }

  // Run modal

  ui::Display* mainDisplay = ui::Manager::getDefault()->display();
  ui::fit_bounds(mainDisplay,
                 this,
                 gfx::Rect(mainDisplay->size()),
                 [](const gfx::Rect& workarea,
                    gfx::Rect& bounds,
                    std::function<gfx::Rect(Widget*)> getWidgetBounds) {
                   gfx::Point center = bounds.center();
                   bounds.setSize(workarea.size() * 3 / 4);
                   bounds.setOrigin(center - gfx::Point(bounds.size() / 2));
                 });

  openWindowInForeground();
}

} // namespace app
