// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_RGBMAP_BASE_H_INCLUDED
#define DOC_RGBMAP_BASE_H_INCLUDED
#pragma once

#include "doc/fit_criteria.h"
#include "doc/palette.h"

namespace doc {

class RgbMapBase {
public:
  int findBestfit(int r, int g, int b, int a,
                  int mask_index) const;
private:
  void rgbToOtherSpace(double& r, double& g, double& b) const;

protected:
  FitCriteria m_fitCriteria;
  const Palette* m_palette = nullptr;
  int m_modifications = 0;
  int m_maskIndex = 0;
};

} // namespace doc

#endif
