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

#include "Vaca/String.h"
#include "Vaca/System.h"

#include "core/core.h"
#include "core/check_args.h"
#include "core/cfg.h"
#include "console.h"

using namespace Vaca;
  
CheckArgs::CheckArgs()
{
  const std::vector<String>& args(System::getArgs());

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
	ase_mode |= MODE_VERBOSE;
      }
      // Show help
      else if (option == L"help") {
	usage(true);
      }
      // Show version
      else if (option == L"version") {
	ase_mode |= MODE_BATCH;

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

  // GUI is the default mode
  if (!(ase_mode & MODE_BATCH))
    ase_mode |= MODE_GUI;
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

  ase_mode |= MODE_BATCH;

  // show options
  if (showHelp) {
    // copyright
    console.printf
      ("%s v%s | animated sprite editor\n%s\n\n",
       PACKAGE, VERSION, COPYRIGHT);

    // usage
    console.printf
      ("%s\n  %s [%s] [%s]...\n\n",
       _("Usage:"), m_exeName.c_str(), _("OPTION"), _("FILE"));

    /* options */
    console.printf
      ("%s:\n"
       "  -palette GFX-FILE        %s\n"
       "  -resolution WxH[xBPP]    %s\n"
       "  -verbose                 %s\n"
       "  -help                    %s\n"
       "  -version                 %s\n"
       "\n",
       _("Options"),
       _("Use a specific palette by default"),
       _("Change the resolution to use"),
       _("Explain what is being done (in stderr or a log file)"),
       _("Display this help and exits"),
       _("Output version information and exit"));

    /* web-site */
    console.printf
      ("Find more information in %s web site: %s\n\n",
       PACKAGE, WEBSITE);
  }
  /* how to show options */
  else {
    console.printf(_("Try \"%s --help\" for more information.\n"), 
		   m_exeName.c_str());
  }
}
