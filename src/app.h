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

#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED

#include "base/signal.h"
#include "base/string.h"
#include "gui/base.h"

#include <vector>

class AppTabsDelegate;
class CheckArgs;
class ColorBar;
class ConfigModule;
class Document;
class Frame;
class Layer;
class LegacyModules;
class LoggerModule;
class MenuBar;
class Params;
class RecentFiles;
class StatusBar;
class Tabs;

namespace tools { class ToolBox; }

class App
{
public:
  App(int argc, char* argv[]);
  ~App();

  static App* instance() { return m_instance; }

  // Functions to get the arguments specified in the command line.
  int getArgc() const { return m_args.size(); }
  const base::string& getArgv(int i) { return m_args[i]; }
  const std::vector<base::string>& getArgs() const { return m_args; }

  // Returns true if ASE is running with GUI available.
  bool isGui() const { return m_isGui; }

  // Runs the ASE application. In GUI mode it's the top-level window,
  // in console/scripting it just runs the specified scripts.
  int run();

  LoggerModule* getLogger() const;
  tools::ToolBox* getToolBox() const;
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
  std::vector<base::string> m_args;
  AppTabsDelegate* m_tabsDelegate;
};

void app_refresh_screen(const Document* document);

void app_rebuild_documents_tabs();
void app_update_document_tab(const Document* document);
bool app_realloc_recent_list();

int app_get_current_image_type();

Frame* app_get_top_window();
MenuBar* app_get_menubar();
StatusBar* app_get_statusbar();
ColorBar* app_get_colorbar();
Widget* app_get_toolbar();
Tabs* app_get_tabsbar();

void app_default_statusbar_message();

int app_get_color_to_clear_layer(Layer* layer);

#endif

