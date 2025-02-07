// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_BEST_FIT_CRITERIA_SELECTOR_H_INCLUDED
#define APP_UI_BEST_FIT_CRITERIA_SELECTOR_H_INCLUDED
#pragma once

#include "doc/fit_criteria.h"
#include "doc/palette.h"
#include "ui/combobox.h"

namespace app {

class BestFitCriteriaSelector : public ui::ComboBox {
public:
  BestFitCriteriaSelector();

  doc::FitCriteria criteria();
  void criteria(doc::FitCriteria criteria);
};

} // namespace app

#endif
