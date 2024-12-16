// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/optional_alert.h"

#include "app/i18n/strings.h"
#include "ui/alert.h"
#include "ui/button.h"

namespace app {

// static
int OptionalAlert::show(Option<bool>& option, const int optionWhenDisabled, const std::string& msg)
{
  if (!option())
    return optionWhenDisabled;

  ui::AlertPtr alert(ui::Alert::create(msg));
  ui::CheckBox* cb = alert->addCheckBox(Strings::general_dont_show());
  const int ret = alert->show();
  if (ret == optionWhenDisabled)
    option(!cb->isSelected());
  return ret;
}

} // namespace app
