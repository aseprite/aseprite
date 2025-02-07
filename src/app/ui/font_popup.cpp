// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/font_popup.h"

#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/font_path.h"
#include "app/i18n/strings.h"
#include "app/match_words.h"
#include "app/ui/search_entry.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "app/util/conversion_to_surface.h"
#include "app/util/freetype_utils.h"
#include "base/fs.h"
#include "base/string.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/fit_bounds.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
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

static std::map<std::string, doc::ImageRef> g_thumbnails;

class FontItem : public ListItem {
public:
  FontItem(const std::string& fn)
    : ListItem(base::get_file_title(fn))
    , m_image(g_thumbnails[fn])
    , m_filename(fn)
  {
  }

  const std::string& filename() const { return m_filename; }

private:
  void onPaint(PaintEvent& ev) override
  {
    ListItem::onPaint(ev);

    if (m_image) {
      Graphics* g = ev.graphics();
      os::SurfaceRef sur = os::instance()->makeRgbaSurface(m_image->width(), m_image->height());

      convert_image_to_surface(m_image.get(),
                               nullptr,
                               sur.get(),
                               0,
                               0,
                               0,
                               0,
                               m_image->width(),
                               m_image->height());

      g->drawRgbaSurface(sur.get(), textWidth() + 4, 0);
    }
  }

  void onSizeHint(SizeHintEvent& ev) override
  {
    ListItem::onSizeHint(ev);
    if (m_image) {
      gfx::Size sz = ev.sizeHint();
      ev.setSizeHint(sz.w + 4 + m_image->width(), std::max(sz.h, m_image->height()));
    }
  }

  void onSelect(bool selected) override
  {
    if (!selected || m_image)
      return;

    ListBox* listbox = static_cast<ListBox*>(parent());
    if (!listbox)
      return;

    auto theme = app::skin::SkinTheme::get(this);
    gfx::Color color = theme->colors.text();

    try {
      m_image.reset(render_text(
        m_filename,
        16,
        "ABCDEabcde", // TODO custom text
        doc::rgba(gfx::getr(color), gfx::getg(color), gfx::getb(color), gfx::geta(color)),
        true)); // antialias

      View* view = View::getView(listbox);
      view->updateView();
      listbox->makeChildVisible(this);

      // Save the thumbnail for future FontPopups
      g_thumbnails[m_filename] = m_image;
    }
    catch (const std::exception&) {
      // Ignore errors
    }
  }

private:
  doc::ImageRef m_image;
  std::string m_filename;
};

FontPopup::FontPopup()
  : PopupWindow(Strings::font_popup_title(),
                ClickBehavior::CloseOnClickInOtherWindow,
                EnterBehavior::DoNothingOnEnter)
  , m_popup(new gen::FontPopup())
{
  setAutoRemap(false);
  setBorder(gfx::Border(4 * guiscale()));

  addChild(m_popup);

  m_popup->search()->Change.connect([this] { onSearchChange(); });
  m_popup->loadFont()->Click.connect([this] { onLoadFont(); });
  m_listBox.setFocusMagnet(true);
  m_listBox.Change.connect([this] { onChangeFont(); });
  m_listBox.DoubleClickItem.connect([this] { onLoadFont(); });

  m_popup->view()->attachToView(&m_listBox);

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

  // Create one FontItem for each font
  for (auto& file : files) {
    std::string ext = base::string_to_lower(base::get_file_extension(file));
    if (ext == "ttf" || ext == "ttc" || ext == "otf" || ext == "dfont")
      m_listBox.addChild(new FontItem(file));
  }

  if (m_listBox.children().empty())
    m_listBox.addChild(new ListItem(Strings::font_popup_empty_fonts()));
}

void FontPopup::showPopup(Display* display, const gfx::Rect& buttonBounds)
{
  m_popup->loadFont()->setEnabled(false);
  m_listBox.selectChild(NULL);

  ui::fit_bounds(display,
                 this,
                 gfx::Rect(buttonBounds.x, buttonBounds.y2(), 32, 32),
                 [](const gfx::Rect& workarea,
                    gfx::Rect& bounds,
                    std::function<gfx::Rect(Widget*)> getWidgetBounds) {
                   bounds.w = workarea.w / 2;
                   bounds.h = workarea.h / 2;
                 });

  // Setup the hot-region
  setHotRegion(
    gfx::Region(gfx::Rect(boundsOnScreen()).enlarge(32 * guiscale() * display->scale())));

  openWindow();
}

void FontPopup::onSearchChange()
{
  std::string searchText = m_popup->search()->text();
  Widget* firstItem = nullptr;

  MatchWords match(searchText);
  for (auto child : m_listBox.children()) {
    bool visible = match(child->text());
    if (visible && !firstItem)
      firstItem = child;
    child->setVisible(visible);
  }

  m_listBox.selectChild(firstItem);
  layout();
}

void FontPopup::onChangeFont()
{
  m_popup->loadFont()->setEnabled(true);
}

void FontPopup::onLoadFont()
{
  FontItem* child = dynamic_cast<FontItem*>(m_listBox.getSelectedChild());
  if (!child)
    return;

  std::string filename = child->filename();
  if (base::is_file(filename))
    Load(filename); // Fire Load signal

  closeWindow(nullptr);
}

} // namespace app
