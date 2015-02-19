// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  class HomeView;
  class INotificationDelegate;
  class MainMenuBar;
  class Notifications;
  class PreviewEditorWindow;
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
    void clickTab(Tabs* tabs, TabView* tabView, ui::MouseButtons buttons) override;
    void clickClose(Tabs* tabs, TabView* tabView) override;
    void mouseOverTab(Tabs* tabs, TabView* tabView) override;
    bool isModified(Tabs* tabs, TabView* tabView) override;

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
    HomeView* m_homeView;
    Notifications* m_notifications;
  };

}

#endif
