// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_ENTER_LICENSE_H_INCLUDED
#define APP_UI_ENTER_LICENSE_H_INCLUDED
#pragma once

#include "drm/license_manager.h"
#include "enter_license.xml.h"

namespace app {

class EnterLicense : public app::gen::EnterLicense {
public:
  EnterLicense();

protected:
  void onBeforeClose(ui::CloseEvent& ev) override;
  void onActivationFailed(drm::LicenseManager::Exception& e);
  void onActivated(drm::ActivatedEvent& ev);

private:
  std::thread m_activation;
  ui::Timer m_timer;
  bool m_activationInProgress;
  obs::connection m_activationFailedConn;
  obs::connection m_activatedConn;

  void startActivation();
  void showError(const std::string& msg);
  void showSuccess();
};

}

#endif
