// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/incompat_file_window.h"

#include "base/trim_string.h"

namespace app {

void IncompatFileWindow::show(std::string incompatibilities)
{
  base::trim_string(incompatibilities,
                    incompatibilities);
  if (!incompatibilities.empty()) {
    errors()->setText(incompatibilities);
    errorsView()->setSizeHint(
      errorsView()->border().size()
      + gfx::Size(0, std::min(textHeight()*16, // 16 lines as max height
                              errors()->sizeHint().h)));
  }
  else {
    errorsPlaceholder()->setVisible(false);
  }

  // Run modal
  openWindowInForeground();
}

} // namespace app
