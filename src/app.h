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

#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

#include "jinete/jbase.h"

#include <vector>

class Layer;
class Sprite;
class Params;
class Command;
class CommandsModule;

class AppEvent
{
public:
  // enumeration of ASE events in the highest application level
  enum Type {
    Exit,
    PaletteChange,
    NumEvents
  };
};

class IAppHook
{
public:
  virtual ~IAppHook() { }
  virtual void on_event() = 0;
};

class LegacyModules;

class App
{
  class Modules;

  static App* m_instance;

  std::vector<std::vector<IAppHook*> > m_apphooks;
  Modules* m_modules;
  LegacyModules* m_legacy;

public:
  App(int argc, char* argv[]);
  ~App();

  static App* instance() { return m_instance; }

  int run();

  void add_hook(AppEvent::Type event, IAppHook* hook);
  void trigger_event(AppEvent::Type event);
  
};

void app_refresh_screen(const Sprite* sprite);

void app_realloc_sprite_list();
bool app_realloc_recent_list();

int app_get_current_image_type();

Frame* app_get_top_window();
Widget* app_get_menubar();
Widget* app_get_statusbar();
Widget* app_get_colorbar();
Widget* app_get_toolbar();
Widget* app_get_tabsbar();

void app_default_statusbar_message();

int app_get_fg_color(Sprite* sprite);
int app_get_bg_color(Sprite* sprite);
int app_get_color_to_clear_layer(Layer* layer);

#endif

