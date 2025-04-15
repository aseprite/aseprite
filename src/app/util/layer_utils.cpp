// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/layer_utils.h"

#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tilesets.h"
#include "fmt/format.h"
#include <memory>

namespace app {

bool layer_is_locked(Editor* editor)
{
  doc::Layer* layer = editor->layer();
  if (!layer)
    return false;

  auto* statusBar = StatusBar::instance();

  if (!layer->isVisibleHierarchy()) {
    if (statusBar) {
      statusBar->showTip(1000, Strings::statusbar_tips_layer_x_is_hidden(layer->name()));
    }
    return true;
  }

  if (!layer->isEditableHierarchy()) {
    if (statusBar) {
      statusBar->showTip(1000, Strings::statusbar_tips_layer_locked(layer->name()));
    }
    return true;
  }

  return false;
}

std::string get_layer_path(const Layer* layer)
{
  std::string path;
  for (; layer != layer->sprite()->root(); layer = layer->parent()) {
    if (!path.empty())
      path.insert(0, "/");
    path.insert(0, layer->name());
  }
  return path;
}

} // namespace app
