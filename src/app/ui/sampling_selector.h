// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FILE_SELECTOR_H_INCLUDED
#define APP_UI_FILE_SELECTOR_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "obs/connection.h"
#include "ui/box.h"
#include "ui/combobox.h"
#include "ui/label.h"

namespace app {

class SamplingSelector : public ui::HBox {
public:
  enum class Behavior { ChangeOnRealTime, ChangeOnSave };

  SamplingSelector(Behavior behavior = Behavior::ChangeOnRealTime);

  void save();

private:
  void onPreferenceChange();

  Behavior m_behavior;
  ui::Label m_downsamplingLabel;
  ui::ComboBox m_downsampling;
  obs::scoped_connection m_samplingChangeConn;
};

} // namespace app

#endif
