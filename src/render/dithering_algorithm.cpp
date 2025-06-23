// Aseprite
// Copyright (C) 2025 Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include <algorithm>
#include <string>
#include <unordered_map>

#include "base/string.h"
#include "render/dithering_algorithm.h"

namespace render {

static const std::unordered_map<DitheringAlgorithm, std::string> names = {
  { DitheringAlgorithm::None,              "none"                },
  { DitheringAlgorithm::Ordered,           "ordered"             },
  { DitheringAlgorithm::Old,               "old"                 },
  { DitheringAlgorithm::FloydSteinberg,    "floyd-steinberg"     },
  { DitheringAlgorithm::FloydSteinberg,    "error-diffusion"     }, // alias
  { DitheringAlgorithm::JarvisJudiceNinke, "jarvis-judice-ninke" },
  { DitheringAlgorithm::Stucki,            "stucki"              },
  { DitheringAlgorithm::Atkinson,          "atkinson"            },
  { DitheringAlgorithm::Burkes,            "burkes"              },
  { DitheringAlgorithm::Sierra,            "sierra"              },
};

bool DitheringAlgorithmIsDiffusion(DitheringAlgorithm algo)
{
  switch (algo) {
    case DitheringAlgorithm::None:
    case DitheringAlgorithm::Ordered:
    case DitheringAlgorithm::Old:     return false;
    default:                          return true;
  }
}

std::string DitheringAlgorithmToString(DitheringAlgorithm algo)
{
  auto it = names.find(algo);
  return (it != names.end()) ? it->second : "unknown";
}

const DitheringAlgorithm DitheringAlgorithmFromString(const std::string name)
{
  auto it = std::find_if(names.begin(), names.end(), [name](const auto& pair) {
    return base::utf8_icmp(pair.second, name) == 0;
  });
  return (it != names.end()) ? it->first : DitheringAlgorithm::Unknown;
}

} // namespace render
