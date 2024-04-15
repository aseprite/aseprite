// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/font_popup.h"

#include "app/file_selector.h"
#include "app/font_info.h"
#include "app/font_path.h"
#include "app/i18n/strings.h"
#include "app/match_words.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/font_data.h"
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

static std::map<std::string, os::SurfaceRef> g_thumbnails;

class FontItem : public ListItem {
public:
  struct ByName { };

  FontItem(const std::string& name, ByName)
    : ListItem(name)
    , m_fontInfo(FontInfo::Type::Name, name,
                 FontInfo::kDefaultSize,
                 text::FontStyle(), true) {
    getCachedThumbnail();
  }

  FontItem(const std::string& fn)
    : ListItem(base::get_file_title(fn))
    , m_fontInfo(FontInfo::Type::File, fn,
                 FontInfo::kDefaultSize,
                 text::FontStyle(), true) {
    getCachedThumbnail();
  }

  FontItem(const std::string& name,
           const text::FontStyle& style,
           const text::FontStyleSetRef& set,
           const text::TypefaceRef& typeface)
    : ListItem(name)
    , m_fontInfo(FontInfo::Type::System, name,
                 FontInfo::kDefaultSize,
                 style, true, typeface)
    , m_set(set) {
    getCachedThumbnail();
  }

  FontInfo fontInfo() const {
    return m_fontInfo;
  }

  obs::signal<void()> ThumbnailGenerated;

private:
  void getCachedThumbnail() {
    m_thumbnail = g_thumbnails[m_fontInfo.thumbnailId()];
  }

  void onPaint(PaintEvent& ev) override {
    ListItem::onPaint(ev);

    generateThumbnail();

    if (m_thumbnail) {
      Graphics* g = ev.graphics();
      g->drawRgbaSurface(m_thumbnail.get(), textWidth()+4, 0);
    }
  }

  void onSizeHint(SizeHintEvent& ev) override {
    ListItem::onSizeHint(ev);
    if (m_thumbnail) {
      gfx::Size sz = ev.sizeHint();
      ev.setSizeHint(
        sz.w + 4 + m_thumbnail->width(),
        std::max(sz.h, m_thumbnail->height()));
    }
  }

  void generateThumbnail() {
    if (m_thumbnail)
      return;

    const auto* theme = app::skin::SkinTheme::get(this);

    try {
      const text::FontMgrRef fontMgr = theme->fontMgr();
      const gfx::Color color = theme->colors.text();
      doc::ImageRef image =
        render_text(fontMgr, m_fontInfo, text(), color);
      if (!image)
        return;

      // Convert the doc::Image into a os::Surface
      m_thumbnail = os::System::instance()
        ->makeRgbaSurface(image->width(),
                          image->height());
      convert_image_to_surface(
        image.get(), nullptr, m_thumbnail.get(),
        0, 0, 0, 0, image->width(), image->height());

      // Save the thumbnail for future FontPopups
      g_thumbnails[m_fontInfo.thumbnailId()] = m_thumbnail;

      ThumbnailGenerated();
    }
    catch (const std::exception&) {
      // Ignore errors
    }
  }

  void onSelect(bool selected) override {
    if (!selected || m_thumbnail)
      return;

    ListBox* listbox = static_cast<ListBox*>(parent());
    if (!listbox)
     return;

    generateThumbnail();
    listbox->makeChildVisible(this);
  }

private:
  os::SurfaceRef m_thumbnail;
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

FontPopup::FontPopup(const FontInfo& fontInfo)
  : PopupWindow(std::string(),
                ClickBehavior::CloseOnClickInOtherWindow,
                EnterBehavior::DoNothingOnEnter)
  , m_popup(new gen::FontPopup())
  , m_timer(100)
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  addChild(m_popup);

  m_timer.Tick.connect([this]{ onTickRelayout(); });
  m_popup->loadFont()->Click.connect([this]{ onLoadFont(); });
  m_listBox.setFocusMagnet(true);
  m_listBox.Change.connect([this]{
    if (m_listBox.hasFocus() ||
        m_listBox.hasCapture()) {
      onFontChange();
    }
  });
  m_listBox.DoubleClickItem.connect([this]{
    onFontChange();
  });

  m_popup->view()->attachToView(&m_listBox);

  // Default fonts
  bool firstThemeFont = true;
  for (auto kv : skin::SkinTheme::get(this)->getWellKnownFonts()) {
    if (!kv.second->filename().empty()) {
      if (firstThemeFont) {
        m_listBox.addChild(new SeparatorInView(Strings::font_popup_theme_fonts()));
        firstThemeFont = false;
      }
      m_listBox.addChild(new FontItem(kv.first, FontItem::ByName()));
    }
  }

  // Create one FontItem for each font
  m_listBox.addChild(new SeparatorInView(Strings::font_popup_system_fonts()));
  bool empty = true;

  // Get system fonts from laf-text module
  const text::FontMgrRef fontMgr = theme()->fontMgr();
  const int n = fontMgr->countFamilies();
  if (n > 0) {
    for (int i=0; i<n; ++i) {
      std::string name = fontMgr->familyName(i);
      text::FontStyleSetRef set = fontMgr->familyStyleSet(i);
      if (set && set->count() > 0) {
        // Match the typeface with the default FontStyle (Normal
        // weight, Upright slant, etc.)
        auto typeface = set->matchStyle(text::FontStyle());
        if (typeface) {
          auto* item = new FontItem(name, typeface->fontStyle(),
                                    set, typeface);
          item->ThumbnailGenerated.connect([this]{ onThumbnailGenerated(); });
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
      for (const auto& file : base::list_files(fontDir)) {
        std::string fullpath = base::join_path(fontDir, file);
        if (base::is_file(fullpath))
          files.push_back(fullpath);
      }
    }

    // Sort all files by "file title"
    std::sort(
      files.begin(), files.end(),
      [](const std::string& a, const std::string& b){
        return base::utf8_icmp(base::get_file_title(a), base::get_file_title(b)) < 0;
      });

    for (auto& file : files) {
      std::string ext = base::string_to_lower(base::get_file_extension(file));
      if (ext == "ttf" || ext == "ttc" ||
          ext == "otf" || ext == "dfont") {
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

void FontPopup::focusListBox()
{
  m_listBox.requestFocus();
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

void FontPopup::showPopup(Display* display,
                          const gfx::Rect& buttonBounds)
{
  m_listBox.selectChild(nullptr);

  ui::fit_bounds(display, this,
                 gfx::Rect(buttonBounds.x, buttonBounds.y2(), 32, 32),
                 [](const gfx::Rect& workarea,
                    gfx::Rect& bounds,
                    std::function<gfx::Rect(Widget*)> getWidgetBounds) {
                   bounds.w = workarea.w / 2;
                   bounds.h = workarea.h / 2;
                 });

  openWindow();
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
    ChangeFont(fontInfo);
}

void FontPopup::onLoadFont()
{
  std::string currentFile;
  const FontInfo fontInfo = selectedFont();
  if (fontInfo.isValid() && fontInfo.type() == FontInfo::Type::File)
    currentFile = fontInfo.name();

  base::paths exts = { "ttf", "ttc", "otf", "dfont" };
  base::paths face;
  if (!show_file_selector(
        Strings::font_popup_select_truetype_fonts(),
        currentFile, exts,
        FileSelectorType::Open, face))
    return;

  ASSERT(!face.empty());
  ChangeFont(FontInfo(FontInfo::Type::File,
                      face.front()));
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
      if (keymsg->scancode() == kKeyEsc ||
          keymsg->scancode() == kKeyEnter ||
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
