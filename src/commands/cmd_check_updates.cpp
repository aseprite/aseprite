/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <allegro.h>
#include <Vaca/String.h>

#include "commands/command.h"
#include "launcher.h"

//////////////////////////////////////////////////////////////////////
// about

class CheckUpdatesCommand : public Command
{
public:
  CheckUpdatesCommand();
  Command* clone() const { return new CheckUpdatesCommand(*this); }

protected:
  void execute(Context* context);
};

CheckUpdatesCommand::CheckUpdatesCommand()
  : Command("check_updates",
	    "Check for New Version",
	    CmdUIOnlyFlag)
{
}

void CheckUpdatesCommand::execute(Context* context)
{
  std::string url;
  url += WEBSITE;
  url += "update/?v=";
  url += VERSION;		// ASE version

  // Operating system
  std::string os;
  switch (os_type) {
    case OSTYPE_WIN3: os = "win3"; break;
    case OSTYPE_WIN95: os = "win95"; break;
    case OSTYPE_WIN98: os = "win98"; break;
    case OSTYPE_WINME: os = "winme"; break;
    case OSTYPE_WINNT: os = "winnt"; break;
    case OSTYPE_WIN2000: os = "win2000"; break;
    case OSTYPE_WINXP: os = "winxp"; break;
    case OSTYPE_WIN2003: os = "win2003"; break;
    case OSTYPE_WINVISTA: os = "winvista"; break;
    case OSTYPE_OS2: os = "os2"; break;
    case OSTYPE_WARP: os = "warp"; break;
    case OSTYPE_DOSEMU: os = "dosemu"; break;
    case OSTYPE_OPENDOS: os = "opendos"; break;
    case OSTYPE_LINUX: os = "linux"; break;
    case OSTYPE_SUNOS: os = "sunos"; break;
    case OSTYPE_FREEBSD: os = "freebsd"; break;
    case OSTYPE_NETBSD: os = "netbsd"; break;
    case OSTYPE_IRIX: os = "irix"; break;
    case OSTYPE_DARWIN: os = "darwin"; break;
    case OSTYPE_QNX: os = "qnx"; break;
    case OSTYPE_UNIX: os = "unix"; break;
    case OSTYPE_BEOS: os = "beos"; break;
    case OSTYPE_MACOS: os = "macos"; break;
    case OSTYPE_MACOSX: os = "macosx"; break;
    default:
      os = "unknown";
      break;
  }
  url += "&os=" + os;

  // Version of the operating system
  if (os_version >= 0) {
    url += "&osver=";
    url += Vaca::convert_to<std::string>(os_version);
  }
  if (os_revision >= 0) {
    url += "&osrev=";
    url += Vaca::convert_to<std::string>(os_revision);
  }

  Launcher::openUrl(url);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_check_updates_command()
{
  return new CheckUpdatesCommand;
}
