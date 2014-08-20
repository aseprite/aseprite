/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/send_crash.h"

#include "app/app.h"
#include "app/resource_finder.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/launcher.h"

#include "generated_send_crash.h"

namespace app {

const char* SendCrash::kDefaultCrashName = "Aseprite-crash.dmp";

void SendCrash::search()
{
#ifdef WIN32
  app::ResourceFinder rf;
  rf.includeHomeDir(kDefaultCrashName);
  m_dumpFilename = rf.defaultFilename();

  if (base::is_file(m_dumpFilename)) {
    App::instance()->showNotification(this);
  }
#endif
}

std::string SendCrash::notificationText()
{
  return "Report last crash";
}

void SendCrash::notificationClick()
{
  if (m_dumpFilename.empty()) {
    ui::Alert::show("Crash Report<<Nothing to report||&OK");
    return;
  }

  app::gen::SendCrash dlg;
  dlg.filename()->setText(m_dumpFilename);
  dlg.filename()->Click.connect(Bind(&SendCrash::onClickFilename, this));

  dlg.openWindowInForeground();
  if (dlg.getKiller() == dlg.deleteFile()) {
    try {
      base::delete_file(m_dumpFilename);
      m_dumpFilename = "";
    }
    catch (const std::exception& ex) {
      ui::Alert::show("Error<<%s||&OK", ex.what());
    }
  }
}

void SendCrash::onClickFilename()
{
  base::launcher::open_folder(m_dumpFilename);
}

} // namespace app
