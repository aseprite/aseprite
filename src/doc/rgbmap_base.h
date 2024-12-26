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
#include "doc/rgbmap.h"

namespace doc {

class RgbMapBase : public RgbMap {
public:
  int findBestfit(int r, int g, int b, int a, int mask_index) const;

  // RgbMap impl
  int modifications() const override { return m_modifications; }
  int maskIndex() const override { return m_maskIndex; }
  FitCriteria fitCriteria() const override { return m_fitCriteria; }
  void fitCriteria(const FitCriteria fitCriteria) override { m_fitCriteria = fitCriteria; }

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
