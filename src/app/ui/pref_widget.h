// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PREF_WIDGET_H_INCLUDED
#define APP_UI_PREF_WIDGET_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "base/split_string.h"
#include "base/exception.h"
#include "ui/message.h"
#include "ui/register_message.h"

namespace app {

extern ui::RegisterMessage kSavePreferencesMessage;

template<class Base>
class BoolPrefWidget : public Base {
public:
  template<typename...Args>
  BoolPrefWidget(Args&&...args)
    : Base(args...)
    , m_option(nullptr) {
  }

  void setPref(const char* prefString) {
    ASSERT(prefString);

    std::vector<std::string> parts;
    base::split_string(prefString, parts, ".");
    if (parts.size() == 2) {
      auto& pref = Preferences::instance();
      auto section = pref.section(parts[0].c_str());
      if (!section)
        throw base::Exception("Preference section not found: %s", prefString);

      m_option =
        dynamic_cast<Option<bool>*>(
          section->option(parts[1].c_str()));

      if (!m_option)
        throw base::Exception("Preference option not found: %s", prefString);

      // Load option value
      this->setSelected((*m_option)());
    }
  }

  void resetWithDefaultValue() {
    // Reset to default value in preferences
    if (m_option)
      this->setSelected(m_option->defaultValue());
  }

protected:
  bool onProcessMessage(ui::Message* msg) override {
    if (msg->type() == kSavePreferencesMessage) {
      ASSERT(m_option);

      // Update Option value.
      (*m_option)(this->isSelected());
    }
    return Base::onProcessMessage(msg);
  }

private:
  Option<bool>* m_option;
};

} // namespace app

#endif
