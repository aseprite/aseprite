// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/font_popup.h"

#include "app/app.h"
#include "app/file_selector.h"
#include "app/fonts/font_data.h"
#include "app/fonts/font_info.h"
#include "app/fonts/font_path.h"
#include "app/fonts/fonts.h"
#include "app/i18n/strings.h"
#include "app/match_words.h"
#include "app/recent_files.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/util/conversion_to_surface.h"
#include "app/util/render_text.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "os/surface.h"
#include "os/system.h"
#include "text/text.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/fit_bounds.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "font_popup.xml.h"

#ifdef _WIN32
  #include <shlobj.h>
  #include <windows.h>
  #undef max
#endif

#include <algorithm>
#include <map>

namespace app {

using namespace ui;

namespace {

struct ThumbnailInfo {
  os::SurfaceRef surface;
  float baseline = 0.0f;
  float descent = 0.0f;
  float ascent = 0.0f;
};

static std::map<std::string, ThumbnailInfo> g_thumbnails;

} // namespace

class FontItem : public ListItem {
public:
  struct ByName {};

  explicit FontItem(const FontInfo& fontInfo)
    : ListItem(fontInfo.humanString())
    , m_fontInfo(fontInfo)
  {
    getCachedThumbnail();
  }

  FontItem(const std::string& name, ByName)
    : ListItem(name)
    , m_fontInfo(FontInfo::Type::Name,
                 name,
                 FontInfo::kDefaultSize,
                 text::FontStyle(),
                 FontInfo::Flags::Antialias)
  {
    getCachedThumbnail();
  }

  explicit FontItem(const std::string& fn)
    : ListItem(base::get_file_title(fn))
    , m_fontInfo(FontInfo::Type::File,
                 fn,
                 FontInfo::kDefaultSize,
                 text::FontStyle(),
                 FontInfo::Flags::Antialias)
  {
    getCachedThumbnail();
  }

  FontItem(const std::string& name, const text::FontStyle& style, const text::FontStyleSetRef& set)
    : ListItem(name)
    , m_fontInfo(FontInfo::Type::System,
                 name,
                 FontInfo::kDefaultSize,
                 style,
                 FontInfo::Flags::Antialias)
    , m_set(set)
  {
    getCachedThumbnail();
  }

  FontInfo fontInfo() const { return m_fontInfo; }

  obs::signal<void()> ThumbnailGenerated;

private:
  void getCachedThumbnail()
  {
    auto it = g_thumbnails.find(m_fontInfo.thumbnailId());
    if (it == g_thumbnails.end())
      return;
    m_thumbnail = it->second;
  }

  float onGetTextBaseline() const override
  {
    text::FontMetrics metrics;
    font()->metrics(&metrics);
    const float descent = std::max<float>(metrics.descent, m_thumbnail.descent);
    return bounds().h - descent;
  }

  void onPaint(PaintEvent& ev) override
  {
    ListItem::onPaint(ev);

    generateThumbnail();
    if (!m_thumbnail.surface)
      return;

    Graphics* g = ev.graphics();
    const auto* theme = app::skin::SkinTheme::get(this);
    const float y = textBaseline() - m_thumbnail.baseline;

    g->drawColoredRgbaSurface(m_thumbnail.surface.get(),
                              theme->colors.text(),
                              textWidth() + 4 * guiscale(),
                              y);
  }

  void onSizeHint(SizeHintEvent& ev) override
  {
    ListItem::onSizeHint(ev);
    if (!m_thumbnail.surface)
      return;

    text::FontMetrics metrics;
    font()->metrics(&metrics);
    const float lineHeight = std::max<float>(metrics.descent, m_thumbnail.descent) -
                             std::min<float>(metrics.ascent, m_thumbnail.ascent);

    gfx::Size sz = ev.sizeHint();
    ev.setSizeHint(sz.w + 4 * guiscale() + m_thumbnail.surface->width(),
                   std::max<float>(sz.h, lineHeight));
  }

  void generateThumbnail()
  {
    if (m_thumbnail.surface)
      return;

    const auto* theme = app::skin::SkinTheme::get(this);
    try {
      Fonts* fonts = Fonts::instance();
      const FontInfo fontInfoDefSize(m_fontInfo,
                                     FontInfo::kDefaultSize,
                                     text::FontStyle(),
                                     FontInfo::Flags::Antialias,
                                     text::FontHinting::Normal);
      const text::FontRef font = fonts->fontFromInfo(fontInfoDefSize);
      if (!font)
        return;

      if (font->type() != text::FontType::SpriteSheet)
        font->setSize(12.0f);

      text::TextBlobRef blob = text::TextBlob::MakeWithShaper(fonts->fontMgr(), font, text());
      if (!blob)
        return;

      doc::ImageRef image = render_text_blob(blob, gfx::rgba(0, 0, 0));
      if (!image)
        return;

      // This font metrics
      text::FontMetrics metrics;
      font->metrics(&metrics);
      m_thumbnail.baseline = blob->baseline();
      m_thumbnail.descent = metrics.descent;
      m_thumbnail.ascent = metrics.ascent;

      // Convert the doc::Image into a os::Surface
      m_thumbnail.surface = os::System::instance()->makeRgbaSurface(image->width(),
                                                                    image->height());
      convert_image_to_surface(image.get(),
                               nullptr,
                               m_thumbnail.surface.get(),
                               0,
                               0,
                               0,
                               0,
                               image->width(),
                               image->height());

      // Save the thumbnail for future FontPopups
      g_thumbnails[m_fontInfo.thumbnailId()] = m_thumbnail;

      ThumbnailGenerated();
    }
    catch (const std::exception&) {
      // Ignore errors
    }
  }

  void onSelect(bool selected) override
  {
    if (!selected || m_thumbnail.surface)
      return;

    ListBox* listbox = static_cast<ListBox*>(parent());
    if (!listbox)
      return;

    generateThumbnail();
    listbox->makeChildVisible(this);
  }

private:
  ThumbnailInfo m_thumbnail;
  FontInfo m_fontInfo;
  text::FontStyleSetRef m_set;
};

bool FontPopup::FontListBox::onProcessMessage(ui::Message* msg)
{
  const bool result = ui::ListBox::onProcessMessage(msg);

  // When we release the mouse button we close the popup, i.e. like
  // selecting an item from a combo box.
  if (msg->type() == ui::kMouseUpMessage) {
    if (auto* win = this->window()) {
      win->closeWindow(nullptr);
    }
  }

  return result;
}

bool FontPopup::FontListBox::onAcceptKeyInput()
{
  // Always accept a kKeyDownMessage so we can get Up/Down keyboard
  // messages from the FontEntry field (when the user is editing the
  // font name).
  return true;
}

FontPopup::FontPopup(const FontInfo& fontInfo)
  : PopupWindow(std::string(),
                ClickBehavior::CloseOnClickInOtherWindow,
                EnterBehavior::DoNothingOnEnter)
  , m_popup(new gen::FontPopup())
  , m_timer(100)
{
  setAutoRemap(false);
  setBorder(gfx::Border(4 * guiscale()));

  addChild(m_popup);

  m_timer.Tick.connect([this] { onTickRelayout(); });
  m_popup->loadFont()->Click.connect([this] { onLoadFont(); });
  m_listBox.setFocusMagnet(true);
  m_listBox.Change.connect([this] {
    if (m_listBox.hasFocus() || m_listBox.hasCapture()) {
      onFontChange();
    }
  });
  m_listBox.DoubleClickItem.connect([this] { onFontChange(); });

  m_popup->view()->attachToView(&m_listBox);

  // Pinned fonts
  m_pinnedSeparator = new SeparatorInView(Strings::font_popup_pinned_fonts());
  m_listBox.addChild(m_pinnedSeparator);

  // Default fonts
  Fonts* fonts = Fonts::instance();
  bool first = true;
  for (const auto& kv : fonts->definedFonts()) {
    if (!kv.second->filename().empty()) {
      if (first) {
        m_listBox.addChild(new SeparatorInView(Strings::font_popup_theme_fonts()));
        first = false;
      }
      m_listBox.addChild(new FontItem(kv.first, FontItem::ByName()));
    }
  }

  // Create one FontItem for each font
  m_listBox.addChild(new SeparatorInView(Strings::font_popup_system_fonts()));
  bool empty = true;

  // Get system fonts from laf-text module
  const text::FontMgrRef fontMgr = fonts->fontMgr();
  const int n = fontMgr->countFamilies();
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      std::string name = fontMgr->familyName(i);
      text::FontStyleSetRef set = fontMgr->familyStyleSet(i);
      if (set && set->count() > 0) {
        // Match the typeface with the default FontStyle (Normal
        // weight, Upright slant, etc.)
        auto typeface = set->matchStyle(text::FontStyle());
        if (typeface) {
          auto* item = new FontItem(name, typeface->fontStyle(), set);
          item->ThumbnailGenerated.connect([this] { onThumbnailGenerated(); });
          m_listBox.addChild(item);
          empty = false;
        }
      }
    }
  }
  // Get fonts listing .ttf files TODO we should be able to remove
  // this code in the future (probably after DirectWrite API is always
  // available).
  else {
    base::paths fontDirs;
    get_font_dirs(fontDirs);

    // Create a list of fullpaths to every font found in all font
    // directories (fontDirs)
    base::paths files;
    for (const auto& fontDir : fontDirs) {
      for (const auto& file : base::list_files(fontDir, base::ItemType::Files)) {
        files.push_back(base::join_path(fontDir, file));
      }
    }

    // Sort all files by "file title"
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
      return base::utf8_icmp(base::get_file_title(a), base::get_file_title(b)) < 0;
    });

    for (auto& file : files) {
      std::string ext = base::string_to_lower(base::get_file_extension(file));
      if (ext == "ttf" || ext == "ttc" || ext == "otf" || ext == "dfont") {
        m_listBox.addChild(new FontItem(file));
        empty = false;
      }
    }
  }

  if (empty)
    m_listBox.addChild(new ListItem(Strings::font_popup_empty_fonts()));

  for (auto* child : m_listBox.children()) {
    if (auto* childItem = dynamic_cast<FontItem*>(child)) {
      if (childItem->fontInfo().title() == childItem->text()) {
        m_listBox.selectChild(childItem);
        break;
      }
    }
  }
}

FontPopup::~FontPopup()
{
  m_timer.stop();
}

void FontPopup::setSearchText(const std::string& searchText)
{
  FontItem* firstItem = nullptr;

  const MatchWords match(searchText);
  for (auto* child : m_listBox.children()) {
    auto* childItem = dynamic_cast<FontItem*>(child);
    if (!childItem)
      continue;

    const bool visible = match(childItem->text());
    if (visible && !firstItem)
      firstItem = childItem;
    childItem->setVisible(visible);
  }

  m_listBox.selectChild(firstItem);
  layout();
}

void FontPopup::showPopup(Display* display, const gfx::Rect& buttonBounds)
{
  m_listBox.selectChild(nullptr);

  recreatePinnedItems();

  ui::fit_bounds(
    display,
    this,
    gfx::Rect(buttonBounds.x, buttonBounds.y2(), buttonBounds.w * 2, buttonBounds.h),
    [](const gfx::Rect& workarea,
       gfx::Rect& bounds,
       std::function<gfx::Rect(Widget*)> getWidgetBounds) { bounds.h = workarea.y2() - bounds.y; });

  openWindow();
}

void FontPopup::recreatePinnedItems()
{
  // Update list of pinned fonts
  if (m_pinnedSeparator) {
    // Delete pinned elements
    while (true) {
      Widget* next = m_pinnedSeparator->nextSibling();
      if (!next || next->type() == kSeparatorWidget)
        break;
      delete next;
    }

    // Recreate pinned elements
    auto& pinnedFonts = App::instance()->recentFiles()->pinnedFonts();
    int i = 1;
    for (const auto& fontInfoStr : pinnedFonts) {
      m_listBox.insertChild(i++, new FontItem(base::convert_to<FontInfo>(fontInfoStr)));
    }
    m_pinnedSeparator->setVisible(!pinnedFonts.empty());
    layout();
  }
}

FontInfo FontPopup::selectedFont()
{
  const FontItem* child = dynamic_cast<FontItem*>(m_listBox.getSelectedChild());
  if (child)
    return child->fontInfo();
  return FontInfo();
}

void FontPopup::onFontChange()
{
  const FontInfo fontInfo = selectedFont();
  if (fontInfo.isValid())
    FontChange(fontInfo);
}

void FontPopup::onLoadFont()
{
  std::string currentFile;
  const FontInfo fontInfo = selectedFont();
  if (fontInfo.isValid() && fontInfo.type() == FontInfo::Type::File)
    currentFile = fontInfo.name();

  base::paths exts = { "ttf", "ttc", "otf", "dfont" };
  base::paths face;
  if (!show_file_selector(Strings::font_popup_select_truetype_fonts(),
                          currentFile,
                          exts,
                          FileSelectorType::Open,
                          face))
    return;

  ASSERT(!face.empty());
  FontChange(FontInfo(FontInfo::Type::File, face.front()));
}

void FontPopup::onThumbnailGenerated()
{
  m_timer.start();
}

void FontPopup::onTickRelayout()
{
  m_popup->view()->updateView();
  m_timer.stop();
}

bool FontPopup::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kKeyDownMessage: {
      const auto* keymsg = static_cast<const KeyMessage*>(msg);

      // Pressing Esc or Enter will just close the popup.
      if (keymsg->scancode() == kKeyEsc || keymsg->scancode() == kKeyEnter ||
          keymsg->scancode() == kKeyEnterPad) {
        EscKey();
        return true;
      }
      break;
    }
  }
  return ui::PopupWindow::onProcessMessage(msg);
}

} // namespace app
