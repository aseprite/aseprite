// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_shades.h"

#include "app/app.h"
#include "app/modules/gfx.h"
#include "app/modules/palettes.h"
#include "app/shade.h"
#include "app/ui/color_bar.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "doc/color_mode.h"
#include "doc/palette.h"
#include "doc/palette_picks.h"
#include "doc/remap.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include <algorithm>


namespace app {

ColorShades::ColorShades(const Shade& colors, ClickType click)
  : Widget(ui::kGenericWidget)
  , m_click(click)
  , m_shade(colors)
  , m_minColors(1)
  , m_hotIndex(-1)
  , m_dragIndex(-1)
  , m_boxSize(12)
{
  setText("No colors");
  initTheme();
}

void ColorShades::setMinColors(int minColors)
{
  m_minColors = minColors;
  invalidate();
}

void ColorShades::reverseShadeColors()
{
  std::reverse(m_shade.begin(), m_shade.end());
  invalidate();
}

doc::Remap* ColorShades::createShadeRemap(bool left)
{
  // We need two or more colors to create a shading remap. In
  // other case, the ShadingInkProcessing will use the full
  // color palette.
  Shade colors = getShade();
  if (colors.size() <= 1)
    return nullptr;

  std::unique_ptr<doc::Remap> remap(
    new doc::Remap(get_current_palette()->size()));

  for (int i=0; i<remap->size(); ++i)
    remap->map(i, i);

  if (left) {
    for (int i=1; i<int(colors.size()); ++i) {
      int j = colors[i].getIndex();
      if (j >= 0 && j < remap->size())
        remap->map(j, colors[i-1].getIndex());
    }
  }
  else {
    for (int i=0; i<int(colors.size())-1; ++i) {
      int j = colors[i].getIndex();
      if (j >= 0 && j < remap->size())
        remap->map(j, colors[i+1].getIndex());
    }
  }
  return remap.release();
}

void ColorShades::setShade(const Shade& shade)
{
  m_shade = shade;
  invalidate();
  parent()->parent()->layout();
}

void ColorShades::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  auto theme = skin::SkinTheme::instance();

  switch (m_click) {
    case ClickEntries:
    case DragAndDropEntries:
      setStyle(theme->styles.normalShadeView());
      break;
    case ClickWholeShade:
      setStyle(theme->styles.menuShadeView());
      break;
  }
}

bool ColorShades::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case ui::kSetCursorMessage:
      if (hasCapture()) {
        ui::set_mouse_cursor(ui::kMoveCursor);
        return true;
      }
      else if (m_click == ClickEntries &&
               m_hotIndex >= 0 &&
               m_hotIndex < int(m_shade.size())) {
        ui::set_mouse_cursor(ui::kHandCursor);
        return true;
      }
      break;

    case ui::kMouseEnterMessage:
    case ui::kMouseLeaveMessage:
      if (!hasCapture())
        m_hotIndex = -1;

      invalidate();
      break;

    case ui::kMouseDownMessage:
      if (m_hotIndex >= 0 &&
          m_hotIndex < int(m_shade.size())) {
        switch (m_click) {
          case ClickEntries: {
            ClickEvent ev(static_cast<ui::MouseMessage*>(msg)->button());
            Click(ev);

            m_hotIndex = -1;
            invalidate();
            break;
          }
          case DragAndDropEntries:
            m_dragIndex = m_hotIndex;
            m_dropBefore = false;
            captureMouse();
            break;
        }
      }
      break;

    case ui::kMouseUpMessage: {
      if (m_click == ClickWholeShade) {
        setSelected(true);

        ClickEvent ev(static_cast<ui::MouseMessage*>(msg)->button());
        Click(ev);

        closeWindow();
      }

      if (m_dragIndex >= 0) {
        ASSERT(m_dragIndex < int(m_shade.size()));

        auto color = m_shade[m_dragIndex];
        m_shade.erase(m_shade.begin()+m_dragIndex);
        if (m_hotIndex >= 0)
          m_shade.insert(m_shade.begin()+m_hotIndex, color);

        m_dragIndex = -1;
        invalidate();

        // Relayout the context bar if we have removed an entry.
        if (m_hotIndex < 0)
          parent()->parent()->layout();
      }

      if (hasCapture())
        releaseMouse();
      break;
    }

    case ui::kMouseMoveMessage: {
      ui::MouseMessage* mouseMsg = static_cast<ui::MouseMessage*>(msg);
      gfx::Point mousePos = mouseMsg->position() - bounds().origin();
      gfx::Rect bounds = clientBounds();
      int hot = -1;

      bounds.shrink(3*ui::guiscale());

      if (bounds.contains(mousePos)) {
        int count = std::max(1, size());
        int boxWidth = std::max(1, bounds.w / count);
        hot = (mousePos.x - bounds.x) / boxWidth;
        hot = base::clamp(hot, 0, count-1);
      }

      if (m_hotIndex != hot) {
        m_hotIndex = hot;
        invalidate();
      }

      bool dropBefore =
        (hot >= 0 && mousePos.x < (bounds.x+m_boxSize*ui::guiscale()*hot)+m_boxSize*ui::guiscale()/2);
      if (m_dropBefore != dropBefore) {
        m_dropBefore = dropBefore;
        invalidate();
      }
      break;
    }
  }
  return Widget::onProcessMessage(msg);
}

void ColorShades::onSizeHint(ui::SizeHintEvent& ev)
{
  int size = this->size();
  if (size < 2)
    ev.setSizeHint(gfx::Size((16+m_boxSize)*ui::guiscale()+textWidth(), 18*ui::guiscale()));
  else {
    if (m_click == ClickWholeShade && size > 16)
      size = 16;
    ev.setSizeHint(gfx::Size(6+m_boxSize*size, 18)*ui::guiscale());
  }
}

void ColorShades::onPaint(ui::PaintEvent& ev)
{
  auto theme = skin::SkinTheme::instance();
  ui::Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();

  theme->paintWidget(g, this, style(), bounds);

  bounds.shrink(3*ui::guiscale());

  Shade colors = getShade();
  if (colors.size() >= m_minColors) {
    gfx::Rect box(bounds.x, bounds.y,
                  bounds.w / std::max(1, int(colors.size())),
                  bounds.h);
    gfx::Rect hotBounds;

    int j = 0;
    for (int i=0; box.x<bounds.x2(); ++i, box.x += box.w) {
      // Make the last box a little bigger to just use all
      // available size
      if (i == int(colors.size())-1)
        box.w = bounds.x2()-box.x;

      app::Color color;

      if (m_dragIndex >= 0 &&
          m_hotIndex == i) {
        color = colors[m_dragIndex];
      }
      else {
        if (j == m_dragIndex) {
          ++j;
        }
        if (j < int(colors.size()))
          color = colors[j++];
        else
          color = app::Color::fromMask();
      }

      draw_color(g, box, color,
                 (doc::ColorMode)app_get_current_pixel_format());

      if (m_hotIndex == i)
        hotBounds = box;
    }

    if (!hotBounds.isEmpty() &&
        isHotEntryVisible()) {
      hotBounds.enlarge(3*ui::guiscale());

      ui::PaintWidgetPartInfo info;
      theme->paintWidgetPart(
        g, theme->styles.shadeSelection(), hotBounds, info);
    }
  }
  else {
    g->fillRect(theme->colors.editorFace(), bounds);
    g->drawAlignedUIText(text(), theme->colors.face(), gfx::ColorNone, bounds,
                         ui::CENTER | ui::MIDDLE);
  }
}

} // namespace app
