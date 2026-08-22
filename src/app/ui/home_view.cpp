// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/home_view.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/crash/data_recovery.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/ui/data_recovery_view.h"
#include "app/ui/main_window.h"
#include "app/ui/recent_grid.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "base/launcher.h"
#include "ui/view.h"
#include "ver/info.h"

#ifdef ENABLE_NEWS
  #include "app/ui/news_listbox.h"
#endif

#if ENABLE_SENTRY
  #include "app/sentry_wrapper.h"
#endif

#ifdef ENABLE_DRM
  #include "aseprite_update.h"
  #include "drm/drm.h"
#endif

namespace app {

using namespace ui;
using namespace app::skin;

HomeView::HomeView() : m_showNews(false), m_recents(new RecentGrid), m_news(nullptr)
{
#ifdef ENABLE_NEWS
  const auto& preferences = App::instance()->preferences();
  setShowNews(preferences.home.showNews());
#endif

  newFile()->Click.connect(&HomeView::onNewFile, this);
  openFile()->Click.connect(&HomeView::onOpenFile, this);
#ifdef ENABLE_DATA_RECOVERY
  if (auto* recovery = App::instance()->dataRecovery()) {
    recoverSprites()->setEnabled(false);
    recovery->SessionsListIsReady.connect([this, recovery] {
      if (!recoverSprites()->isEnabled() && recovery->hasRecoverySessions()) {
        recoverSprites()->setStyle(SkinTheme::get(this)->styles.workspaceAttentionLink());
        layout();
      }
      recoverSprites()->setEnabled(true);
    });
    recoverSprites()->Click.connect(&HomeView::onRecoverSprites, this);
  }
  else {
    recoverSprites()->setVisible(false);
  }
#else
  recoverSprites()->setVisible(false);
#endif

  recentsView()->attachToView(m_recents);
  recentsView()->InitTheme.connect([this] {
    const auto* theme = SkinTheme::get(this);
    const int barSize = theme->dimensions.miniScrollbarSize();
    recentsView()->horizontalBar()->setBarWidth(barSize);
    recentsView()->verticalBar()->setBarWidth(barSize);
    recentsView()->horizontalBar()->setStyle(theme->styles.transparentScrollbar());
    recentsView()->verticalBar()->setStyle(theme->styles.transparentScrollbar());
    recentsView()->horizontalBar()->setThumbStyle(theme->styles.transparentScrollbarThumb());
    recentsView()->verticalBar()->setThumbStyle(theme->styles.transparentScrollbarThumb());
  });

  recentsSection()->InitTheme.connect([this] {
    const auto* theme = SkinTheme::get(this);
    const auto& pref = Preferences::instance();
    // Attempt to preserve compatibility with themes that don't have the workspaceBorder part by
    // styling it as a normal panel
    if (pref.theme.selected.defaultValue() != pref.theme.selected() &&
        theme->parts.workspaceBorder()->isDefault()) {
      recentsSection()->setStyle(theme->styles.workspaceView());
    }
    else {
      recentsSection()->setStyle(theme->styles.workspacePanel());
    }
    invalidate();
  });

  auto setRecentsMode = [recents = m_recents](const int index) {
    switch (index) {
      case 0:  recents->setMode(RecentGrid::Mode::List); break;
      case 1:  recents->setMode(RecentGrid::Mode::SmallThumbnail); break;
      case 2:  recents->setMode(RecentGrid::Mode::BigThumbnail); break;
      default: break;
    }
  };
  mode()->setSelectedItem(std::clamp<int>(preferences.home.mode(), 0, 2));
  setRecentsMode(mode()->selectedItem());
  mode()->ItemChange.connect([this, setRecentsMode](const ButtonSet::Item* item) {
    const int index = mode()->getItemIndex(item);
    setRecentsMode(index);
    App::instance()->preferences().home.mode(index);
  });

  auto updateWithRecentFiles = [this] {
    const auto* recents = App::instance()->recentFiles();
    recentFolders()->setEnabled(!recents->recentFolders().empty());
    mode()->setEnabled(!recents->recentFiles().empty() || !recents->pinnedFiles().empty());
  };
  m_recentFilesConn = App::instance()->recentFiles()->Changed.connect(updateWithRecentFiles);
  updateWithRecentFiles();
  recentFolders()->ItemChange.connect([this](const ButtonSet::Item*) {
    Menu menu;
    auto* title = new MenuSeparator;
    title->setText(Strings::home_view_recent_folders());
    menu.addChild(title);
    for (const auto& path : App::instance()->recentFiles()->recentFolders()) {
      auto* menuItem = new MenuItem(path); // TODO: Make pretty_path generic and use that?
      menuItem->Click.connect([path] { base::launcher::open_folder(path); });
      menu.addChild(menuItem);
    }
    const auto& bounds = recentFolders()->bounds();
    menu.showPopup(gfx::Point(bounds.x, bounds.y2()), display());
    recentFolders()->setSelectedItem(nullptr);
  });

#ifdef ENABLE_NEWS
  news()->Click.connect([this] { setShowNews(!m_showNews); });
  closeSidepanel()->Click.connect([this] { setShowNews(false); });
#else
  news()->Click.connect([] { base::launcher::open_url("https://blog.aseprite.org/"); });
#endif

#if ENABLE_SENTRY
  // Show this option in home tab only when we require consent for the
  // first time and there is crash data available to report

  if (Sentry::requireConsent() && Sentry::areThereCrashesToReport()) {
    shareContainer()->setVisible(true);
    shareCrashdb()->Click.connect([this] {
      if (shareCrashdb()->isSelected())
        Sentry::giveConsent();
      else
        Sentry::revokeConsent();
    });

    InitTheme.connect([this] {
      auto b = border();
      b.bottom(0);
      setBorder(b);
    });
  }
#endif

  initTheme();
}

#if ENABLE_SENTRY
void HomeView::updateConsentCheckbox()
{
  if (Sentry::requireConsent()) {
    shareContainer()->setVisible(true);
    shareCrashdb()->setSelected(false);
  }
  else if (Sentry::consentGiven()) {
    shareContainer()->setVisible(false);
    shareCrashdb()->setSelected(true);
  }
  layout();
}
#endif

std::string HomeView::getTabText()
{
  return Strings::home_view_title();
}

TabIcon HomeView::getTabIcon()
{
  return TabIcon::HOME;
}

gfx::Color HomeView::getTabColor()
{
  return gfx::ColorNone;
}

bool HomeView::onCloseView(Workspace* workspace, bool quitting)
{
  workspace->removeView(this);
  return true;
}

void HomeView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getTabPopupMenu();
  if (!menu)
    return;

  menu->showPopup(mousePosInDisplay(), display());
}

void HomeView::onWorkspaceViewSelected()
{
  StatusBar::instance()->showDefaultText();
}

void HomeView::onNewInputPriority(InputChainElement* element, const ui::Message* msg)
{
  // Do nothing
}

bool HomeView::onCanCut(Context* ctx)
{
  return false;
}

bool HomeView::onCanCopy(Context* ctx)
{
  return false;
}

bool HomeView::onCanPaste(Context* ctx)
{
  return (ctx->clipboard()->format() == ClipboardFormat::Image);
}

bool HomeView::onCanClear(Context* ctx)
{
  return false;
}

bool HomeView::onCut(Context* ctx)
{
  return false;
}

bool HomeView::onCopy(Context* ctx)
{
  return false;
}

bool HomeView::onPaste(Context* ctx, const gfx::Point* position)
{
  auto clipboard = ctx->clipboard();
  if (clipboard->format() == ClipboardFormat::Image) {
    // Create new sprite from the clipboard image.
    Params params;
    params.set("ui", "false");
    params.set("fromClipboard", "true");
    ctx->executeCommand(Commands::instance()->byId(CommandId::NewFile()), params);
    return true;
  }
  else
    return false;
}

bool HomeView::onClear(Context* ctx)
{
  // Do nothing
  return false;
}

void HomeView::onCancel(Context* ctx)
{
  // Do nothing
}

bool HomeView::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage: {
      const auto* keyMsg = static_cast<KeyMessage*>(msg);
      if (keyMsg->scancode() == kKeyTab && manager() &&
          App::instance()->workspace()->activeView() == this) {
        return manager()->processFocusMovementMessage(msg);
      }
      break;
    }
  }
  return gen::HomeView::onProcessMessage(msg);
}

void HomeView::onNewFile()
{
  Command* command = Commands::instance()->byId(CommandId::NewFile());
  UIContext::instance()->executeCommandFromMenuOrShortcut(command);
}

void HomeView::onOpenFile()
{
  Command* command = Commands::instance()->byId(CommandId::OpenFile());
  UIContext::instance()->executeCommandFromMenuOrShortcut(command);
}

#ifdef ENABLE_NEWS
void HomeView::setShowNews(const bool showNews)
{
  if (m_showNews == showNews)
    return;

  m_showNews = showNews;
  if (m_showNews && !m_news) {
    m_news = new NewsListBox;
    m_news->InitTheme.connect([this] {
      m_news->setBorder(gfx::Border(6 * guiscale()));
      m_news->setChildSpacing(6 * guiscale());
    });
    m_news->initTheme();
    m_news->setExpansive(true);
    sidepanelBox()->addChild(m_news);
  }
  App::instance()->preferences().home.showNews(m_showNews);
  sidepanel()->setVisible(m_showNews);

  newsSeparator()->setVisible(!m_showNews);
  news()->setVisible(!m_showNews);

  layout();
}
#endif

#ifdef ENABLE_UPDATER

void HomeView::onCheckingUpdates()
{
  notification()->setVisible(true);
  updateLink()->setText(Strings::home_view_checking_updates());
  layout();
}

void HomeView::onUpToDate()
{
  notification()->setVisible(false);
  layout();
}

void HomeView::onNewUpdate(const std::string& url, const std::string& version)
{
  notification()->setVisible(true);
  updateLink()->setText(Strings::home_view_new_version_available(get_app_name(), version));
  #ifdef ENABLE_DRM
  DRM_INVALID
  {
    updateLink()->setUrl(url);
  }
  else
  {
    updateLink()->setUrl("");
    updateLink()->Click.connect([version] {
      app::AsepriteUpdate dlg(version);
      dlg.openWindowInForeground();
    });
  }
  #else
  updateLink()->setUrl(url);
  #endif

  // Update the news button to highlight that there's an update, even if the sidebar is disabled.
  news()->InitTheme.connect([this] {
    const auto* theme = SkinTheme::get(this);
    news()->setStyle(theme->styles.workspaceAttentionLink());
  });
  news()->initTheme();

  layout();
}

#endif // ENABLE_UPDATER

void HomeView::onRecoverSprites()
{
#ifdef ENABLE_DATA_RECOVERY
  #ifdef ENABLE_TRIAL_MODE
  DRM_INVALID
  {
    return;
  }
  #endif

  App::instance()->mainWindow()->showDataRecovery();

  recoverSprites()->setStyle(SkinTheme::get(this)->styles.workspaceLink());
  layout();
#endif
}

} // namespace app
