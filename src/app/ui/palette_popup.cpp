// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/palette_popup.h"

#include "app/app.h"
#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/i18n/strings.h"
#include "app/launcher.h"
#include "app/match_words.h"
#include "app/res/palettes_loader_delegate.h"
#include "app/res/resource.h"
#include "app/ui/palettes_listbox.h"
#include "app/ui/search_entry.h"
#include "app/ui_context.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/fit_bounds.h"
#include "ui/keys.h"
#include "ui/message.h"
#include "ui/scale.h"
#include "ui/theme.h"
#include "ui/view.h"

#ifdef ENABLE_LOSPEC
  #include "app/file/palette_file.h"
  #include "app/modules/palettes.h"
  #include "app/res/http_loader.h"
  #include "base/fs.h"
  #include "doc/palette.h"
  #include "ui/alert.h"
#endif

#include "palette_popup.xml.h"

namespace app {

using namespace ui;

#ifdef ENABLE_LOSPEC
namespace {

// Turns a Lospec palette title into a safe filename (without
// extension) to be used as a preset name.
std::string sanitize_preset_name(const std::string& title)
{
  std::string out;
  out.reserve(title.size());
  for (const char ch : title) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
        ch == ' ' || ch == '-' || ch == '_' || ch == '.' || ch == '(' || ch == ')') {
      out.push_back(ch);
    }
    else {
      out.push_back('_');
    }
  }
  // Trim surrounding spaces/dots that could be problematic.
  while (!out.empty() && (out.front() == ' ' || out.front() == '.'))
    out.erase(out.begin());
  while (!out.empty() && (out.back() == ' ' || out.back() == '.'))
    out.pop_back();
  if (out.empty())
    out = "lospec-palette";
  return out;
}

// Options offered in the color-count filter combo box (in order).
struct ColorFilterOption {
  const char* label;
  lospec::ColorFilter filter;
  int colorNumber;
};

const ColorFilterOption kColorFilterOptions[] = {
  { "Any colors", lospec::ColorFilter::Any,   0  },
  { "4 colors",   lospec::ColorFilter::Exact, 4  },
  { "8 colors",   lospec::ColorFilter::Exact, 8  },
  { "16 colors",  lospec::ColorFilter::Exact, 16 },
  { "32 colors",  lospec::ColorFilter::Exact, 32 },
  { "64 colors",  lospec::ColorFilter::Exact, 64 },
  { "64+ colors", lospec::ColorFilter::Min,   64 },
};

} // anonymous namespace
#endif // ENABLE_LOSPEC

PalettePopup::PalettePopup()
  : PopupWindow("Palettes", ClickBehavior::CloseOnClickInOtherWindow)
  , m_popup(new gen::PalettePopup())
#ifdef ENABLE_LOSPEC
  , m_importTimer(100, this)
#endif
{
  setAutoRemap(false);
  setEnterBehavior(EnterBehavior::DoNothingOnEnter);

  addChild(m_popup);

  m_paletteListBox.DoubleClickItem.connect([this] { onLoadPal(); });
  m_paletteListBox.FinishLoading.connect([this] { onSearchChange(); });
  m_popup->search()->Change.connect([this] { onSearchChange(); });
  m_popup->refresh()->Click.connect([this] { onRefresh(); });
  m_popup->loadPal()->Click.connect([this] { onLoadPal(); });
  m_popup->openFolder()->Click.connect([this] { onOpenFolder(); });

  m_popup->view()->attachToView(&m_paletteListBox);

  m_paletteListBox.PalChange.connect(&PalettePopup::onPalChange, this);

#ifdef ENABLE_LOSPEC
  // Lospec online mode.
  m_popup->lospec()->Click.connect([this] { onToggleLospec(); });
  m_popup->importPal()->Click.connect([this] { onImportPal(); });
  m_popup->importUsePal()->Click.connect([this] { onImportUsePal(); });
  m_popup->lospecView()->attachToView(&m_lospecListBox);
  m_lospecListBox.DoubleClickItem.connect([this] { onImportPal(); });
  m_lospecListBox.Change.connect([this] { onLospecSelChange(); });
  m_lospecListBox.FinishLoading.connect([this] { onLospecSelChange(); });
  m_lospecListBox.ImportPalette.connect(
    [this](const lospec::PaletteInfo& info) { onImportPalette(info, false); });
  m_importTimer.Tick.connect([this] { onTick(); });

  // Populate the color-count filter combo box.
  for (const auto& opt : kColorFilterOptions)
    m_popup->colorFilter()->addItem(opt.label);
  m_popup->colorFilter()->setSelectedItemIndex(0);
  m_popup->colorFilter()->Change.connect([this] { startLospecSearch(); });

  // Start in local-presets mode.
  setLospecMode(false);
#else
  // No Lospec support: hide the related widgets.
  m_popup->lospec()->setVisible(false);
  m_popup->importPal()->setVisible(false);
  m_popup->importUsePal()->setVisible(false);
  m_popup->lospecView()->setVisible(false);
  m_popup->colorFilter()->setVisible(false);
#endif

  InitTheme.connect([this] { setBorder(gfx::Border(4 * guiscale())); });
  initTheme();
}

PalettePopup::~PalettePopup()
{
#ifdef ENABLE_LOSPEC
  if (m_importTimer.isRunning())
    m_importTimer.stop();
  if (m_importLoader) {
    m_importLoader->abort();
    delete m_importLoader;
    m_importLoader = nullptr;
  }
#endif
}

void PalettePopup::showPopup(ui::Display* display, const gfx::Rect& buttonPos)
{
  m_popup->loadPal()->setEnabled(false);
  m_popup->openFolder()->setEnabled(false);
#ifdef ENABLE_LOSPEC
  m_popup->importPal()->setEnabled(false);
  m_popup->importUsePal()->setEnabled(false);
#endif
  m_paletteListBox.selectChild(NULL);

  fit_bounds(display,
             this,
             gfx::Rect(buttonPos.x, buttonPos.y2(), 32, 32),
             [](const gfx::Rect& workarea,
                gfx::Rect& bounds,
                std::function<gfx::Rect(Widget*)> getWidgetBounds) {
               bounds.w = workarea.w / 2;
               bounds.h = workarea.h * 3 / 4;
             });

  openWindowInForeground();
}

bool PalettePopup::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage: {
      KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
      KeyScancode scancode = keyMsg->scancode();
      bool refresh = (scancode == kKeyF5 || (msg->ctrlPressed() && scancode == kKeyR) ||
                      (msg->cmdPressed() && scancode == kKeyR));
      if (refresh) {
        onRefresh();
        return true;
      }
      break;
    }
  }
  return ui::PopupWindow::onProcessMessage(msg);
}

void PalettePopup::onPalChange(const doc::Palette* palette)
{
#ifdef ENABLE_LOSPEC
  if (m_lospecMode)
    return;
#endif

  const bool state = (UIContext::instance()->activeDocument() && palette != nullptr);

  m_popup->loadPal()->setEnabled(state);
  m_popup->openFolder()->setEnabled(state);
}

void PalettePopup::onSearchChange()
{
#ifdef ENABLE_LOSPEC
  // In Lospec mode the search box queries the online palette list.
  if (m_lospecMode) {
    startLospecSearch();
    return;
  }
#endif

  MatchWords match(m_popup->search()->text());
  bool selected = false;

  for (auto child : m_paletteListBox.children()) {
    if (dynamic_cast<ResourceListItem*>(child)) {
      const bool vis = match(child->text());
      child->setVisible(vis);
      if (!selected && vis) {
        selected = true;
        m_paletteListBox.selectChild(child);
      }
    }
    else
      child->setVisible(true);
  }

  if (!selected)
    m_paletteListBox.selectChild(nullptr);

  m_popup->view()->layout();
}

void PalettePopup::onRefresh()
{
#ifdef ENABLE_LOSPEC
  if (m_lospecMode) {
    startLospecSearch();
    return;
  }
#endif
  m_paletteListBox.reload();
}

void PalettePopup::onLoadPal()
{
  const doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    Commands::instance()->byId(CommandId::SetPalette()));
  cmd->setPalette(palette);
  UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);

  m_paletteListBox.requestFocus();
  m_paletteListBox.invalidate();
}

void PalettePopup::onOpenFolder()
{
  Resource* res = m_paletteListBox.selectedResource();
  if (!res)
    return;

  launcher::open_folder(res->path());
}

// ----------------------------------------------------------------------
// Lospec online mode

#ifdef ENABLE_LOSPEC

void PalettePopup::onToggleLospec()
{
  setLospecMode(!m_lospecMode);
}

void PalettePopup::startLospecSearch()
{
  int idx = m_popup->colorFilter()->getSelectedItemIndex();
  if (idx < 0 || idx >= int(sizeof(kColorFilterOptions) / sizeof(kColorFilterOptions[0])))
    idx = 0;

  const ColorFilterOption& opt = kColorFilterOptions[idx];
  m_lospecListBox.search(m_popup->search()->text(), opt.filter, opt.colorNumber);
}

void PalettePopup::setLospecMode(bool online)
{
  m_lospecMode = online;

  m_popup->view()->setVisible(!online);
  m_popup->lospecView()->setVisible(online);

  // The load/open-folder buttons only make sense for local presets,
  // while the import buttons only make sense for the online list.
  m_popup->loadPal()->setVisible(!online);
  m_popup->openFolder()->setVisible(!online);
  m_popup->importPal()->setVisible(online);
  m_popup->importUsePal()->setVisible(online);

  // The color-count filter only applies to the online list.
  m_popup->colorFilter()->setVisible(online);

  m_popup->lospec()->setSelected(online);

  // Clear the search box when switching modes.
  m_popup->search()->setText("");

  updateButtons();

  if (online) {
    // Reset the color filter and load the most popular palettes.
    m_popup->colorFilter()->setSelectedItemIndex(0);
    startLospecSearch();
  }
  else {
    onSearchChange();
  }

  layout();
}

void PalettePopup::onLospecSelChange()
{
  updateButtons();
}

void PalettePopup::updateButtons()
{
  if (m_lospecMode) {
    const bool hasSel = (m_lospecListBox.selectedPalette() != nullptr);
    const bool idle = (m_importLoader == nullptr);
    m_popup->importPal()->setEnabled(hasSel && idle);
    // "Import & Use" additionally needs an active sprite to apply the
    // palette to.
    m_popup->importUsePal()->setEnabled(hasSel && idle &&
                                        UIContext::instance()->activeDocument() != nullptr);
  }
}

void PalettePopup::onImportPal()
{
  const lospec::PaletteInfo* info = m_lospecListBox.selectedPalette();
  if (info)
    onImportPalette(*info, false);
}

void PalettePopup::onImportUsePal()
{
  const lospec::PaletteInfo* info = m_lospecListBox.selectedPalette();
  if (info)
    onImportPalette(*info, true);
}

void PalettePopup::onImportPalette(const lospec::PaletteInfo& info, bool andUse)
{
  // Already importing something.
  if (m_importLoader)
    return;

  m_importAndUse = andUse;
  m_importTitle = info.title;
  m_importLoader = new HttpLoader(lospec::make_download_url(info.slug));
  m_importTimer.start();
  updateButtons();
}

void PalettePopup::onTick()
{
  if (!m_importLoader || !m_importLoader->isDone())
    return;

  const std::string fn = m_importLoader->filename();
  const std::string title = m_importTitle;

  delete m_importLoader;
  m_importLoader = nullptr;
  m_importTimer.stop();

  if (fn.empty()) {
    ui::Alert::show(Strings::alerts_error_loading_file(title));
    updateButtons();
    return;
  }

  // Load the downloaded .gpl and save it as a preset so it shows up in
  // the local palette list.
  std::unique_ptr<doc::Palette> pal = load_palette(fn.c_str());
  if (!pal) {
    ui::Alert::show(Strings::alerts_error_loading_file(title));
    updateButtons();
    return;
  }

  const std::string preset = sanitize_preset_name(title);
  const std::string presetFn = get_preset_palette_filename(preset, ".gpl");
  if (!save_palette(presetFn.c_str(), pal.get(), 0, nullptr)) {
    ui::Alert::show(Strings::alerts_error_saving_file(presetFn));
    updateButtons();
    return;
  }

  // Reload the local presets list so the new palette appears there.
  App::instance()->PalettePresetsChange();

  // Optionally apply the imported palette to the active sprite.
  if (m_importAndUse) {
    SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
      Commands::instance()->byId(CommandId::SetPalette()));
    cmd->setPalette(pal.get());
    UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
  }

  m_importAndUse = false;
  updateButtons();
}

#endif // ENABLE_LOSPEC

} // namespace app
