/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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
#include "base/system_console.h"
#include "base/unique_ptr.h"
#include "raster/pixel_format.h"

#include <vector>

class Document;
class Layer;
class LegacyModules;
class LoggerModule;
class MainWindow;
class RecentFiles;

namespace ui {
  class MenuBar;
  class Widget;
  class Window;
}

namespace tools { class ToolBox; }

class App
{
public:
  App(int argc, const char* argv[]);
  ~App();

  static App* instance() { return m_instance; }

  // Returns true if ASEPRITE is running with GUI available.
  bool isGui() const { return m_isGui; }

  // Runs the ASEPRITE application. In GUI mode it's the top-level
  // window, in console/scripting it just runs the specified scripts.
  int run();

  tools::ToolBox* getToolBox() const;
  RecentFiles* getRecentFiles() const;
  MainWindow* getMainWindow() const { return m_mainWindow; }

  // App Signals
  Signal0<void> Exit;
  Signal0<void> PaletteChange;
  Signal0<void> PenSizeBeforeChange;
  Signal0<void> PenSizeAfterChange;
  Signal0<void> CurrentToolChange;

private:
  typedef std::vector<base::string> FileList;
  class Modules;

  static App* m_instance;

  base::SystemConsole m_systemConsole;
  Modules* m_modules;
  LegacyModules* m_legacy;
  bool m_isGui;
  bool m_isShell;
  UniquePtr<MainWindow> m_mainWindow;
  FileList m_files;
};

void app_refresh_screen(const Document* document);

void app_rebuild_documents_tabs();

PixelFormat app_get_current_pixel_format();

void app_default_statusbar_message();

int app_get_color_to_clear_layer(Layer* layer);

#endif
