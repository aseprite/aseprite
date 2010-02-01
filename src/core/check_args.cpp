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

#include <allegro/unicode.h>

#include "core/core.h"
#include "core/check_args.h"
#include "core/cfg.h"
#include "console.h"
  
CheckArgs::CheckArgs(int argc, char* argv[])
  : m_exe_name(argv[0])
{
  Console console;
  int i, n, len;
  char *arg;

  for (i=1; i<argc; i++) {
    arg = argv[i];

    for (n=0; arg[n] == '-'; n++);
    len = strlen(arg+n);

    /* option */
    if ((n > 0) && (len > 0)) {
      /* use other palette file */
      if (strncmp(arg+n, "palette", len) == 0) {
	if (++i < argc)
	  m_palette_filename = argv[i];
	else
	  usage(false);
      }
      /* video resolution */
      else if (strncmp(arg+n, "resolution", len) == 0) {
	if (++i < argc) {
	  int c, num1=0, num2=0, num3=0;
	  char *tok;

	  /* el próximo argumento debe indicar un formato de
	     resolución algo como esto: 320x240[x8] o [8] */
	  c = 0;

	  for (tok=ustrtok(argv[i], "x"); tok;
	       tok=ustrtok(NULL, "x")) {
	    switch (c) {
	      case 0: num1 = ustrtol(tok, NULL, 10); break;
	      case 1: num2 = ustrtol(tok, NULL, 10); break;
	      case 2: num3 = ustrtol(tok, NULL, 10); break;
	    }
	    c++;
	  }

	  switch (c) {
	    case 1:
	      set_config_int("GfxMode", "Depth", num1);
	      break;
	    case 2:
	    case 3:
	      set_config_int("GfxMode", "Width", num1);
	      set_config_int("GfxMode", "Height", num2);
	      if (c == 3)
		set_config_int("GfxMode", "Depth", num3);
	      break;
	  }
	}
	else {
	  console.printf(_("%s: option \"res\" requires an argument\n"), 
			 m_exe_name);
	  usage(false);
	}
      }
      /* verbose mode */
      else if (strncmp(arg+n, "verbose", len) == 0) {
	ase_mode |= MODE_VERBOSE;
      }
      /* show help */
      else if (strncmp(arg+n, "help", len) == 0) {
	usage(true);
      }
      /* show version */
      else if (strncmp(arg+n, "version", len) == 0) {
	ase_mode |= MODE_BATCH;

	console.printf("ase %s\n", VERSION);
      }
      /* invalid argument */
      else {
	usage(false);
      }
    }
    /* graphic file to open */
    else if (n == 0)
      m_options.push_back(new Option(Option::OpenSprite, argv[i]));
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
}

/**
 * Shows the available options for the program
 */
void CheckArgs::usage(bool show_help)
{
  Console console;

  ase_mode |= MODE_BATCH;

  // show options
  if (show_help) {
    // copyright
    console.printf
      ("ase %s -- Allegro Sprite Editor, %s\n"
       COPYRIGHT "\n\n",
       VERSION, _("A tool to create sprites"));

    // usage
    console.printf
      ("%s\n  %s [%s] [%s]...\n\n",
       _("Usage:"), m_exe_name, _("OPTION"), _("FILE"));

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
      ("%s: %s\n\n",
       _("Find more information in the ASE's official web site at:"), WEBSITE);
  }
  /* how to show options */
  else {
    console.printf(_("Try \"%s --help\" for more information.\n"), 
		   m_exe_name);
  }
}
