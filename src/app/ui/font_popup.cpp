// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/font_popup.h"

#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "app/util/freetype_utils.h"
#include "base/bind.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "font_popup.xml.h"

#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
#endif

#include <queue>
#include <map>

namespace app {

using namespace ui;

static std::map<std::string, doc::ImageRef> g_thumbnails;

class FontItem : public ListItem {
public:
  FontItem(const std::string& fn)
    : ListItem(base::get_file_title(fn))
    , m_image(g_thumbnails[fn])
    , m_filename(fn) {
  }

  const std::string& filename() const {
    return m_filename;
  }

private:
  void onPaint(PaintEvent& ev) override {
    ListItem::onPaint(ev);

    if (m_image) {
      Graphics* g = ev.getGraphics();
      she::Surface* sur = she::instance()->createRgbaSurface(m_image->width(),
                                                             m_image->height());

      convert_image_to_surface(
        m_image.get(), nullptr, sur,
        0, 0, 0, 0, m_image->width(), m_image->height());

      g->drawRgbaSurface(sur, getTextWidth()+4, 0);
      sur->dispose();
    }
  }

  void onSizeHint(SizeHintEvent& ev) override {
    ListItem::onSizeHint(ev);
    if (m_image) {
      gfx::Size sz = ev.sizeHint();
      ev.setSizeHint(
        sz.w + 4 + m_image->width(),
        MAX(sz.h, m_image->height()));
    }
  }

  void onSelect() override {
    if (m_image)
      return;

    ListBox* listbox = static_cast<ListBox*>(getParent());
    if (!listbox)
      return;

    app::skin::SkinTheme* theme = app::skin::SkinTheme::instance();
    gfx::Color color = theme->colors.text();

    try {
      m_image.reset(
        render_text(
          m_filename, 16,
          "ABCDEabcde",             // TODO custom text
          doc::rgba(gfx::getr(color),
                    gfx::getg(color),
                    gfx::getb(color),
                    gfx::geta(color)),
          true));                   // antialias

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
  : PopupWindow("Fonts",
                kCloseOnClickInOtherWindow,
                kDoNothingOnEnter)
  , m_popup(new gen::FontPopup())
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  addChild(m_popup);

  m_popup->loadFont()->Click.connect(Bind<void>(&FontPopup::onLoadFont, this));
  m_listBox.setFocusMagnet(true);
  m_listBox.Change.connect(Bind<void>(&FontPopup::onChangeFont, this));
  m_listBox.DoubleClickItem.connect(Bind<void>(&FontPopup::onLoadFont, this));

  m_popup->view()->attachToView(&m_listBox);

  std::queue<std::string> fontDirs;
#if _WIN32
  {
    std::vector<wchar_t> buf(MAX_PATH);
    HRESULT hr = SHGetFolderPath(NULL, CSIDL_FONTS, NULL,
                                 SHGFP_TYPE_DEFAULT, &buf[0]);
    if (hr == S_OK) {
      fontDirs.push(base::to_utf8(&buf[0]));
    }
  }
#elif __APPLE__
  {
    fontDirs.push("/System/Library/Fonts/");
    fontDirs.push("/Library/Fonts");
    fontDirs.push("~/Library/Fonts");
  }
#else  // Unix-like
  {
    fontDirs.push("/usr/share/fonts");
    fontDirs.push("/usr/local/share/fonts");
    fontDirs.push("~/.fonts");
  }
#endif

  // Create a list of fullpaths to every font found in all font
  // directories (fontDirs)
  std::vector<std::string> files;
  while (!fontDirs.empty()) {
    std::string fontDir = fontDirs.front();
    fontDirs.pop();

    auto fontDirFiles = base::list_files(fontDir);
    for (const auto& file : fontDirFiles) {
      std::string fullpath = base::join_path(fontDir, file);
      if (base::is_directory(fullpath))
        fontDirs.push(fullpath); // Add subdirectory
      else
        files.push_back(fullpath);
    }
  }

  // Sort all files by "file title"
  std::sort(
    files.begin(), files.end(),
    [](const std::string& a, const std::string& b){
      return base::utf8_icmp(base::get_file_title(a), base::get_file_title(b)) < 0;
    });

  // Create one FontItem for each font
  for (auto& file : files) {
    if (base::string_to_lower(base::get_file_extension(file)) == "ttf")
      m_listBox.addChild(new FontItem(file));
  }

  if (m_listBox.children().empty())
    m_listBox.addChild(new ListItem("No system fonts were found"));
}

void FontPopup::showPopup(const gfx::Rect& bounds)
{
  m_popup->loadFont()->setEnabled(false);
  m_listBox.selectChild(NULL);

  moveWindow(bounds);

  // Setup the hot-region
  setHotRegion(gfx::Region(gfx::Rect(bounds).enlarge(32 * guiscale())));

  openWindow();
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
    Load(filename);             // Fire Load signal

  closeWindow(nullptr);
}

} // namespace app
