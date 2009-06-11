/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <stdio.h>
#include <allegro.h>
#if defined ALLEGRO_WINDOWS && defined DEBUGMODE
#include <winalleg.h>
#include <psapi.h>
#endif

#include "jinete/jsystem.h"

#include "commands/commands.h"
#include "core/app.h"
#include "widgets/statebar.h"

static void cmd_refresh_execute(const char *argument)
{
  jmouse_hide();
  clear_to_color(screen, makecol(0, 0, 0));
  jmouse_show();

  {
    const CurrentSpriteReader sprite;
    app_refresh_screen(sprite);
  }

  /* print memory information */
#if defined ALLEGRO_WINDOWS && defined DEBUGMODE
  {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      statusbar_show_tip(app_get_statusbar(), 1000,
			 "Current memory: %.16g KB (%lu)\n"
			 "Peak of memory: %.16g KB (%lu)",
			 pmc.WorkingSetSize / 1024.0, pmc.WorkingSetSize,
			 pmc.PeakWorkingSetSize / 1024.0, pmc.PeakWorkingSetSize);
    }
  }
#endif
}

Command cmd_refresh = {
  CMD_REFRESH,
  NULL,
  NULL,
  cmd_refresh_execute,
  NULL
};
