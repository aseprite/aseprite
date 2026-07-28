// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/app.h"
#include "app/extensions.h"
#include "app/i18n/strings.h"
#include "app/script/about_extension_window.h"
#include "app/script/engine.h"
#include "app/script/permissions.h"
#include "app/ui/skin/skin_theme.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "clip/clip.h"
#include "ui/alert.h"
#include "ui/menu.h"
#include "ui/system.h"
#include "ui/view.h"

#include "app/script/permission_dialog.h"

namespace app::script {
PermissionDialog::PermissionDialog(const std::string& origin,
                                   const std::string& extensionName,
                                   const Permission permission,
                                   const std::string& match)
  : m_isExtension(!extensionName.empty())
  , m_timer(1000)
  , m_scaryAllowCooldown(4000) // TODO: Configurable?
{
  if (m_isExtension) {
    originLink()->setText(extensionName);
    originLink()->Click.connect([extensionName] {
      if (const auto* ext = App::instance()->extensions().find(extensionName))
        AboutExtensionWindow::show(ext);
    });
    requiresPreamble()->setText(Strings::script_access_the_extension());
  }
  else {
    originLink()->setText(base::get_file_name(origin));
    originLink()->Click.connect(
      [origin] { base::launcher::open_file(base::get_file_path(origin)); });
    tooltipManager()->addTooltipFor(originLink(), origin, ui::BOTTOM);
    requiresPreamble()->setText(Strings::script_access_the_script());
  }

  if (permission == Permission::IORead || permission == Permission::IOWrite ||
      permission == Permission::SpriteRead || permission == Permission::SpriteWrite) {
    remember()->setVisible(false);
    rememberFile()->setVisible(true);
    rememberDirectory()->setVisible(true);
  }

  if (permission == Permission::LoadLib || permission == Permission::Execute ||
      permission == Permission::Network || permission == Permission::Preferences) {
    rememberExtra()->setVisible(true);
  }

  permissionWarning()->setVisible(permission_is_scary(permission));

  if (match.empty()) {
    matchContainer()->setVisible(false);
  }
  else {
    matchText()->setText(match);
    matchText()->selectAll();
    matchText()->selectionClear();

    // Ensures we can see the most important part of long paths
    execute_from_ui_thread([this] {
      auto* view = View::getView(matchText());
      view->setViewScroll(view->viewportBounds().point2());
    });

    matchButtons()->ItemChange.connect([this, match](ButtonSet::Item*) {
      if (matchButtons()->selectedItem() == 0)
        clip::set_text(match);
      else
        base::launcher::open_folder(match);
      matchButtons()->setSelectedItem(nullptr);
    });
  }

  const std::string& who = m_isExtension ? Strings::script_access_extension() :
                                           Strings::script_access_script();
  switch (permission) {
    case Permission::Network:
      wants()->setText(Strings::script_access_wants_network());
      permissionLabel()->setText(Strings::script_access_permission_network());
      rememberExtra()->setText(Strings::script_access_remember_network());
      matchButtons()->getItem(1)->setEnabled(false);
      break;
    case Permission::SpriteRead:
      wants()->setText(Strings::script_access_wants_sprite_read());
      permissionLabel()->setText(Strings::script_access_permission_sprite_read());
      break;
    case Permission::SpriteWrite:
      wants()->setText(Strings::script_access_wants_sprite_write());
      permissionLabel()->setText(Strings::script_access_permission_sprite_write());
      break;
    case Permission::IORead:
      wants()->setText(Strings::script_access_wants_io_read());
      permissionLabel()->setText(Strings::script_access_permission_io_read());
      permissionWarning()->setText(Strings::script_access_permission_io_read_warning(who));
      break;
    case Permission::IOWrite:
      wants()->setText(Strings::script_access_wants_io_write());
      permissionLabel()->setText(Strings::script_access_permission_io_write());
      permissionWarning()->setText(Strings::script_access_permission_io_write_warning(who));
      break;
    case Permission::ClipboardRead:
      permissionLabel()->setText(Strings::script_access_permission_clipboard_read());
      break;
    case Permission::ClipboardWrite:
      permissionLabel()->setText(Strings::script_access_permission_clipboard_write());
      break;
    case Permission::TemporaryFile:
      permissionLabel()->setText(Strings::script_access_permission_temporary_file());
      break;
    case Permission::Preferences:
      permissionLabel()->setText(Strings::script_access_permission_preferences());
      rememberExtra()->setText(Strings::script_access_remember_preferences());
      matchButtons()->getItem(1)->setEnabled(false);
      break;
    case Permission::Debug:
      permissionLabel()->setText(Strings::script_access_permission_debug());
      break;
    case Permission::Bytecode:
      permissionLabel()->setText(Strings::script_access_permission_bytecode());
      permissionWarning()->setText(Strings::script_access_permission_bytecode_warning(who));
      break;
    case Permission::Execute:
      wants()->setText(Strings::script_access_wants_execute());
      permissionLabel()->setText(Strings::script_access_permission_execute());
      permissionWarning()->setText(Strings::script_access_permission_execute_warning(who));
      rememberExtra()->setText(Strings::script_access_remember_execute());
      break;
    case Permission::LoadLib:
      wants()->setText(Strings::script_access_wants_loadlib());
      permissionLabel()->setText(Strings::script_access_permission_loadlib());
      permissionWarning()->setText(Strings::script_access_permission_loadlib_warning(who));
      rememberExtra()->setText(Strings::script_access_remember_loadlib());
      break;
    case Permission::Unknown: throw std::runtime_error("Invalid permission");
  }

  if (base::is_file(match)) {
    matchExtra()->setText(Strings::script_access_file_exists());
    matchExtra()->setVisible(true);
  }

  wants()->setVisible(!wants()->text().empty());
  dotdotdot()->Click.connect(&PermissionDialog::showPopup, this);

  if (permission_is_scary(permission)) {
    permissionIcon()->setSurface(skin::SkinTheme::get(this)->parts.warningBox()->bitmapRef(0));

    if (m_scaryAllowCooldown > 0) {
      const std::string& original = allow()->text();
      auto tick = [this, original] {
        m_scaryAllowCooldown -= m_timer.interval();
        if (m_scaryAllowCooldown <= 0) {
          allow()->setText(original);
          allow()->setEnabled(true);
          m_timer.stop();
        }
        else {
          allow()->setText(
            fmt::format("{} ({})", original, std::ceil(m_scaryAllowCooldown / 1000.0)));
        }
      };
      m_scaryAllowCooldown += m_timer.interval();
      tick();
      m_timer.Tick.connect(tick);
      m_timer.start();
    }
    else {
      allow()->setEnabled(true);
    }
  }
  else {
    allow()->setEnabled(true);
  }
}

std::pair<bool, PermissionDialog::Remember> PermissionDialog::ask()
{
  openWindowInForeground();

  if (closer() == dotdotdot())
    return std::make_pair(true, Remember::FullAccess);

  auto shouldRemember = Remember::Nothing;
  if (remember()->isSelected())
    shouldRemember = matchText()->text().empty() ? Remember::Permission : Remember::Match;
  if (rememberFile()->isSelected())
    shouldRemember = Remember::Match;
  if (rememberDirectory()->isSelected())
    shouldRemember = Remember::Directory;
  if (rememberExtra()->isSelected())
    shouldRemember = Remember::AllOfType;

  return std::make_pair(closer() == allow(), shouldRemember);
}

void PermissionDialog::showPopup()
{
  Menu menu;
  MenuItem fullAccess(Strings::script_access_give_full_access());
  fullAccess.Click.connect([this] {
    if (Alert::show(Strings::alerts_permissions_full_access(
          m_isExtension ? Strings::script_access_extension() : Strings::script_access_script())) ==
        1)
      closeWindow(dotdotdot());
  });
  MenuItem help(Strings::script_access_help());
  help.Click.connect(
    [] { base::launcher::open_url("https://www.aseprite.org/docs/preferences#security"); });

  menu.addChild(&fullAccess);
  menu.addChild(new MenuSeparator);
  menu.addChild(&help);

  const auto& bounds = dotdotdot()->bounds();
  menu.showPopup(gfx::Point(bounds.x, bounds.y2()), display());
}
} // namespace app::script
