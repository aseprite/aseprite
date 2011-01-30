/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "gui/manager.h"
#include "gui/frame.h"

#include "app.h"
#include "gfxmode.h"
#include "console.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "sprite_wrappers.h"
#include "ui_context.h"

//////////////////////////////////////////////////////////////////////
// GfxMode

GfxMode::GfxMode()
{
  m_card    = 0;
  m_width   = 0;
  m_height  = 0;
  m_depth   = 0;
  m_scaling = 0;
}

void GfxMode::updateWithCurrentMode()
{
  m_card    = gfx_driver->id;
  m_width   = SCREEN_W;
  m_height  = SCREEN_H;
  m_depth   = bitmap_color_depth(screen);
  m_scaling = get_screen_scaling();
}

bool GfxMode::setGfxMode() const
{
  // Try change the new graphics mode
  set_color_depth(m_depth);
  set_screen_scaling(m_scaling);

  // Set the mode
  if (set_gfx_mode(m_card, m_width, m_height, 0, 0) < 0) {
    // Error setting the new mode
    return false;
  }

  gui_setup_screen(true);

  // Set to a black palette
  set_black_palette();

  // Restore palette all screen stuff
  {
    const CurrentSpriteReader sprite(UIContext::instance());
    app_refresh_screen(sprite);
  }

  // Redraw top window
  if (app_get_top_window()) {
    app_get_top_window()->remap_window();
    jmanager_refresh_screen();
  }

  return true;
}

//////////////////////////////////////////////////////////////////////
// CurrentGfxModeGuard

CurrentGfxModeGuard::CurrentGfxModeGuard()
{
  m_oldMode.updateWithCurrentMode();
  m_keep = true;
}

CurrentGfxModeGuard::~CurrentGfxModeGuard()
{
  if (!m_keep)
    tryGfxMode(m_oldMode);
}

bool CurrentGfxModeGuard::tryGfxMode(const GfxMode& newMode)
{
  m_keep = false;

  // Try to set the new graphics mode
  if (!newMode.setGfxMode()) {
    // Error!, well, we need to return to the old graphics mode
    if (!m_oldMode.setGfxMode()) {
      // Oh no! more errors!, we can't restore the old graphics mode!
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("FATAL ERROR: Unable to restore the old graphics mode!\n");
      exit(1);
    }
    // Only print a message of the old error
    else {
      Console console;
      console.printf("Error setting graphics mode: %dx%d %d bpp\n",
		     newMode.getWidth(), newMode.getHeight(), newMode.getDepth());
      return false;
    }
  }
  return true;
}

void CurrentGfxModeGuard::keep()
{
  m_keep = true;
}

const GfxMode& CurrentGfxModeGuard::getOriginal() const
{
  return m_oldMode;
}
