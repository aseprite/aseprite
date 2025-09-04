// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/slice_utils.h"

#include "app/context_access.h"
#include "app/site.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "fmt/format.h"

namespace app {

std::string get_unique_slice_name(const doc::Sprite* sprite, const std::string& namePrefix)
{
  std::string prefix = namePrefix.empty() ? "Slice" : namePrefix;
  int max = 0;

  for (doc::Slice* slice : sprite->slices())
    if (std::strncmp(slice->name().c_str(), prefix.c_str(), prefix.size()) == 0)
      max = std::max(max, (int)std::strtol(slice->name().c_str() + prefix.size(), nullptr, 10));

  return fmt::format("{} {}", prefix, max + 1);
}

std::vector<doc::Slice*> get_selected_slices(const Site& site)
{
  std::vector<Slice*> selectedSlices;
  if (site.sprite() && !site.selectedSlices().empty()) {
    for (auto* slice : site.sprite()->slices()) {
      if (site.selectedSlices().contains(slice->id())) {
        selectedSlices.push_back(slice);
      }
    }
  }
  return selectedSlices;
}

} // namespace app
