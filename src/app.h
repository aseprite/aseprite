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

#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

#include "base/signal.h"
#include "gui/jbase.h"

#include <vector>

class CheckArgs;
class ColorBar;
class ConfigModule;
class Frame;
class Layer;
class LegacyModules;
class LoggerModule;
class Params;
class RecentFiles;
class Sprite;
class StatusBar;
class Tabs;
class ToolBox;

class App
{
public:
  App();
  ~App();

  static App* instance() { return m_instance; }

  // Returns true if ASE is running with GUI available.
  bool isGui() const { return m_isGui; }

  // Runs the ASE application. In GUI mode it's the top-level window,
  // in console/scripting it just runs the specified scripts.
  int run();

  LoggerModule* getLogger() const;
  ToolBox* getToolBox() const;
  RecentFiles* getRecentFiles() const;

  // App Signals
  Signal0<void> Exit;
  Signal0<void> PaletteChange;
  Signal0<void> PenSizeBeforeChange;
  Signal0<void> PenSizeAfterChange;
  Signal0<void> CurrentToolChange;

private:
  class Modules;

  static App* m_instance;

  ConfigModule* m_configModule;
  CheckArgs* m_checkArgs;
  LoggerModule* m_loggerModule;
  Modules* m_modules;
  LegacyModules* m_legacy;
  bool m_isGui;
};

void app_refresh_screen(const Sprite* sprite);

void app_realloc_sprite_list();
bool app_realloc_recent_list();

int app_get_current_image_type();

Frame* app_get_top_window();
Widget* app_get_menubar();
StatusBar* app_get_statusbar();
ColorBar* app_get_colorbar();
Widget* app_get_toolbar();
Tabs* app_get_tabsbar();

void app_default_statusbar_message();

int app_get_color_to_clear_layer(Layer* layer);

#endif

