// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/palettes_listbox.h"

#include "app/app.h"
#include "app/doc.h"
#include "app/extensions.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/modules/palettes.h"
#include "app/res/palette_resource.h"
#include "app/res/palettes_loader_delegate.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/icon_button.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/tooltips.h"

namespace app {

using namespace ui;
using namespace app::skin;

static constexpr bool is_url_char(const int chr)
{
  return ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || (chr >= '0' && chr <= '9') ||
          (chr == ':' || chr == '/' || chr == '@' || chr == '?' || chr == '!' || chr == '#' ||
           chr == '-' || chr == '_' || chr == '~' || chr == '.' || chr == ',' || chr == ';' ||
           chr == '*' || chr == '+' || chr == '=' || chr == '[' || chr == ']' || chr == '(' ||
           chr == ')' || chr == '$' || chr == '\''));
}

constexpr const char* kSectionName = "PinnedPalettes";
constexpr const uint8_t kPinnedLimit = 9999;

class PalettesListItem : public ResourceListItem {
  class CommentButton : public IconButton {
  public:
    explicit CommentButton(const std::string& comment)
      : IconButton(SkinTheme::instance()->parts.iconUserData())
      , m_comment(comment)
    {
      setFocusStop(false);
      setTransparent(true);
    }

  private:
    void onClick() override
    {
      IconButton::onClick();

      std::string::size_type j, i = m_comment.find("http");
      if (i != std::string::npos) {
        for (j = i + 4; j != m_comment.size() && is_url_char(m_comment[j]); ++j)
          ;
        base::launcher::open_url(m_comment.substr(i, j - i));
      }
    }

    std::string m_comment;
  };

public:
  PalettesListItem(Resource* resource, TooltipManager* tooltips, bool isPinned)
    : ResourceListItem(resource)
    , m_resource(resource)
    , m_tooltips(tooltips)
  {
    auto* filler = new BoxFiller;
    m_hbox.setFocusStop(false);
    m_hbox.setTransparent(true);
    filler->setTransparent(true);
    filler->setFocusStop(false);
    m_hbox.addChild(filler);

    const std::string& comment = static_cast<PaletteResource*>(resource)->palette()->comment();
    if (!comment.empty()) {
      auto* commentButton = new CommentButton(comment);
      tooltips->addTooltipFor(commentButton, comment, LEFT);
      m_hbox.addChild(commentButton);
    }

    setPinned(isPinned);

    addChild(&m_hbox);
    m_hbox.setVisible(false);
    m_initialized = true;
  }

  void setPinned(bool isPinned)
  {
    const auto& part = isPinned ? SkinTheme::instance()->parts.pinned() :
                                  SkinTheme::instance()->parts.unpinned();
    IconButton* pinnedButton;

    if (m_initialized) {
      pinnedButton = static_cast<IconButton*>(m_hbox.lastChild());
      pinnedButton->setIcon(part);
      m_tooltips->removeTooltipFor(pinnedButton);
    }
    else {
      pinnedButton = new IconButton(part);
      pinnedButton->setTransparent(true);
      pinnedButton->setFocusStop(false);

      const std::string resourceId = m_resource->id();
      pinnedButton->Click.connect([&, resourceId] {
        if (auto* p = dynamic_cast<PalettesListBox*>(parent()))
          p->togglePinned(resourceId);
      });

      m_hbox.addChild(pinnedButton);
    }

    m_tooltips->addTooltipFor(
      pinnedButton,
      isPinned ? Strings::resource_listbox_unpin() : Strings::resource_listbox_pin(),
      LEFT);
  }

  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kMouseLeaveMessage: {
        m_hbox.setVisible(false);
        invalidate();
        break;
      }
      case kMouseEnterMessage: {
        m_hbox.setVisible(true);
        invalidate();
        break;
      }
    }

    return ResourceListItem::onProcessMessage(msg);
  }

  bool m_initialized = false;
  Resource* m_resource;
  TooltipManager* m_tooltips;
  HBox m_hbox;
};

PalettesListBox::PalettesListBox()
  : ResourcesListBox(new ResourcesLoader(std::make_unique<PalettesLoaderDelegate>()))
  , m_pinnedSeparator("", HORIZONTAL)
{
  m_pinnedSeparator.setFocusStop(false);
  m_pinnedSeparator.InitTheme.connect(
    [&] { m_pinnedSeparator.setBorder(m_pinnedSeparator.border() * guiscale() * 2); });

  addChild(&m_tooltips);

  loadPinnedQuiet();
  FinishLoading.connect(&PalettesListBox::loadPinned, this);

  m_extPaletteChanges = App::instance()->extensions().PalettesChange.connect(
    [this] { markToReload(); });
  m_extPresetsChanges = App::instance()->PalettePresetsChange.connect([this] { markToReload(); });
}

void PalettesListBox::togglePinned(const std::string& paletteId)
{
  ASSERT(!paletteId.empty());

  bool isPinning = false;
  auto it = std::find(m_pinned.begin(), m_pinned.end(), paletteId);
  if (it != m_pinned.end()) {
    m_pinned.erase(it);
  }
  else {
    isPinning = true;

    if (m_pinned.size() >= kPinnedLimit)
      m_pinned.erase(m_pinned.begin()); // Pop the oldest one when going over.

    m_pinned.push_back(paletteId);
  }

  for (Widget* child : children()) {
    if (auto* item = dynamic_cast<PalettesListItem*>(child);
        item && item->m_resource->id() == paletteId) {
      item->setPinned(isPinning);

      if (isPinning) {
        item->setSelected(true);
      }

      // Hack to avoid the buttons lingering when re-sorting
      item->sendMessage(new Message(kMouseLeaveMessage));

      // We could break here, but that wouldn't let us deselect every child so our
      // new pinned can be selected and then we can center on it.
    }
    else if (child != nullptr && isPinning) {
      child->setSelected(false);
    }
  }

  toggleSeparator();
  savePinned();
  sortItems();
  layout();

  if (isPinning)
    centerScroll();
}

void PalettesListBox::loadPinned()
{
  const bool wasEmpty = m_pinned.empty();
  loadPinnedQuiet();

  if (wasEmpty && m_pinned.empty())
    return; // Avoid a relayout with no changes and no pinned palettes.

  toggleSeparator();
  sortItems();
  layout();
}

void PalettesListBox::toggleSeparator()
{
  // For some reason, just toggling visibility does not work properly, so we have to reparent.
  if (!m_pinned.empty() && m_pinnedSeparator.parent() == nullptr)
    addChild(&m_pinnedSeparator);
  else if (m_pinned.empty() && m_pinnedSeparator.parent() == this)
    removeChild(&m_pinnedSeparator);
}

void PalettesListBox::savePinned()
{
  for (const auto& key : enum_config_keys(kSectionName)) {
    del_config_value(kSectionName, key.c_str());
  }

  // Crudely compare the outgoing pins to the current list so that we can remove any that don't
  // exist anymore, faster than checking the filesystem for the palette file or doing the check when
  // we load.
  const auto& c = children();
  std::vector<std::string_view> ids;
  ids.resize(c.size());
  for (size_t i = 0; i < c.size(); ++i) {
    ids[i] = c[i]->text();
  }

  for (size_t i = 0; i < m_pinned.size(); ++i) {
    const auto& pinned = m_pinned[i];
    auto it = std::find(ids.begin(), ids.end(), pinned);
    if (it == ids.end())
      continue;

    set_config_string(kSectionName, fmt::format("{:04d}", i).c_str(), pinned.c_str());
  }
}

const Palette* PalettesListBox::selectedPalette()
{
  Resource* resource = selectedResource();
  if (!resource)
    return NULL;

  return static_cast<PaletteResource*>(resource)->palette();
}

ResourceListItem* PalettesListBox::onCreateResourceItem(Resource* resource)
{
  return new PalettesListItem(
    resource,
    &m_tooltips,
    std::find(m_pinned.begin(), m_pinned.end(), resource->id()) != m_pinned.end());
}

void PalettesListBox::onResourceChange(Resource* resource)
{
  const doc::Palette* palette = static_cast<PaletteResource*>(resource)->palette();
  PalChange(palette);
}

void PalettesListBox::onPaintResource(Graphics* g, gfx::Rect& bounds, Resource* resource)
{
  auto theme = SkinTheme::get(this);
  const doc::Palette* palette = static_cast<PaletteResource*>(resource)->palette();
  os::Surface* tick = theme->parts.checkSelected()->bitmap(0);

  // Draw tick (to say "this palette matches the active sprite
  // palette").
  auto view = UIContext::instance()->activeView();
  if (view && view->document()) {
    auto docPal = view->document()->sprite()->palette(view->editor()->frame());
    if (docPal && *docPal == *palette)
      g->drawRgbaSurface(tick, bounds.x, bounds.y + bounds.h / 2 - tick->height() / 2);
  }

  bounds.x += tick->width();
  bounds.w -= tick->width();

  gfx::Rect box(bounds.x, bounds.y + bounds.h - 6 * guiscale(), 4 * guiscale(), 4 * guiscale());

  for (int i = 0; i < palette->size(); ++i) {
    doc::color_t c = palette->getEntry(i);

    g->fillRect(gfx::rgba(doc::rgba_getr(c), doc::rgba_getg(c), doc::rgba_getb(c)), box);

    box.x += box.w;
  }
}

void PalettesListBox::onResourceSizeHint(Resource* resource, gfx::Size& size)
{
  size = gfx::Size(0, (2 + 16 + 2) * guiscale());
}

void PalettesListBox::loadPinnedQuiet()
{
  m_pinned.clear();
  for (const auto& key : enum_config_keys(kSectionName)) {
    m_pinned.push_back(get_config_string(kSectionName, key.c_str(), nullptr));
  }
  if (m_pinned.size() >= kPinnedLimit)
    m_pinned.resize(kPinnedLimit);
}

void PalettesListBox::sortItems()
{
  ListBox::sortItems([&](const Widget* a, const Widget* b) {
    const bool aPinned = std::find(m_pinned.begin(), m_pinned.end(), a->text()) != m_pinned.end();
    const bool bPinned = std::find(m_pinned.begin(), m_pinned.end(), b->text()) != m_pinned.end();

    if (aPinned == bPinned)
      return base::compare_filenames(a->text(), b->text()) < 0;

    return (aPinned && !bPinned);
  });
}

} // namespace app
