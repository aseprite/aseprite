// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/lospec_listbox.h"

#include "app/i18n/strings.h"
#include "app/res/http_loader.h"
#include "app/ui/skin/skin_theme.h"
#include "base/fstream_path.h"
#include "doc/color.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/view.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace app {

using namespace app::skin;
using namespace ui;

namespace {

// A list item that shows a Lospec palette name plus a preview row of
// its colors.
class LospecListItem : public ListItem {
public:
  LospecListItem(const lospec::PaletteInfo& info) : ListItem(info.title), m_info(info) {}

  const lospec::PaletteInfo& info() const { return m_info; }

protected:
  void onSizeHint(SizeHintEvent& ev) override
  {
    ev.setSizeHint(gfx::Size(0, (2 + 8 + 2 + 8 + 2) * guiscale()));
  }

  void onPaint(PaintEvent& ev) override
  {
    auto theme = SkinTheme::get(this);
    Graphics* g = ev.graphics();
    gfx::Rect bounds = clientBounds();
    gfx::Color bgcolor, fgcolor;

    if (isSelected()) {
      bgcolor = theme->colors.listitemSelectedFace();
      fgcolor = theme->colors.listitemSelectedText();
    }
    else {
      bgcolor = theme->colors.listitemNormalFace();
      fgcolor = theme->colors.listitemNormalText();
    }

    g->fillRect(bgcolor, bounds);

    g->drawText(this->text(),
                fgcolor,
                gfx::ColorNone,
                gfx::Point(bounds.x + 2 * guiscale(), bounds.y + 2 * guiscale()));

    // Color preview row.
    if (!m_info.colors.empty()) {
      const int sw = 4 * guiscale();
      gfx::Rect box(bounds.x + 2 * guiscale(),
                    bounds.y + bounds.h - sw - 2 * guiscale(),
                    sw,
                    sw);
      const int maxColors = std::min<int>(int(m_info.colors.size()),
                                          std::max(1, (bounds.w - 4 * guiscale()) / sw));
      for (int i = 0; i < maxColors; ++i) {
        const doc::color_t c = m_info.colors[i];
        g->fillRect(gfx::rgba(doc::rgba_getr(c), doc::rgba_getg(c), doc::rgba_getb(c)), box);
        box.x += box.w;
      }
    }
  }

private:
  lospec::PaletteInfo m_info;
};

// Plain message item (e.g. "No results" / "Problem loading" /
// loading progress).
class MessageItem : public ListItem {
public:
  MessageItem(const std::string& text) : ListItem(text) {}
};

} // anonymous namespace

LospecListBox::LospecListBox() : m_timer(100, this), m_loader(nullptr)
{
  m_timer.Tick.connect(&LospecListBox::onTick, this);
}

LospecListBox::~LospecListBox()
{
  stopLoading();
}

void LospecListBox::search(const std::string& query,
                          lospec::ColorFilter filter,
                          int colorNumber)
{
  stopLoading();

  m_query = query;
  m_filter = filter;
  m_colorNumber = colorNumber;
  m_loadedCount = 0;
  m_totalCount = -1;
  m_progressItem = nullptr;
  clearItems();

  if (View* view = View::getView(this))
    view->updateView();

  // Start loading from the first page; the rest are fetched
  // automatically as each page arrives.
  m_loading = true;
  startLoadingPage(1);
}

void LospecListBox::startLoadingPage(int page)
{
  m_page = page;
  delete m_loader;
  m_loader = new HttpLoader(lospec::make_search_url(m_query, page, m_filter, m_colorNumber));
  if (!m_timer.isRunning())
    m_timer.start();
}

void LospecListBox::stopLoading()
{
  m_loading = false;
  if (m_timer.isRunning())
    m_timer.stop();
  if (m_loader) {
    m_loader->abort();
    delete m_loader;
    m_loader = nullptr;
  }
}

const lospec::PaletteInfo* LospecListBox::selectedPalette() const
{
  if (auto* item = dynamic_cast<LospecListItem*>(const_cast<LospecListBox*>(this)->getSelectedChild()))
    return &item->info();
  return nullptr;
}

void LospecListBox::clearItems()
{
  while (auto child = lastChild())
    removeChild(child);
}

bool LospecListBox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kCloseMessage:
      if (m_loader)
        m_loader->abort();
      break;
  }
  return ListBox::onProcessMessage(msg);
}

void LospecListBox::onChange()
{
  // Forward double-clicks via DoubleClickItem in the popup; here we
  // just keep default selection behavior.
  ListBox::onChange();
}

void LospecListBox::updateProgress()
{
  if (!m_loading)
    return;

  std::string text;
  if (m_totalCount > 0)
    text = Strings::lospec_listbox_loading_progress(std::to_string(m_loadedCount),
                                                    std::to_string(m_totalCount));
  else
    text = Strings::lospec_listbox_loading();

  if (!m_progressItem) {
    m_progressItem = new MessageItem(text);
    addChild(m_progressItem);
  }
  else {
    m_progressItem->setText(text);
  }
}

void LospecListBox::onTick()
{
  if (!m_loader || !m_loader->isDone())
    return;

  const std::string fn = m_loader->filename();
  delete m_loader;
  m_loader = nullptr;

  // Network/HTTP error.
  if (fn.empty()) {
    // If we already loaded some palettes, just stop quietly; otherwise
    // show an error message.
    if (m_loadedCount == 0) {
      clearItems();
      m_progressItem = nullptr;
      addChild(new MessageItem(Strings::lospec_listbox_problem_loading()));
    }
    else if (m_progressItem) {
      removeChild(m_progressItem);
      delete m_progressItem;
      m_progressItem = nullptr;
    }
    stopLoading();
    if (View* view = View::getView(this))
      view->updateView();
    layout();
    FinishLoading();
    return;
  }

  const int added = parsePage(fn);

  // Parse error on the very first page.
  if (added < 0 && m_loadedCount == 0) {
    clearItems();
    m_progressItem = nullptr;
    addChild(new MessageItem(Strings::lospec_listbox_problem_loading()));
    stopLoading();
    if (View* view = View::getView(this))
      view->updateView();
    layout();
    FinishLoading();
    return;
  }

  // No more palettes (empty page) -> we've reached the end.
  const bool reachedEnd =
    (added <= 0) || (m_totalCount > 0 && m_loadedCount >= m_totalCount);

  if (reachedEnd) {
    // Remove the progress indicator.
    if (m_progressItem) {
      removeChild(m_progressItem);
      delete m_progressItem;
      m_progressItem = nullptr;
    }

    if (m_loadedCount == 0)
      addChild(new MessageItem(Strings::lospec_listbox_no_results()));

    stopLoading();
    if (View* view = View::getView(this))
      view->updateView();
    layout();
    FinishLoading();
    return;
  }

  // Otherwise keep loading the next page.
  updateProgress();
  if (View* view = View::getView(this))
    view->updateView();
  layout();

  startLoadingPage(m_page + 1);
}

int LospecListBox::parsePage(const std::string& filename)
{
  std::string json;
  {
    std::ifstream f(FSTREAM_PATH(filename), std::ios::binary);
    if (f) {
      std::ostringstream ss;
      ss << f.rdbuf();
      json = ss.str();
    }
  }

  std::vector<lospec::PaletteInfo> palettes;
  int totalCount = -1;
  if (json.empty() || !lospec::parse_search_result(json, palettes, &totalCount))
    return -1;

  if (totalCount >= 0)
    m_totalCount = totalCount;

  // Insert the new palettes before the progress item (which lives at
  // the bottom of the list).
  int insertIndex = getItemsCount();
  if (m_progressItem)
    --insertIndex;

  for (const auto& info : palettes) {
    auto* item = new LospecListItem(info);
    insertChild(insertIndex++, item);
  }

  m_loadedCount += int(palettes.size());
  return int(palettes.size());
}

} // namespace app
