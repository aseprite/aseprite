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

#include "Vaca/Application.h"
#include "Vaca/String.h"

#include "check_args.h"
#include "console.h"
#include "core/cfg.h"

using namespace Vaca;
  
CheckArgs::CheckArgs()
  : m_consoleOnly(false)
  , m_verbose(false)
{
  const std::vector<String>& args(Application::getArgs());

  // Exe name
  m_exeName = args[0];

  // Convert arguments to recognized options
  Console console;
  size_t i, n, len;

  for (i=1; i<args.size(); i++) {
    const String& arg(args[i]);

    for (n=0; arg[n] == '-'; n++);
    len = arg.size()-n;

    // Option
    if ((n > 0) && (len > 0)) {
      String option = arg.substr(n);
      
      // Use other palette file
      if (option == L"palette") {
	if (++i < args.size())
	  m_paletteFilename = args[i];
	else
	  usage(false);
      }
      // Video resolution
      else if (option == L"resolution") {
	if (++i < args.size()) {
	  // The following argument should indicate the resolution
	  // in a format like: 320x240[x8]
	  std::vector<String> parts;
	  split_string(args[i], parts, L"x");

	  switch (parts.size()) {
	    case 1:
	      set_config_int("GfxMode", "Depth", convert_to<double>(parts[0]));
	      break;
	    case 2:
	    case 3:
	      set_config_int("GfxMode", "Width", convert_to<double>(parts[0]));
	      set_config_int("GfxMode", "Height", convert_to<double>(parts[1]));
	      if (parts.size() == 3)
		set_config_int("GfxMode", "Depth", convert_to<double>(parts[2]));
	      break;
	    default:
	      usage(false);
	      break;
	  }
	}
	else {
	  console.printf(_("%s: option \"res\" requires an argument\n"), 
			 m_exeName.c_str());
	  usage(false);
	}
      }
      // Verbose mode
      else if (option == L"verbose") {
	m_verbose = true;
      }
      // Show help
      else if (option == L"help") {
	usage(true);
      }
      // Show version
      else if (option == L"version") {
	m_consoleOnly = true;
	console.printf("%s %s\n", PACKAGE, VERSION);
      }
      // Invalid argument
      else {
	usage(false);
      }
    }
    // Graphic file to open
    else if (n == 0) {
      m_options.push_back(new Option(Option::OpenSprite, args[i]));
    }
  }
}

CheckArgs::~CheckArgs()
{
  clear();
}

void CheckArgs::clear()
{
  for (iterator it = begin(); it != end(); ++it) {
    Option* option = *it;
    delete option;
  }
  m_options.clear();
}

// Shows the available options for the program
void CheckArgs::usage(bool showHelp)
{
  Console console;

  // Activate this flag so the GUI is not initialized by the App::run().
  m_consoleOnly = true;

  // Show options
  if (showHelp) {
    // Copyright
    console.printf
      ("%s v%s | Allegro Sprite Editor | A pixel art program\n%s\n\n",
       PACKAGE, VERSION, COPYRIGHT);

    // Usage
    console.printf
      ("%s\n  %s [%s] [%s]...\n\n",
       _("Usage:"), m_exeName.c_str(), _("OPTION"), _("FILE"));

    // Available Options
    console.printf
      ("Options:\n"
       "  -palette GFX-FILE        Use a specific palette by default\n"
       "  -resolution WxH[xBPP]    Change the resolution to use\n"
       "  -verbose                 Explain what is being done (in stderr or a log file)\n"
       "  -help                    Display this help and exits\n"
       "  -version                 Output version information and exit\n"
       "\n");

    // Web-site
    console.printf
      ("Find more information in %s web site: %s\n\n",
       PACKAGE, WEBSITE);
  }
  // How to show options
  else {
    console.printf(_("Try \"%s --help\" for more information.\n"), 
		   m_exeName.c_str());
  }
}
