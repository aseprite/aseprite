/* Aseprite
 * Copyright (C) 2001-2025  David Capello
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

#include "app/app.h"
#include "app/commands/command.h"
#include "app/color.h"
#include "app/context.h"
#include "app/pref/preferences.h"
#include "app/ui/color_bar.h"
#include "app/ui/status_bar.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/view.h"

#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#endif
#ifdef __linux__
    #include <X11/Xlib.h>
#endif


namespace app {

class OffCanvasSelectCommand : public Command {
public:
  OffCanvasSelectCommand();

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

OffCanvasSelectCommand::OffCanvasSelectCommand()
  : Command("OffCanvasSelect",CmdUIOnlyFlag)
{
}

bool OffCanvasSelectCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

bool OffCanvasSelectCommand::onChecked(Context* context)
{
  return false;
}

void OffCanvasSelectCommand::onExecute(Context* context)
{
  // Get current mouse position in screen coordinates
  gfx::Point mousePos = ui::get_mouse_position();
  app::Color color;

#ifdef _WIN32
  // Windows implementation
  HDC hdc = GetDC(NULL);
  if (hdc) {
    COLORREF c = GetPixel(hdc, mousePos.x, mousePos.y);
    color = app::Color::fromRgb(
      GetRValue(c),
      GetGValue(c),
      GetBValue(c),
      255); // Fully opaque
    ReleaseDC(NULL, hdc);
  }
#elif __linux__
    // ---- Linux/X11 code starts here ----
    //I don't use linux, and I can't find a inbuilt library that will always exist for getting
    //the mouse location adn the screen pixel. If you know one, please tell em and I'll add it
    color = app::Color::fromRgb(255,0,255);  //placeholder 



#else
  color = app::Color::fromRgb(255,0,255);  //placeholder incase there are any isusues
#endif

  // Set the foreground color in Aseprite's color bar
  if (color != app::Color::fromMask()) {
    ColorBar::instance()->setFgColor(color);
    }
}

Command* CommandFactory::createOffCanvasSelectCommand()
{
  return new OffCanvasSelectCommand;
}

} // namespace app