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

#ifndef MAIN_WINDOW_H_INCLUDED
#define MAIN_WINDOW_H_INCLUDED

#include "ui/window.h"
#include "widgets/tabs.h"

class ColorBar;
class MainMenuBar;
class StatusBar;
class Tabs;
namespace ui { class Splitter; }
namespace widgets
{
  class MiniEditorWindow;
  class Workspace;
}

class MainWindow : public ui::Window
                 , public TabsDelegate
{
public:
  MainWindow();
  ~MainWindow();

  MainMenuBar* getMenuBar() { return m_menuBar; }
  Tabs* getTabsBar() { return m_tabsBar; }
  widgets::Workspace* getWorkspace() { return m_workspace; }
  widgets::MiniEditorWindow* getMiniEditor() { return m_miniEditor; }

  void reloadMenus();

  bool isAdvancedMode() const { return m_advancedMode; }
  void setAdvancedMode(bool advanced);

  // TabsDelegate implementation.
  void clickTab(Tabs* tabs, TabView* tabView, int button);
  void mouseOverTab(Tabs* tabs, TabView* tabView);

protected:
  void onSaveLayout(ui::SaveLayoutEvent& ev) OVERRIDE;

private:
  MainMenuBar* m_menuBar;       // the menu bar widget
  StatusBar* m_statusBar;       // the status bar widget
  ColorBar* m_colorBar;         // the color bar widget
  ui::Splitter* m_colorBarSplitter;
  ui::Widget* m_toolBar;        // the tool bar widget
  Tabs* m_tabsBar;              // The tabs bar widget
  double m_lastSplitterPos;
  bool m_advancedMode;
  widgets::Workspace* m_workspace; // The workspace (document's views)
  widgets::MiniEditorWindow* m_miniEditor;
};

#endif
