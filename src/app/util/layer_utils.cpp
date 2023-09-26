// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
  if (layer && !layer->isEditableHierarchy()) {
#ifdef ENABLE_UI
    if (auto statusBar = StatusBar::instance())
      statusBar->showTip(
        1000, fmt::format(Strings::statusbar_tips_layer_locked(), layer->name()));
#endif
    return true;
  }
  return false;
}

} // namespace app
