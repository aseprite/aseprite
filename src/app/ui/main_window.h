/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_UI_MAIN_WINDOW_H_INCLUDED
#define APP_UI_MAIN_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "ui/window.h"

namespace ui {
  class Splitter;
}

namespace app {

  class ColorBar;
  class ContextBar;
  class INotificationDelegate;
  class MainMenuBar;
  class PreviewEditorWindow;
  class Notifications;
  class StartView;
  class StatusBar;
  class Tabs;
  class Timeline;
  class Workspace;

  class MainWindow : public ui::Window
                   , public TabsDelegate {
  public:
    enum Mode {
      NormalMode,
      ContextBarAndTimelineMode,
      EditorOnlyMode
    };

    MainWindow();
    ~MainWindow();

    MainMenuBar* getMenuBar() { return m_menuBar; }
    ContextBar* getContextBar() { return m_contextBar; }
    Tabs* getTabsBar() { return m_tabsBar; }
    Timeline* getTimeline() { return m_timeline; }
    Workspace* getWorkspace() { return m_workspace; }
    PreviewEditorWindow* getPreviewEditor() { return m_previewEditor; }

    void start();
    void reloadMenus();
    void showNotification(INotificationDelegate* del);

    Mode getMode() const { return m_mode; }
    void setMode(Mode mode);

    bool getTimelineVisibility() const;
    void setTimelineVisibility(bool visible);
    void popTimeline();

    // TabsDelegate implementation.
    void clickTab(Tabs* tabs, TabView* tabView, ui::MouseButtons buttons);
    void mouseOverTab(Tabs* tabs, TabView* tabView);

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSaveLayout(ui::SaveLayoutEvent& ev) override;
    void onActiveViewChange();

  private:
    MainMenuBar* m_menuBar;
    ContextBar* m_contextBar;
    StatusBar* m_statusBar;
    ColorBar* m_colorBar;
    ui::Splitter* m_colorBarSplitter;
    ui::Splitter* m_timelineSplitter;
    ui::Widget* m_toolBar;
    Tabs* m_tabsBar;
    double m_lastSplitterPos;
    double m_lastTimelineSplitterPos;
    Mode m_mode;
    Timeline* m_timeline;
    Workspace* m_workspace;
    PreviewEditorWindow* m_previewEditor;
    StartView* m_startView;
    Notifications* m_notifications;
  };

}

#endif
