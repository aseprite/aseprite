// Aseprite View Library
// Copyright (c) 2020-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef VIEW_LAYERS_H_INCLUDED
#define VIEW_LAYERS_H_INCLUDED
#pragma once

namespace doc {
  class Layer;
}

namespace view {

  // Calculates a possible candidate to be selected in case that we
  // have a specific "selectedLayer" and are going to delete the given
  // "layerToDelete".
  doc::Layer* candidate_if_layer_is_deleted(
    const doc::Layer* selectedLayer,
    const doc::Layer* layerToDelete);

} // namespace view

#endif  // VIEW_LAYERS_H_INCLUDED
