// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/best_fit_criteria_selector.h"

#include "app/i18n/strings.h"

namespace app {

BestFitCriteriaSelector::BestFitCriteriaSelector()
{
  // addItem() must match the FitCriteria enum
  static_assert(int(doc::FitCriteria::DEFAULT) == 0 && int(doc::FitCriteria::RGB) == 1 &&
                  int(doc::FitCriteria::linearizedRGB) == 2 && int(doc::FitCriteria::CIEXYZ) == 3 &&
                  int(doc::FitCriteria::CIELAB) == 4,
                "Unexpected doc::FitCriteria values");

  addItem(Strings::best_fit_criteria_selector_default());
  addItem(Strings::best_fit_criteria_selector_rgb());
  addItem(Strings::best_fit_criteria_selector_linearized_rgb());
  addItem(Strings::best_fit_criteria_selector_cie_xyz());
  addItem(Strings::best_fit_criteria_selector_cie_lab());

  criteria(doc::FitCriteria::DEFAULT);
}

doc::FitCriteria BestFitCriteriaSelector::criteria()
{
  return (doc::FitCriteria)getSelectedItemIndex();
}

void BestFitCriteriaSelector::criteria(const doc::FitCriteria criteria)
{
  setSelectedItemIndex((int)criteria);
}

} // namespace app
