// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/main_window.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/crash/data_recovery.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/notification_delegate.h"
#include "app/pref/preferences.h"
#include "app/ui/browser_view.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/home_view.h"
#include "app/ui/main_menu_bar.h"
#include "app/ui/notifications.h"
#include "app/ui/preview_editor.h"
#include "app/ui/skin/skin_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui/workspace.h"
#include "app/ui/workspace_tabs.h"
#include "app/ui_context.h"
#include "base/fs.h"
#include "os/system.h"
#include "ui/message.h"
#include "ui/splitter.h"
#include "ui/system.h"
#include "ui/tooltips.h"
#include "ui/view.h"

#ifdef ENABLE_SCRIPTING
  #include "app/ui/devconsole_view.h"
#endif

namespace app {

using namespace ui;

class ScreenScalePanic : public INotificationDelegate {
public:
  std::string notificationText() override {
    return "Reset Scale!";
  }

  void notificationClick() override {
    auto& pref = Preferences::instance();

    const int newScreenScale = 2;
    const int newUIScale = 1;

    if (pref.general.screenScale() != newScreenScale)
      pref.general.screenScale(newScreenScale);

    if (pref.general.uiScale() != newUIScale)
      pref.general.uiScale(newUIScale);

    pref.save();

    ui::set_theme(ui::get_theme(), newUIScale);

    Manager::getDefault()
      ->updateAllDisplaysWithNewScale(newScreenScale);
  }
};

MainWindow::MainWindow()
  : m_mode(NormalMode)
  , m_homeView(nullptr)
  , m_scalePanic(nullptr)
  , m_browserView(nullptr)
#ifdef ENABLE_SCRIPTING
  , m_devConsoleView(nullptr)
#endif
{
}

// This 'initialize' function is a way to split the creation of the
// MainWindow. First a minimal instance of MainWindow is created, then
// all UI components that can trigger the Console to report any
// unexpected errors/warnings in the initialization. Prior to this,
// Aseprite could fail in the same constructor, and the Console didn't
// have access to the App::instance()->mainWindow() pointer.
//
// Refer to https://github.com/aseprite/aseprite/issues/3914
void MainWindow::initialize()
{
  m_tooltipManager = new TooltipManager();
  m_menuBar = new MainMenuBar();

  // Register commands to load menus+shortcuts for these commands
  Editor::registerCommands();

  // Load all menus+keys for the first time
  AppMenus::instance()->reload();

  // Setup the main menubar
  m_menuBar->setMenu(AppMenus::instance()->getRootMenu());

  m_notifications = new Notifications();
  m_statusBar = new StatusBar(m_tooltipManager);
  m_toolBar = new ToolBar();
  m_tabsBar = new WorkspaceTabs(this);
  m_workspace = new Workspace();
  m_previewEditor = new PreviewEditorWindow();
  m_colorBar = new ColorBar(colorBarPlaceholder()->align(),
                            m_tooltipManager);
  m_contextBar = new ContextBar(m_tooltipManager, m_colorBar);

  // The timeline (AniControls) tooltips will use the keyboard
  // shortcuts loaded above.
  m_timeline = new Timeline(m_tooltipManager);

  m_workspace->setTabsBar(m_tabsBar);
  m_workspace->ActiveViewChanged.connect(&MainWindow::onActiveViewChange, this);

  // configure all widgets to expansives
  m_menuBar->setExpansive(true);
  m_contextBar->setExpansive(true);
  m_contextBar->setVisible(false);
  m_statusBar->setExpansive(true);
  m_colorBar->setExpansive(true);
  m_toolBar->setExpansive(true);
  m_tabsBar->setExpansive(true);
  m_timeline->setExpansive(true);
  m_workspace->setExpansive(true);
  m_notifications->setVisible(false);

  // Add the widgets in the boxes
  addChild(m_tooltipManager);
  menuBarPlaceholder()->addChild(m_menuBar);
  menuBarPlaceholder()->addChild(m_notifications);
  contextBarPlaceholder()->addChild(m_contextBar);
  colorBarPlaceholder()->addChild(m_colorBar);
  toolBarPlaceholder()->addChild(m_toolBar);
  statusBarPlaceholder()->addChild(m_statusBar);
  tabsPlaceholder()->addChild(m_tabsBar);
  workspacePlaceholder()->addChild(m_workspace);
  timelinePlaceholder()->addChild(m_timeline);

  // Default splitter positions
  colorBarSplitter()->setPosition(m_colorBar->sizeHint().w);
  timelineSplitter()->setPosition(75);

  // Reconfigure workspace when the timeline position is changed.
  auto& pref = Preferences::instance();
  pref.general.timelinePosition.AfterChange.connect([this]{ configureWorkspaceLayout(); });
  pref.general.showMenuBar.AfterChange.connect([this]{ configureWorkspaceLayout(); });

  // Prepare the window
  remapWindow();

  AppMenus::instance()->rebuildRecentList();

  // When the language is change, we reload the menu bar strings and
  // relayout the whole main window.
  Strings::instance()->LanguageChange.connect([this] { onLanguageChange(); });
}

MainWindow::~MainWindow()
{
  delete m_scalePanic;

#ifdef ENABLE_SCRIPTING
  if (m_devConsoleView) {
    if (m_devConsoleView->parent() && m_workspace)
      m_workspace->removeView(m_devConsoleView);
    delete m_devConsoleView;
  }
#endif

  if (m_browserView) {
    if (m_browserView->parent() && m_workspace)
      m_workspace->removeView(m_browserView);
    delete m_browserView;
  }

  if (m_homeView) {
    if (m_homeView->parent() && m_workspace)
      m_workspace->removeView(m_homeView);
    delete m_homeView;
  }
  if (m_contextBar)
    delete m_contextBar;
  if (m_previewEditor)
    delete m_previewEditor;

  // Destroy the workspace first so ~Editor can dettach slots from
  // ColorBar. TODO this is a terrible hack for slot/signal stuff,
  // connections should be handle in a better/safer way.
  if (m_workspace)
    delete m_workspace;

  // Remove the root-menu from the menu-bar (because the rootmenu
  // module should destroy it).
  if (m_menuBar)
    m_menuBar->setMenu(NULL);
}

void MainWindow::onLanguageChange()
{
  auto commands = Commands::instance();
  std::vector<std::string> commandIDs;
  commands->getAllIds(commandIDs);

  for (const auto& commandID : commandIDs) {
    Command* command = commands->byId(commandID.c_str());
    command->generateFriendlyName();
  }

  m_menuBar->reload();
  layout();
  invalidate();
}

DocView* MainWindow::getDocView()
{
  return dynamic_cast<DocView*>(m_workspace->activeView());
}

HomeView* MainWindow::getHomeView()
{
  if (!m_homeView)
    m_homeView = new HomeView;
  return m_homeView;
}

#ifdef ENABLE_UPDATER
CheckUpdateDelegate* MainWindow::getCheckUpdateDelegate()
{
  return getHomeView();
}
#endif

#if ENABLE_SENTRY
void MainWindow::updateConsentCheckbox()
{
  getHomeView()->updateConsentCheckbox();
}
#endif

void MainWindow::showNotification(INotificationDelegate* del)
{
  m_notifications->addLink(del);
  m_notifications->setVisible(true);
  m_notifications->parent()->layout();
}

void MainWindow::showHomeOnOpen()
{
  // Don't open Home tab
  if (!Preferences::instance().general.showHome()) {
    configureWorkspaceLayout();
    return;
  }

  if (!getHomeView()->parent()) {
    TabView* selectedTab = m_tabsBar->getSelectedTab();

    // Show "Home" tab in the first position, and select it only if
    // there is no other view selected.
    m_workspace->addView(m_homeView, 0);
    if (selectedTab)
      m_tabsBar->selectTab(selectedTab);
    else
      m_tabsBar->selectTab(m_homeView);
  }
}

void MainWindow::showHome()
{
  if (!getHomeView()->parent()) {
    m_workspace->addView(m_homeView, 0);
  }
  m_tabsBar->selectTab(m_homeView);
}

void MainWindow::showDefaultStatusBar()
{
  if (DocView* docView = getDocView())
    m_statusBar->showDefaultText(docView->document());
  else if (isHomeSelected())
    m_statusBar->showAbout();
  else
    m_statusBar->clearText();
}

bool MainWindow::isHomeSelected() const
{
  return (m_homeView && m_workspace->activeView() == m_homeView);
}

void MainWindow::showBrowser(const std::string& filename,
                             const std::string& section)
{
  if (!m_browserView)
    m_browserView = new BrowserView;

  m_browserView->loadFile(filename, section);

  if (!m_browserView->parent()) {
    m_workspace->addView(m_browserView);
    m_tabsBar->selectTab(m_browserView);
  }
}

void MainWindow::showDevConsole()
{
#ifdef ENABLE_SCRIPTING
  if (!m_devConsoleView)
    m_devConsoleView = new DevConsoleView;

  if (!m_devConsoleView->parent()) {
    m_workspace->addView(m_devConsoleView);
    m_tabsBar->selectTab(m_devConsoleView);
  }
#endif
}

void MainWindow::setMode(Mode mode)
{
  // Check if we already are in the given mode.
  if (m_mode == mode)
    return;

  m_mode = mode;
  configureWorkspaceLayout();
}

bool MainWindow::getTimelineVisibility() const
{
  return Preferences::instance().general.visibleTimeline();
}

void MainWindow::setTimelineVisibility(bool visible)
{
  Preferences::instance().general.visibleTimeline(visible);

  configureWorkspaceLayout();
}

void MainWindow::popTimeline()
{
  Preferences& preferences = Preferences::instance();

  if (!preferences.general.autoshowTimeline())
    return;

  if (!getTimelineVisibility())
    setTimelineVisibility(true);
}

void MainWindow::dataRecoverySessionsAreReady()
{
  getHomeView()->dataRecoverySessionsAreReady();
}

bool MainWindow::onProcessMessage(ui::Message* msg)
{
  if (msg->type() == kOpenMessage)
    showHomeOnOpen();

  return Window::onProcessMessage(msg);
}

void MainWindow::onInitTheme(ui::InitThemeEvent& ev)
{
  app::gen::MainWindow::onInitTheme(ev);
  if (m_previewEditor)
    m_previewEditor->initTheme();
}

void MainWindow::onSaveLayout(SaveLayoutEvent& ev)
{
  // Invert the timeline splitter position before we save the setting.
  if (Preferences::instance().general.timelinePosition() ==
      gen::TimelinePosition::LEFT) {
    timelineSplitter()->setPosition(100 - timelineSplitter()->getPosition());
  }

  Window::onSaveLayout(ev);
}

void MainWindow::onResize(ui::ResizeEvent& ev)
{
  app::gen::MainWindow::onResize(ev);

  os::Window* nativeWindow = (display() ? display()->nativeWindow(): nullptr);
  if (nativeWindow && nativeWindow->screen()) {
    const int scale = nativeWindow->scale()*ui::guiscale();

    // We can check for the available workarea to know that the user
    // can resize the window to its full size and there will be enough
    // room to display some common dialogs like (for example) the
    // Preferences dialog.
    if ((scale > 2) &&
        (!m_scalePanic)) {
      const gfx::Size wa = nativeWindow->screen()->workarea().size();
      if ((wa.w / scale < 256 ||
           wa.h / scale < 256)) {
        showNotification(m_scalePanic = new ScreenScalePanic);
      }
    }
  }
}

// When the active view is changed from methods like
// Workspace::splitView(), this function is called, and we have to
// inform to the UIContext that the current view has changed.
void MainWindow::onActiveViewChange()
{
  // First we have to configure the MainWindow layout (e.g. show
  // Timeline if needed) as UIContext::setActiveView() will configure
  // several widgets (calling updateUsingEditor() functions) using the
  // active document, and we need to know the available space on
  // screen for each widget (e.g. the Timeline will configure its
  // scrollable area/position depending on the number of
  // layers/frames, but it needs to know its position on screen
  // first).
  configureWorkspaceLayout();

  if (DocView* docView = getDocView())
    UIContext::instance()->setActiveView(docView);
  else
    UIContext::instance()->setActiveView(nullptr);
}

bool MainWindow::isTabModified(Tabs* tabs, TabView* tabView)
{
  if (DocView* docView = dynamic_cast<DocView*>(tabView)) {
    Doc* document = docView->document();
    return document->isModified();
  }
  else {
    return false;
  }
}

bool MainWindow::canCloneTab(Tabs* tabs, TabView* tabView)
{
  ASSERT(tabView)

  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  return view->canCloneWorkspaceView();
}

void MainWindow::onSelectTab(Tabs* tabs, TabView* tabView)
{
  if (!tabView)
    return;

  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  if (m_workspace->activeView() != view)
    m_workspace->setActiveView(view);
}

void MainWindow::onCloseTab(Tabs* tabs, TabView* tabView)
{
  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  ASSERT(view);
  if (view)
    m_workspace->closeView(view, false);
}

void MainWindow::onCloneTab(Tabs* tabs, TabView* tabView, int pos)
{
  EditorView::SetScrollUpdateMethod(EditorView::KeepOrigin);

  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  WorkspaceView* clone = view->cloneWorkspaceView();
  ASSERT(clone);

  m_workspace->addViewToPanel(
    static_cast<WorkspaceTabs*>(tabs)->panel(), clone, true, pos);

  clone->onClonedFrom(view);
}

void MainWindow::onContextMenuTab(Tabs* tabs, TabView* tabView)
{
  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  ASSERT(view);
  if (view)
    view->onTabPopup(m_workspace);
}

void MainWindow::onTabsContainerDoubleClicked(Tabs* tabs)
{
  WorkspacePanel* mainPanel = m_workspace->mainPanel();
  WorkspaceView* oldActiveView = mainPanel->activeView();
  Doc* oldDoc = UIContext::instance()->activeDocument();

  Command* command = Commands::instance()->byId(CommandId::NewFile());
  UIContext::instance()->executeCommandFromMenuOrShortcut(command);

  Doc* newDoc = UIContext::instance()->activeDocument();
  if (newDoc != oldDoc) {
    WorkspacePanel* doubleClickedPanel =
      static_cast<WorkspaceTabs*>(tabs)->panel();

    // TODO move this code to workspace?
    // Put the new sprite in the double-clicked tabs control
    if (doubleClickedPanel != mainPanel) {
      WorkspaceView* newView = m_workspace->activeView();
      m_workspace->removeView(newView);
      m_workspace->addViewToPanel(doubleClickedPanel, newView, false, -1);

      // Re-activate the old view in the main panel
      mainPanel->setActiveView(oldActiveView);
      doubleClickedPanel->setActiveView(newView);
    }
  }
}

void MainWindow::onMouseOverTab(Tabs* tabs, TabView* tabView)
{
  // Note: tabView can be NULL
  if (DocView* docView = dynamic_cast<DocView*>(tabView))
    m_statusBar->showDefaultText(docView->document());
  else if (tabView)
    m_statusBar->setStatusText(0, tabView->getTabText());
  else
    m_statusBar->showDefaultText();
}

void MainWindow::onMouseLeaveTab()
{
  m_statusBar->showDefaultText();
}

DropViewPreviewResult MainWindow::onFloatingTab(
  Tabs* tabs,
  TabView* tabView,
  const gfx::Point& screenPos)
{
  return m_workspace->setDropViewPreview(
    screenPos,
    dynamic_cast<WorkspaceView*>(tabView),
    static_cast<WorkspaceTabs*>(tabs));
}

void MainWindow::onDockingTab(Tabs* tabs, TabView* tabView)
{
  m_workspace->removeDropViewPreview();
}

DropTabResult MainWindow::onDropTab(Tabs* tabs,
                                    TabView* tabView,
                                    const gfx::Point& screenPos,
                                    const bool clone)
{
  m_workspace->removeDropViewPreview();

  DropViewAtResult result =
    m_workspace->dropViewAt(screenPos,
                            dynamic_cast<WorkspaceView*>(tabView),
                            clone);

  if (result == DropViewAtResult::MOVED_TO_OTHER_PANEL)
    return DropTabResult::REMOVE;
  else if (result == DropViewAtResult::CLONED_VIEW)
    return DropTabResult::DONT_REMOVE;
  else
    return DropTabResult::NOT_HANDLED;
}

void MainWindow::configureWorkspaceLayout()
{
  const auto& pref = Preferences::instance();
  bool normal = (m_mode == NormalMode);
  bool isDoc = (getDocView() != nullptr);

  if (os::instance()->menus() == nullptr ||
      pref.general.showMenuBar()) {
    m_menuBar->resetMaxSize();
  }
  else {
    m_menuBar->setMaxSize(gfx::Size(0, 0));
  }

  m_menuBar->setVisible(normal);
  m_notifications->setVisible(normal && m_notifications->hasNotifications());
  m_tabsBar->setVisible(normal);
  colorBarPlaceholder()->setVisible(normal && isDoc);
  m_toolBar->setVisible(normal && isDoc);
  m_statusBar->setVisible(normal);
  m_contextBar->setVisible(
    isDoc &&
    (m_mode == NormalMode ||
     m_mode == ContextBarAndTimelineMode));

  // Configure timeline
  {
    auto timelinePosition = pref.general.timelinePosition();
    bool invertWidgets = false;
    int align = VERTICAL;
    switch (timelinePosition) {
      case gen::TimelinePosition::LEFT:
        align = HORIZONTAL;
        invertWidgets = true;
        break;
      case gen::TimelinePosition::RIGHT:
        align = HORIZONTAL;
        break;
      case gen::TimelinePosition::BOTTOM:
        break;
    }

    timelineSplitter()->setAlign(align);
    timelinePlaceholder()->setVisible(
      isDoc &&
      (m_mode == NormalMode ||
       m_mode == ContextBarAndTimelineMode) &&
      pref.general.visibleTimeline());

    bool invertSplitterPos = false;
    if (invertWidgets) {
      if (timelineSplitter()->firstChild() == workspacePlaceholder() &&
          timelineSplitter()->lastChild() == timelinePlaceholder()) {
        timelineSplitter()->removeChild(workspacePlaceholder());
        timelineSplitter()->addChild(workspacePlaceholder());
        invertSplitterPos = true;
      }
    }
    else {
      if (timelineSplitter()->firstChild() == timelinePlaceholder() &&
          timelineSplitter()->lastChild() == workspacePlaceholder()) {
        timelineSplitter()->removeChild(timelinePlaceholder());
        timelineSplitter()->addChild(timelinePlaceholder());
        invertSplitterPos = true;
      }
    }
    if (invertSplitterPos)
      timelineSplitter()->setPosition(100 - timelineSplitter()->getPosition());
  }

  if (m_contextBar->isVisible()) {
    m_contextBar->updateForActiveTool();
  }

  layout();
  invalidate();
}

} // namespace app
