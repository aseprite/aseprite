// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_PERMISSION_DIALOG_H_INCLUDED
#define APP_SCRIPT_PERMISSION_DIALOG_H_INCLUDED

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/script/permissions.h"

#include "script_access.xml.h"

namespace app::script {

class PermissionDialog : public gen::ScriptAccess {
public:
  enum class Remember : uint8_t { Nothing, Permission, Match, Directory, FullAccess, AllOfType };
  PermissionDialog(const std::string& origin,
                   const std::string& extensionName,
                   Permission permission,
                   const std::string& match);

  std::pair<bool, Remember> ask();

private:
  void showPopup();
  bool m_isExtension;
  ui::Timer m_timer;
  base::tick_t m_scaryAllowCooldown;
};

} // namespace app::script
#endif // APP_SCRIPT_PERMISSION_DIALOG_H_INCLUDED
