// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
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
#include "app/ui/dock.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/home_view.h"
#include "app/ui/layout_selector.h"
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
#include "os/event.h"
#include "os/event_queue.h"
#include "os/system.h"
#include "ui/app_state.h"
#include "ui/drag_event.h"
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

static const char* kLegacyLayoutMainWindowSection = "layout:main_window";
static const char* kLegacyLayoutTimelineSplitter = "timeline_splitter";
static const char* kLegacyLayoutColorBarSplitter = "color_bar_splitter";

class ScreenScalePanic : public INotificationDelegate {
public:
  std::string notificationText() override { return "Reset Scale!"; }

  void notificationClick() override
  {
    auto& pref = Preferences::instance();

    const int newScreenScale = 2;
    const int newUIScale = 1;

    if (pref.general.screenScale() != newScreenScale)
      pref.general.screenScale(newScreenScale);

    if (pref.general.uiScale() != newUIScale)
      pref.general.uiScale(newUIScale);

    pref.save();

    ui::set_theme(ui::get_theme(), newUIScale);

    Manager::getDefault()->updateAllDisplays(newScreenScale, pref.general.gpuAcceleration());
  }
};

MainWindow::MainWindow()
  : ui::Window(ui::Window::DesktopWindow)
  , m_tooltipManager(new TooltipManager)
  , m_dock(new Dock)
  , m_customizableDock(new Dock)
  , m_mode(NormalMode)
  , m_homeView(nullptr)
  , m_scalePanic(nullptr)
  , m_browserView(nullptr)
#ifdef ENABLE_SCRIPTING
  , m_devConsoleView(nullptr)
#endif
{
  enableFlags(ALLOW_DROP);
  setNeedsTabletPressure(true);
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
  m_menuBar = std::make_unique<MainMenuBar>();
  m_layoutSelector = std::make_unique<LayoutSelector>(m_tooltipManager);

  // Register commands to load menus+shortcuts for these commands
  Editor::registerCommands();

  // Load all menus+keys for the first time
  AppMenus::instance()->reload();

  // Setup the main menubar
  m_menuBar->setMenu(AppMenus::instance()->getRootMenu());

  m_notifications = std::make_unique<Notifications>();
  m_statusBar = std::make_unique<StatusBar>(m_tooltipManager);
  m_toolBar = std::make_unique<ToolBar>();
  m_tabsBar = std::make_unique<WorkspaceTabs>(this);
  m_workspace = std::make_unique<Workspace>();
  m_previewEditor = std::make_unique<PreviewEditorWindow>();
  m_colorBar = std::make_unique<ColorBar>(m_tooltipManager);
  m_contextBar = std::make_unique<ContextBar>(m_tooltipManager, m_colorBar.get());

  // The timeline (AniControls) tooltips will use the keyboard
  // shortcuts loaded above.
  m_timeline = std::make_unique<Timeline>(m_tooltipManager);

  m_workspace->setTabsBar(m_tabsBar.get());
  m_workspace->BeforeViewChanged.connect(&MainWindow::onBeforeViewChange, this);
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

  // IDs to create UI layouts from a Dock (see app::Layout
  // constructor).
  m_colorBar->setId("colorbar");
  m_contextBar->setId("contextbar");
  m_statusBar->setId("statusbar");
  m_timeline->setId("timeline");
  m_toolBar->setId("toolbar");
  m_workspace->setId("workspace");

  // Add the widgets in the boxes
  addChild(m_tooltipManager);
  addChild(m_dock);

  m_customizableDockPlaceholder = std::make_unique<Widget>();
  m_customizableDockPlaceholder->addChild(m_customizableDock);
  m_customizableDockPlaceholder->InitTheme.connect([this] {
    auto theme = static_cast<skin::SkinTheme*>(this->theme());
    m_customizableDock->setBgColor(theme->colors.workspace());
  });
  m_customizableDockPlaceholder->initTheme();

  m_dock->top()->right()->dock(ui::RIGHT, m_notifications.get());
  m_dock->top()->right()->dock(ui::CENTER, m_layoutSelector.get());
  m_dock->top()->dock(ui::BOTTOM, m_tabsBar.get());
  m_dock->top()->dock(ui::CENTER, m_menuBar.get());
  m_dock->dock(ui::CENTER, m_customizableDockPlaceholder.get());

  // After the user resizes the dock we save the updated layout
  m_saveDockLayoutConn = m_customizableDock->UserResizedDock.connect(
    [this] { saveActiveLayout(); });

  setDefaultLayout();
  if (LayoutPtr layout = m_layoutSelector->activeLayout())
    loadUserLayout(layout.get());

  // Reconfigure workspace when the timeline position is changed.
  auto& pref = Preferences::instance();
  pref.general.timelinePosition.AfterChange.connect([this] { configureWorkspaceLayout(); });
  pref.general.showMenuBar.AfterChange.connect([this] { configureWorkspaceLayout(); });

  // Prepare the window
  remapWindow();

  AppMenus::instance()->rebuildRecentList();

  // When the language is change, we reload the menu bar strings and
  // relayout the whole main window.
  Strings::instance()->LanguageChange.connect([this] { onLanguageChange(); });
}

MainWindow::~MainWindow()
{
  m_timelineResizeConn.disconnect();
  m_colorBarResizeConn.disconnect();
  m_saveDockLayoutConn.disconnect();

  m_dock->resetDocks();
  m_customizableDock->resetDocks();

  m_layoutSelector.reset();
  m_scalePanic.reset();

#ifdef ENABLE_SCRIPTING
  if (m_devConsoleView) {
    if (m_devConsoleView->parent() && m_workspace)
      m_workspace->removeView(m_devConsoleView.get());
    m_devConsoleView.reset();
  }
#endif

  if (m_browserView) {
    if (m_browserView->parent() && m_workspace)
      m_workspace->removeView(m_browserView.get());
    m_browserView.reset();
  }

  if (m_homeView) {
    if (m_homeView->parent() && m_workspace)
      m_workspace->removeView(m_homeView.get());
    m_homeView.reset();
  }
  m_contextBar.reset();
  m_previewEditor.reset();

  // Destroy the workspace first so ~Editor can dettach slots from
  // ColorBar. TODO this is a terrible hack for slot/signal stuff,
  // connections should be handle in a better/safer way.
  m_workspace.reset();

  // Remove the root-menu from the menu-bar (because the rootmenu
  // module should destroy it).
  if (m_menuBar)
    m_menuBar->setMenu(nullptr);
}

void MainWindow::onLanguageChange()
{
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
    m_homeView = std::make_unique<HomeView>();
  return m_homeView.get();
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
  layout();
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
    m_workspace->addView(m_homeView.get(), 0);
    if (selectedTab)
      m_tabsBar->selectTab(selectedTab);
    else
      m_tabsBar->selectTab(m_homeView.get());
  }
}

void MainWindow::showHome()
{
  if (!getHomeView()->parent()) {
    m_workspace->addView(m_homeView.get(), 0);
  }
  m_tabsBar->selectTab(m_homeView.get());
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
  return (m_homeView && m_workspace->activeView() == m_homeView.get());
}

void MainWindow::showBrowser(const std::string& filename, const std::string& section)
{
  if (!m_browserView)
    m_browserView = std::make_unique<BrowserView>();

  m_browserView->loadFile(filename, section);

  if (!m_browserView->parent()) {
    m_workspace->addView(m_browserView.get());
    m_tabsBar->selectTab(m_browserView.get());
  }
}

void MainWindow::showDevConsole()
{
#ifdef ENABLE_SCRIPTING
  if (!m_devConsoleView)
    m_devConsoleView = std::make_unique<DevConsoleView>();

  if (!m_devConsoleView->parent()) {
    m_workspace->addView(m_devConsoleView.get());
    m_tabsBar->selectTab(m_devConsoleView.get());
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

void MainWindow::setDefaultLayout()
{
  m_timelineResizeConn.disconnect();
  m_colorBarResizeConn.disconnect();

  auto colorBarWidth = get_config_double(kLegacyLayoutMainWindowSection,
                                         kLegacyLayoutColorBarSplitter,
                                         m_colorBar->sizeHint().w);

  m_customizableDock->resetDocks();
  m_customizableDock->dock(ui::LEFT, m_colorBar.get(), gfx::Size(colorBarWidth, 0));
  m_customizableDock->dock(ui::BOTTOM, m_statusBar.get());
  m_customizableDock->center()->dock(ui::TOP, m_contextBar.get());
  m_customizableDock->center()->dock(ui::RIGHT, m_toolBar.get());
  m_customizableDock->center()->center()->dock(ui::BOTTOM,
                                               m_timeline.get(),
                                               gfx::Size(64 * guiscale(), 64 * guiscale()));
  m_customizableDock->center()->center()->dock(ui::CENTER, m_workspace.get());
  configureWorkspaceLayout();
}

void MainWindow::setMirroredDefaultLayout()
{
  m_timelineResizeConn.disconnect();
  m_colorBarResizeConn.disconnect();

  auto colorBarWidth = get_config_double(kLegacyLayoutMainWindowSection,
                                         kLegacyLayoutColorBarSplitter,
                                         m_colorBar->sizeHint().w);

  m_customizableDock->resetDocks();
  m_customizableDock->dock(ui::RIGHT, m_colorBar.get(), gfx::Size(colorBarWidth, 0));
  m_customizableDock->dock(ui::BOTTOM, m_statusBar.get());
  m_customizableDock->center()->dock(ui::TOP, m_contextBar.get());
  m_customizableDock->center()->dock(ui::LEFT, m_toolBar.get());
  m_customizableDock->center()->center()->dock(ui::BOTTOM,
                                               m_timeline.get(),
                                               gfx::Size(64 * guiscale(), 64 * guiscale()));
  m_customizableDock->center()->center()->dock(ui::CENTER, m_workspace.get());
  configureWorkspaceLayout();
}

void MainWindow::loadUserLayout(const Layout* layout)
{
  m_timelineResizeConn.disconnect();
  m_colorBarResizeConn.disconnect();

  m_customizableDock->resetDocks();

  if (!layout->loadLayout(m_customizableDock))
    setDefaultLayout();

  this->layout();
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
  ui::Window::onInitTheme(ev);
  noBorderNoChildSpacing();
  if (m_previewEditor)
    m_previewEditor->initTheme();
}

void MainWindow::onResize(ui::ResizeEvent& ev)
{
  ui::Window::onResize(ev);

  os::Window* nativeWindow = (display() ? display()->nativeWindow() : nullptr);
  if (nativeWindow && nativeWindow->screen()) {
    const int scale = nativeWindow->scale() * ui::guiscale();

    // We can check for the available workarea to know that the user
    // can resize the window to its full size and there will be enough
    // room to display some common dialogs like (for example) the
    // Preferences dialog.
    if ((scale > 2) && (!m_scalePanic)) {
      const gfx::Size wa = nativeWindow->screen()->workarea().size();
      if ((wa.w / scale < 256 || wa.h / scale < 256)) {
        m_scalePanic = std::make_unique<ScreenScalePanic>();
        showNotification(m_scalePanic.get());
      }
    }
  }
}

void MainWindow::onBeforeViewChange()
{
  UIContext::instance()->notifyBeforeActiveSiteChanged();
}

// When the active view is changed from methods like
// Workspace::splitView(), this function is called, and we have to
// inform to the UIContext that the current view has changed.
void MainWindow::onActiveViewChange()
{
  // If we are closing the app, we just ignore all view changes (as
  // docs will be destroyed and views closed).
  if (get_app_state() != AppState::kNormal)
    return;

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

void MainWindow::onDrop(ui::DragEvent& e)
{
  if (e.hasImage() && !e.hasPaths()) {
    auto* cmd = Commands::instance()->byId(CommandId::NewFile());
    Params params;
    params.set("fromDraggedData", "true");
    UIContext::instance()->setDraggedData(std::make_unique<DraggedData>(e.getImage()));
    UIContext::instance()->executeCommand(cmd, params);
    e.handled(true);
    invalidate();
    flushRedraw();
    os::Event ev;
    os::System::instance()->eventQueue()->queueEvent(ev);
  }
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

  m_workspace->addViewToPanel(static_cast<WorkspaceTabs*>(tabs)->panel(), clone, true, pos);

  clone->onClonedFrom(view);
}

void MainWindow::onContextMenuTab(Tabs* tabs, TabView* tabView)
{
  WorkspaceView* view = dynamic_cast<WorkspaceView*>(tabView);
  ASSERT(view);
  if (view)
    view->onTabPopup(m_workspace.get());
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
    WorkspacePanel* doubleClickedPanel = static_cast<WorkspaceTabs*>(tabs)->panel();

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

DropViewPreviewResult MainWindow::onFloatingTab(Tabs* tabs,
                                                TabView* tabView,
                                                const gfx::Point& screenPos)
{
  return m_workspace->setDropViewPreview(screenPos,
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
    m_workspace->dropViewAt(screenPos, dynamic_cast<WorkspaceView*>(tabView), clone);

  if (result == DropViewAtResult::MOVED_TO_OTHER_PANEL)
    return DropTabResult::REMOVE;
  else if (result == DropViewAtResult::CLONED_VIEW)
    return DropTabResult::DONT_REMOVE;
  else
    return DropTabResult::NOT_HANDLED;
}

void MainWindow::configureWorkspaceLayout()
{
  // First layout to get the bounds of some widgets
  layout();

  const auto& pref = Preferences::instance();
  bool normal = (m_mode == NormalMode);
  bool isDoc = (getDocView() != nullptr);

  if (os::System::instance()->menus() == nullptr || pref.general.showMenuBar()) {
    if (!m_menuBar->parent())
      m_dock->top()->dock(CENTER, m_menuBar.get());
  }
  else {
    if (m_menuBar->parent())
      m_dock->undock(m_menuBar.get());
  }

  m_menuBar->setVisible(normal);
  m_notifications->setVisible(normal && m_notifications->hasNotifications());
  m_tabsBar->setVisible(normal);

  // TODO set visibility of color bar widgets
  m_colorBar->setVisible(normal && isDoc);
  m_colorBarResizeConn = m_customizableDock->Resize.connect(
    [this] { saveColorBarConfiguration(); });

  m_toolBar->setVisible(normal && isDoc);
  m_statusBar->setVisible(normal);
  m_contextBar->setVisible(isDoc && (m_mode == NormalMode || m_mode == ContextBarAndTimelineMode));

  // Configure timeline
  {
    const gfx::Rect workspaceBounds = m_customizableDock->center()->center()->bounds();
    // Get legacy timeline position and splitter position
    auto timelinePosition = pref.general.timelinePosition();
    auto timelineSplitterPos =
      get_config_double(kLegacyLayoutMainWindowSection, kLegacyLayoutTimelineSplitter, 75.0) /
      100.0;
    int side = ui::BOTTOM;

    m_customizableDock->undock(m_timeline.get());

    int w, h;
    w = h = 64;

    switch (timelinePosition) {
      case gen::TimelinePosition::LEFT:
        side = ui::LEFT;
        w = (workspaceBounds.w * (1.0 - timelineSplitterPos)) / guiscale();
        break;
      case gen::TimelinePosition::RIGHT:
        side = ui::RIGHT;
        w = (workspaceBounds.w * (1.0 - timelineSplitterPos)) / guiscale();
        break;
      case gen::TimelinePosition::BOTTOM:
        side = ui::BOTTOM;
        h = (workspaceBounds.h * (1.0 - timelineSplitterPos)) / guiscale();
        break;
    }

    // Listen to resizing changes in the dock that contains the
    // timeline (so we save the new splitter position)
    m_timelineResizeConn = m_customizableDock->center()->center()->Resize.connect(
      [this] { saveTimelineConfiguration(); });

    m_customizableDock->center()->center()->dock(side,
                                                 m_timeline.get(),
                                                 gfx::Size(w * guiscale(), h * guiscale()));

    m_timeline->setVisible(isDoc && (m_mode == NormalMode || m_mode == ContextBarAndTimelineMode) &&
                           pref.general.visibleTimeline());
  }

  if (m_contextBar->isVisible()) {
    m_contextBar->updateForActiveTool();
  }

  layout();
}

void MainWindow::saveTimelineConfiguration()
{
  const auto& pref = Preferences::instance();
  const gfx::Rect timelineBounds = m_timeline->bounds();
  const gfx::Rect workspaceBounds = m_customizableDock->center()->center()->bounds();
  auto timelinePosition = pref.general.timelinePosition();
  double timelineSplitterPos = 0.75;

  switch (timelinePosition) {
    case gen::TimelinePosition::LEFT:
    case gen::TimelinePosition::RIGHT:
      timelineSplitterPos = 1.0 - double(timelineBounds.w) / workspaceBounds.w;
      break;
    case gen::TimelinePosition::BOTTOM:
      timelineSplitterPos = 1.0 - double(timelineBounds.h) / workspaceBounds.h;
      break;
  }

  set_config_double(kLegacyLayoutMainWindowSection,
                    kLegacyLayoutTimelineSplitter,
                    std::clamp(timelineSplitterPos * 100.0, 1.0, 99.0));
}

void MainWindow::saveColorBarConfiguration()
{
  set_config_double(kLegacyLayoutMainWindowSection,
                    kLegacyLayoutColorBarSplitter,
                    m_colorBar->bounds().w);
}

void MainWindow::saveActiveLayout()
{
  ASSERT(m_layoutSelector);

  auto id = m_layoutSelector->activeLayoutId();
  auto layout = Layout::MakeFromDock(id, id, m_customizableDock);
  m_layoutSelector->updateActiveLayout(layout);
}

} // namespace app
