// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
#include "app/modules/palettes.h"
#include "app/res/palette_resource.h"
#include "app/res/palettes_loader_delegate.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/icon_button.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/launcher.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/tooltips.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace app::skin;

static bool is_url_char(int chr)
{
  return ((chr >= 'a' && chr <= 'z') ||
          (chr >= 'A' && chr <= 'Z') ||
          (chr >= '0' && chr <= '9') ||
          (chr == ':' || chr == '/' || chr == '@' ||
           chr == '?' || chr == '!' || chr == '#' ||
           chr == '-' || chr == '_' || chr == '~' ||
           chr == '.' || chr == ',' || chr == ';' ||
           chr == '*' || chr == '+' || chr == '=' ||
           chr == '[' || chr == ']' ||
           chr == '(' || chr == ')' ||
           chr == '$' || chr == '\''));
}

class PalettesListItem : public ResourceListItem {

  class CommentButton : public IconButton {
  public:
    CommentButton(const std::string& comment)
      : IconButton(SkinTheme::instance()->parts.iconUserData())
      , m_comment(comment) {
      setFocusStop(false);
      initTheme();
    }

  private:
    void onInitTheme(InitThemeEvent& ev) override {
      IconButton::onInitTheme(ev);

      auto theme = SkinTheme::get(this);
      setBgColor(theme->colors.listitemNormalFace());
    }

    void onClick() override {
      IconButton::onClick();

      std::string::size_type j, i = m_comment.find("http");
      if (i != std::string::npos) {
        for (j=i+4; j != m_comment.size() && is_url_char(m_comment[j]); ++j)
          ;
        base::launcher::open_url(m_comment.substr(i, j-i));
      }
    }

    std::string m_comment;
  };

public:
  PalettesListItem(Resource* resource, TooltipManager* tooltips)
    : ResourceListItem(resource)
    , m_comment(nullptr)
  {
    std::string comment = static_cast<PaletteResource*>(resource)->palette()->comment();
    if (!comment.empty()) {
      addChild(m_comment = new CommentButton(comment));

      tooltips->addTooltipFor(m_comment, comment, LEFT);
    }
  }

private:
  void onResize(ResizeEvent& ev) override {
    ResourceListItem::onResize(ev);

    if (m_comment) {
      auto reqSz = m_comment->sizeHint();
      m_comment->setBounds(
        gfx::Rect(ev.bounds().x+ev.bounds().w-reqSz.w,
                  ev.bounds().y+ev.bounds().h/2-reqSz.h/2,
                  reqSz.w, reqSz.h));
    }
  }

  CommentButton* m_comment;
};

PalettesListBox::PalettesListBox()
  : ResourcesListBox(
    new ResourcesLoader(
      std::make_unique<PalettesLoaderDelegate>()))
{
  addChild(&m_tooltips);

  m_extPaletteChanges =
    App::instance()->extensions().PalettesChange.connect(
      [this]{ reload(); });
  m_extPresetsChanges =
    App::instance()->PalettePresetsChange.connect(
      [this]{ reload(); });
}

const doc::Palette* PalettesListBox::selectedPalette()
{
  Resource* resource = selectedResource();
  if (!resource)
    return NULL;

  return static_cast<PaletteResource*>(resource)->palette();
}

ResourceListItem* PalettesListBox::onCreateResourceItem(Resource* resource)
{
  return new PalettesListItem(resource, &m_tooltips);
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
      g->drawRgbaSurface(tick, bounds.x, bounds.y+bounds.h/2-tick->height()/2);
  }

  bounds.x += tick->width();
  bounds.w -= tick->width();

  gfx::Rect box(
    bounds.x, bounds.y+bounds.h-6*guiscale(),
    4*guiscale(), 4*guiscale());

  for (int i=0; i<palette->size(); ++i) {
    doc::color_t c = palette->getEntry(i);

    g->fillRect(gfx::rgba(
        doc::rgba_getr(c),
        doc::rgba_getg(c),
        doc::rgba_getb(c)), box);

    box.x += box.w;
  }
}

void PalettesListBox::onResourceSizeHint(Resource* resource, gfx::Size& size)
{
  size = gfx::Size(0, (2+16+2)*guiscale());
}

} // namespace app
