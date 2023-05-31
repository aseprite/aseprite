// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/layer_utils.h"

#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "doc/layer.h"
#include "fmt/format.h"

namespace app {

bool layer_is_locked(Editor* editor)
{
  doc::Layer* layer = editor->layer();
  if (!layer)
    return false;

#ifdef ENABLE_UI
  auto statusBar = StatusBar::instance();
#endif

  if (!layer->isVisibleHierarchy()) {
#ifdef ENABLE_UI
    if (statusBar) {
      statusBar->showTip(
        1000, fmt::format(Strings::statusbar_tips_layer_x_is_hidden(),
                          layer->name()));
    }
#endif
    return true;
  }

  if (!layer->isEditableHierarchy()) {
#ifdef ENABLE_UI
    if (statusBar) {
      statusBar->showTip(
        1000, fmt::format(Strings::statusbar_tips_layer_locked(), layer->name()));
    }
#endif
    return true;
  }

  return false;
}

} // namespace app
