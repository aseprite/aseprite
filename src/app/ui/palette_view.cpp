// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/ui/editor/editor.h"
#include "app/ui/palette_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/util/clipboard.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "os/font.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace app {

using namespace ui;
using namespace app::skin;

PaletteView::PaletteView(bool editable, PaletteViewStyle style, PaletteViewDelegate* delegate, int boxsize)
  : Widget(kGenericWidget)
  , m_state(State::WAITING)
  , m_editable(editable)
  , m_style(style)
  , m_delegate(delegate)
  , m_columns(16)
  , m_boxsize(boxsize)
  , m_currentEntry(-1)
  , m_rangeAnchor(-1)
  , m_isUpdatingColumns(false)
  , m_hot(Hit::NONE)
  , m_copy(false)
{
  setFocusStop(true);
  setDoubleBuffered(true);

  m_palConn = App::instance()->PaletteChange.connect(&PaletteView::onAppPaletteChange, this);
  m_csConn = App::instance()->ColorSpaceChange.connect(
    base::Bind<void>(&PaletteView::invalidate, this));

  InitTheme.connect(
    [this]{
      setBorder(gfx::Border(1 * guiscale()));
      setChildSpacing(1 * guiscale());
    });
  initTheme();
}

void PaletteView::setColumns(int columns)
{
  int old_columns = m_columns;
  m_columns = columns;

  if (m_columns != old_columns) {
    View* view = View::getView(this);
    if (view)
      view->updateView();

    invalidate();
  }
}

void PaletteView::deselect()
{
  bool invalidate = (m_selectedEntries.picks() > 0);

  m_selectedEntries.resize(currentPalette()->size());
  m_selectedEntries.clear();

  if (invalidate)
    this->invalidate();
}

void PaletteView::selectColor(int index)
{
  if (index < 0 || index >= currentPalette()->size())
    return;

  if (m_currentEntry != index || !m_selectedEntries[index]) {
    m_currentEntry = index;
    m_rangeAnchor = index;

    update_scroll(m_currentEntry);
    invalidate();
  }
}

void PaletteView::selectExactMatchColor(const app::Color& color)
{
  int index = findExactIndex(color);
  if (index >= 0)
    selectColor(index);
}

void PaletteView::selectRange(int index1, int index2)
{
  m_rangeAnchor = index1;
  m_currentEntry = index2;

  std::fill(m_selectedEntries.begin()+std::min(index1, index2),
            m_selectedEntries.begin()+std::max(index1, index2)+1, true);

  update_scroll(index2);
  invalidate();
}

int PaletteView::getSelectedEntry() const
{
  return m_currentEntry;
}

bool PaletteView::getSelectedRange(int& index1, int& index2) const
{
  int i, i2, j;

  // Find the first selected entry
  for (i=0; i<(int)m_selectedEntries.size(); ++i)
    if (m_selectedEntries[i])
      break;

  // Find the first unselected entry after i
  for (i2=i+1; i2<(int)m_selectedEntries.size(); ++i2)
    if (!m_selectedEntries[i2])
      break;

  // Find the last selected entry
  for (j=m_selectedEntries.size()-1; j>=0; --j)
    if (m_selectedEntries[j])
      break;

  if (j-i+1 == i2-i) {
    index1 = i;
    index2 = j;
    return true;
  }
  else
    return false;
}

void PaletteView::getSelectedEntries(PalettePicks& entries) const
{
  entries = m_selectedEntries;
}

int PaletteView::getSelectedEntriesCount() const
{
  return m_selectedEntries.picks();
}

app::Color PaletteView::getColorByPosition(const gfx::Point& pos)
{
  gfx::Point relPos = pos - bounds().origin();
  Palette* palette = currentPalette();
  for (int i=0; i<palette->size(); ++i) {
    if (getPaletteEntryBounds(i).contains(relPos))
      return app::Color::fromIndex(i);
  }
  return app::Color::fromMask();
}

int PaletteView::getBoxSize() const
{
  return int(m_boxsize);
}

void PaletteView::setBoxSize(double boxsize)
{
  m_boxsize = MID(4.0, boxsize, 32.0);

  if (m_delegate)
    m_delegate->onPaletteViewChangeSize(int(m_boxsize));

  View* view = View::getView(this);
  if (view)
    view->layout();
}

void PaletteView::clearSelection()
{
  if (!m_selectedEntries.picks())
    return;

  Palette palette(*currentPalette());
  Palette newPalette(palette);
  newPalette.resize(MAX(1, newPalette.size() - m_selectedEntries.picks()));

  Remap remap = create_remap_to_move_picks(m_selectedEntries, palette.size());
  for (int i=0; i<palette.size(); ++i) {
    if (!m_selectedEntries[i])
      newPalette.setEntry(remap[i], palette.getEntry(i));
  }

  m_currentEntry = m_selectedEntries.firstPick();
  m_selectedEntries.clear();
  stopMarchingAnts();

  setNewPalette(&palette, &newPalette, PaletteViewModification::CLEAR);
}

void PaletteView::cutToClipboard()
{
  if (!m_selectedEntries.picks())
    return;

  clipboard::copy_palette(currentPalette(), m_selectedEntries);

  clearSelection();
}

void PaletteView::copyToClipboard()
{
  if (!m_selectedEntries.picks())
    return;

  clipboard::copy_palette(currentPalette(), m_selectedEntries);

  startMarchingAnts();
  invalidate();
}

void PaletteView::pasteFromClipboard()
{
  if (clipboard::get_current_format() == clipboard::ClipboardPaletteEntries) {
    if (m_delegate)
      m_delegate->onPaletteViewPasteColors(
        clipboard::get_palette(),
        clipboard::get_palette_picks(),
        m_selectedEntries);

    // We just hide the marching ants, the user can paste multiple
    // times.
    stopMarchingAnts();
    invalidate();
  }
}

void PaletteView::discardClipboardSelection()
{
  if (isMarchingAntsRunning()) {
    stopMarchingAnts();
    invalidate();
  }
}

bool PaletteView::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
      FocusOrClick(msg);
      break;

    case kKeyDownMessage:
    case kKeyUpMessage:
    case kMouseEnterMessage:
      if (hasMouse())
        updateCopyFlag(msg);
      break;

    case kMouseDownMessage:
      switch (m_hot.part) {

        case Hit::COLOR:
          m_state = State::SELECTING_COLOR;

          // As we can ctrl+click color bar + timeline, now we have to
          // re-prioritize the color bar on each click.
          FocusOrClick(msg);
          break;

        case Hit::OUTLINE:
          m_state = State::DRAGGING_OUTLINE;
          break;

        case Hit::RESIZE_HANDLE:
          m_state = State::RESIZING_PALETTE;
          break;
      }

      captureMouse();

      // Continue...

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (m_state == State::SELECTING_COLOR &&
          m_hot.part == Hit::COLOR) {
        int idx = m_hot.color;
        idx = MID(0, idx, currentPalette()->size()-1);

        MouseButtons buttons = mouseMsg->buttons();

        if (hasCapture() && ((idx != m_currentEntry) ||
                             (msg->type() == kMouseDownMessage) ||
                             ((buttons & kButtonMiddle) == kButtonMiddle))) {
          if ((buttons & kButtonMiddle) == 0) {
            if (!msg->ctrlPressed() && !msg->shiftPressed())
              deselect();

            if (msg->type() == kMouseMoveMessage)
              selectRange(m_rangeAnchor, idx);
            else {
              selectColor(idx);
              m_selectedEntries[idx] = true;
            }
          }

          // Emit signal
          if (m_delegate)
            m_delegate->onPaletteViewIndexChange(idx, buttons);
        }
      }

      if (m_state == State::DRAGGING_OUTLINE &&
          m_hot.part == Hit::COLOR) {
        update_scroll(m_hot.color);
      }

      if (hasCapture())
        return true;

      break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        switch (m_state) {

          case State::DRAGGING_OUTLINE:
            if (m_hot.part == Hit::COLOR ||
                m_hot.part == Hit::POSSIBLE_COLOR) {
              int i = m_hot.color;
              if (!m_copy && i > m_selectedEntries.firstPick())
                i += m_selectedEntries.picks();
              dropColors(i);
            }
            break;

          case State::RESIZING_PALETTE:
            if (m_hot.part == Hit::COLOR ||
                m_hot.part == Hit::POSSIBLE_COLOR) {
              int newPalSize = MAX(1, m_hot.color);
              Palette newPalette(*currentPalette());
              newPalette.resize(newPalSize);
              setNewPalette(currentPalette(), &newPalette,
                            PaletteViewModification::RESIZE);
            }
            break;
        }

        m_state = State::WAITING;
        setStatusBar();
        invalidate();
      }
      return true;

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (!view)
        break;

      gfx::Point delta = static_cast<MouseMessage*>(msg)->wheelDelta();

      if (msg->onlyCtrlPressed() ||
          msg->onlyCmdPressed()) {
        int z = delta.x - delta.y;
        setBoxSize(m_boxsize + z);
      }
      else {
        gfx::Point scroll = view->viewScroll();

        if (static_cast<MouseMessage*>(msg)->preciseWheel())
          scroll += delta;
        else
          scroll += delta * 3 * int(m_boxsize*guiscale());

        view->setViewScroll(scroll);
      }
      break;
    }

    case kMouseLeaveMessage:
      m_hot = Hit(Hit::NONE);
      setStatusBar();
      invalidate();
      break;

    case kSetCursorMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      Hit hit = hitTest(mouseMsg->position() - bounds().origin());
      if (hit != m_hot) {
        // Redraw only when we put the mouse in other part of the
        // widget (e.g. if we move from color to color, we don't want
        // to redraw the whole widget if we're on WAITING state).
        if ((m_state == State::WAITING && hit.part != m_hot.part) ||
            (m_state != State::WAITING && hit != m_hot)) {
          invalidate();
        }
        m_hot = hit;
        setStatusBar();
      }
      setCursor();
      return true;
    }

    case kTouchMagnifyMessage: {
      setBoxSize(m_boxsize + m_boxsize * static_cast<ui::TouchMessage*>(msg)->magnification());
      break;
    }

  }

  return Widget::onProcessMessage(msg);
}

void PaletteView::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  int outlineWidth = theme->dimensions.paletteOutlineWidth();
  ui::Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();
  Palette* palette = currentPalette();
  int fgIndex = -1;
  int bgIndex = -1;
  int transparentIndex = -1;
  bool hotColor = (m_hot.part == Hit::COLOR ||
                   m_hot.part == Hit::POSSIBLE_COLOR);
  bool dragging = (m_state == State::DRAGGING_OUTLINE && hotColor);
  bool resizing = (m_state == State::RESIZING_PALETTE && hotColor);

  if (m_style == FgBgColors && m_delegate) {
    fgIndex = findExactIndex(m_delegate->onPaletteViewGetForegroundIndex());
    bgIndex = findExactIndex(m_delegate->onPaletteViewGetBackgroundIndex());

    if (current_editor && current_editor->sprite()->pixelFormat() == IMAGE_INDEXED)
      transparentIndex = current_editor->sprite()->transparentColor();
  }

  g->fillRect(theme->colors.editorFace(), bounds);

  // Draw palette entries
  int picksCount = m_selectedEntries.picks();
  int idxOffset = 0;
  int boxOffset = 0;
  int palSize = palette->size();
  if (dragging && !m_copy) palSize -= picksCount;
  if (resizing) palSize = m_hot.color;

  for (int i=0; i<palSize; ++i) {
    if (dragging) {
      if (!m_copy) {
        while (i+idxOffset < m_selectedEntries.size() &&
               m_selectedEntries[i+idxOffset])
          ++idxOffset;
      }
      if (!boxOffset && m_hot.color == i) {
        boxOffset += picksCount;
      }
    }

    gfx::Rect box = getPaletteEntryBounds(i + boxOffset);
    gfx::Color gfxColor = drawEntry(g, box, i + idxOffset);
    const int boxsize = int(m_boxsize * guiscale());

    switch (m_style) {

      case SelectOneColor:
        if (m_currentEntry == i)
          g->fillRect(color_utils::blackandwhite_neg(gfxColor),
                      gfx::Rect(box.center(), gfx::Size(guiscale(), guiscale())));
        break;

      case FgBgColors:
        if (fgIndex == i) {
          gfx::Color neg = color_utils::blackandwhite_neg(gfxColor);
          for (int i=0; i<int(boxsize/2); ++i)
            g->drawHLine(neg, box.x, box.y+i, int(boxsize/2)-i);
        }

        if (bgIndex == i) {
          gfx::Color neg = color_utils::blackandwhite_neg(gfxColor);
          for (int i=0; i<int(boxsize/4); ++i)
            g->drawHLine(neg, box.x+box.w-(i+1), box.y+box.h-int(boxsize/4)+i, i+1);
        }

        if (transparentIndex == i)
          g->fillRect(color_utils::blackandwhite_neg(gfxColor),
                      gfx::Rect(box.center(), gfx::Size(guiscale(), guiscale())));
        break;
    }
  }

  // Handle to resize palette

  if (m_editable && !dragging) {
    os::Surface* handle = theme->parts.palResize()->bitmap(0);
    gfx::Rect box = getPaletteEntryBounds(palSize);
    g->drawRgbaSurface(handle,
                       box.x+box.w/2-handle->width()/2,
                       box.y+box.h/2-handle->height()/2);
  }

  // Draw selected entries

  PaintWidgetPartInfo info;
  if (m_hot.part == Hit::OUTLINE)
    info.styleFlags |= ui::Style::Layer::kMouse;

  PalettePicks dragPicks;
  int j = 0;
  if (dragging) {
    dragPicks.resize(m_hot.color+picksCount);
    std::fill(dragPicks.begin()+m_hot.color, dragPicks.end(), true);
  }
  PalettePicks& picks = (dragging ? dragPicks: m_selectedEntries);

  for (int i=0; i<palette->size(); ++i) {
    if (!m_selectedEntries[i])
      continue;

    int k = (dragging ? m_hot.color+j: i);

    gfx::Rect box, clipR;
    getEntryBoundsAndClip(k, picks, box, clipR, outlineWidth);

    IntersectClip clip(g, clipR);
    if (clip) {
      // Draw color being dragged + label
      if (dragging) {
        gfx::Rect box2 = getPaletteEntryBounds(k);
        gfx::Color gfxColor = drawEntry(g, box2, i); // Draw color entry

        gfx::Color neg = color_utils::blackandwhite_neg(gfxColor);
        os::Font* minifont = theme->getMiniFont();
        std::string text = base::convert_to<std::string>(k);
        g->setFont(minifont);
        g->drawText(text, neg, gfx::ColorNone,
                    gfx::Point(box2.x + box2.w/2 - minifont->textLength(text)/2,
                               box2.y + box2.h/2 - minifont->height()/2));
      }

      // Draw the selection
      theme->paintWidgetPart(
        g, theme->styles.colorbarSelection(), box, info);
    }

    ++j;
  }

  // Draw marching ants
  if ((m_state == State::WAITING) &&
      (isMarchingAntsRunning()) &&
      (clipboard::get_current_format() == clipboard::ClipboardPaletteEntries)) {
    Palette* clipboardPalette = clipboard::get_palette();
    const PalettePicks& clipboardPicks = clipboard::get_palette_picks();

    if (clipboardPalette &&
        clipboardPalette->countDiff(palette, nullptr, nullptr) == 0) {
      for (int i=0; i<clipboardPicks.size(); ++i) {
        if (!clipboardPicks[i])
          continue;

        gfx::Rect box, clipR;
        getEntryBoundsAndClip(i, clipboardPicks, box, clipR, outlineWidth);

        IntersectClip clip(g, clipR);
        if (clip) {
          CheckedDrawMode checked(g, getMarchingAntsOffset(),
                                  gfx::rgba(0, 0, 0, 255),
                                  gfx::rgba(255, 255, 255, 255));
          g->drawRect(gfx::rgba(0, 0, 0), box);
        }
      }
    }
  }
}

void PaletteView::onResize(ui::ResizeEvent& ev)
{
  if (!m_isUpdatingColumns) {
    m_isUpdatingColumns = true;
    View* view = View::getView(this);
    if (view) {
      int columns =
        (view->viewportBounds().w-this->childSpacing()*2)
        / (int(m_boxsize*guiscale())+this->childSpacing());
      setColumns(MAX(1, columns));
    }
    m_isUpdatingColumns = false;
  }

  Widget::onResize(ev);
}

void PaletteView::onSizeHint(ui::SizeHintEvent& ev)
{
  div_t d = div(currentPalette()->size(), m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem)? 1: 0);

  if (m_editable) {
    ++rows;
  }

  const int boxsize = int(m_boxsize * guiscale());
  ev.setSizeHint(
    gfx::Size(
      border().width() + cols*boxsize + (cols-1)*childSpacing(),
      border().height() + rows*boxsize + (rows-1)*childSpacing()));
}

void PaletteView::onDrawMarchingAnts()
{
  invalidate();
}

void PaletteView::update_scroll(int color)
{
  View* view = View::getView(this);
  if (!view)
    return;

  gfx::Rect vp = view->viewportBounds();
  gfx::Point scroll;
  int x, y, cols;
  div_t d;
  const int boxsize = int(m_boxsize * guiscale());

  scroll = view->viewScroll();

  d = div(currentPalette()->size(), m_columns);
  cols = m_columns;

  y = (boxsize+childSpacing()) * (color / cols);
  x = (boxsize+childSpacing()) * (color % cols);

  if (scroll.x > x)
    scroll.x = x;
  else if (scroll.x+vp.w-boxsize-2 < x)
    scroll.x = x-vp.w+boxsize+2;

  if (scroll.y > y)
    scroll.y = y;
  else if (scroll.y+vp.h-boxsize-2 < y)
    scroll.y = y-vp.h+boxsize+2;

  view->setViewScroll(scroll);
}

void PaletteView::onAppPaletteChange()
{
  m_selectedEntries.resize(currentPalette()->size());

  View* view = View::getView(this);
  if (view)
    view->layout();
}

gfx::Rect PaletteView::getPaletteEntryBounds(int index) const
{
  gfx::Rect bounds = clientBounds();
  int cols = m_columns;
  int col = index % cols;
  int row = index / cols;
  int boxsize = int(m_boxsize * guiscale());

  return gfx::Rect(
    bounds.x + border().left() + col*(boxsize+childSpacing()),
    bounds.y + border().top() + row*(boxsize+childSpacing()),
    boxsize, boxsize);
}

PaletteView::Hit PaletteView::hitTest(const gfx::Point& pos)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  int outlineWidth = theme->dimensions.paletteOutlineWidth();
  Palette* palette = currentPalette();

  if (m_state == State::WAITING && m_editable) {
    // First check if the mouse is inside the selection outline.
    for (int i=0; i<palette->size(); ++i) {
      if (!m_selectedEntries[i])
        continue;

      const int max = palette->size();
      bool top    = (i >= m_columns            && i-m_columns >= 0  ? m_selectedEntries[i-m_columns]: false);
      bool bottom = (i < max-m_columns         && i+m_columns < max ? m_selectedEntries[i+m_columns]: false);
      bool left   = ((i%m_columns)>0           && i-1         >= 0  ? m_selectedEntries[i-1]: false);
      bool right  = ((i%m_columns)<m_columns-1 && i+1         < max ? m_selectedEntries[i+1]: false);

      gfx::Rect box = getPaletteEntryBounds(i);
      box.enlarge(outlineWidth);

      if ((!top    && gfx::Rect(box.x, box.y, box.w, outlineWidth).contains(pos)) ||
          (!bottom && gfx::Rect(box.x, box.y+box.h-outlineWidth, box.w, outlineWidth).contains(pos)) ||
          (!left   && gfx::Rect(box.x, box.y, outlineWidth, box.h).contains(pos)) ||
          (!right  && gfx::Rect(box.x+box.w-outlineWidth, box.y, outlineWidth, box.h).contains(pos)))
        return Hit(Hit::OUTLINE, i);
    }

    // Check if we are in the resize handle
    if (getPaletteEntryBounds(palette->size()).contains(pos)) {
      return Hit(Hit::RESIZE_HANDLE, palette->size());
    }
  }

  // Check if we are inside a color.
  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  for (int i=0; ; ++i) {
    gfx::Rect box = getPaletteEntryBounds(i);
    if (i >= palette->size() && box.y2() > vp.h)
      break;

    box.w += childSpacing();
    box.h += childSpacing();
    if (box.contains(pos))
      return Hit(Hit::COLOR, i);
  }

  gfx::Rect box = getPaletteEntryBounds(0);
  int boxsize = int(m_boxsize * guiscale());
  box.w = (boxsize+childSpacing());
  box.h = (boxsize+childSpacing());

  int colsLimit = m_columns;
  if (m_state == State::DRAGGING_OUTLINE)
    --colsLimit;
  int i = MID(0, (pos.x-vp.x)/box.w, colsLimit) + MAX(0, pos.y/box.h)*m_columns;
  return Hit(Hit::POSSIBLE_COLOR, i);
}

void PaletteView::dropColors(int beforeIndex)
{
  Palette palette(*currentPalette());
  if (beforeIndex >= palette.size()) {
    palette.resize(beforeIndex);
    m_selectedEntries.resize(palette.size());
  }

  Palette newPalette(palette);
  Remap remap(palette.size());

  // Copy colors
  if (m_copy) {
    int picks = m_selectedEntries.picks();
    ASSERT(picks >= 1);

    remap = create_remap_to_expand_palette(palette.size()+picks,
                                           picks,
                                           beforeIndex);

    newPalette.resize(palette.size()+picks);
    for (int i=0; i<palette.size(); ++i)
      newPalette.setEntry(remap[i], palette.getEntry(i));

    for (int i=0, j=0; i<palette.size(); ++i) {
      if (m_selectedEntries[i])
        newPalette.setEntry(beforeIndex + (j++), palette.getEntry(i));
    }

    for (int i=0, j=0; i<palette.size(); ++i) {
      if (m_selectedEntries[i]) {
        if (m_currentEntry == i) {
          m_currentEntry = beforeIndex + j;
          break;
        }
        ++j;
      }
    }

    for (int i=0; i<palette.size(); ++i)
      m_selectedEntries[i] = (i >= beforeIndex && i < beforeIndex + picks);
  }
  // Move colors
  else {
    remap = create_remap_to_move_picks(m_selectedEntries, beforeIndex);

    auto oldSelectedCopies = m_selectedEntries;
    for (int i=0; i<palette.size(); ++i) {
      newPalette.setEntry(remap[i], palette.getEntry(i));
      m_selectedEntries[remap[i]] = oldSelectedCopies[i];
    }

    m_currentEntry = remap[m_currentEntry];
  }

  setNewPalette(&palette, &newPalette,
                PaletteViewModification::DRAGANDDROP);
}

void PaletteView::getEntryBoundsAndClip(int i, const PalettePicks& entries,
                                        gfx::Rect& box, gfx::Rect& clip,
                                        int outlineWidth) const
{
  box = clip = getPaletteEntryBounds(i);
  box.enlarge(outlineWidth);

  // Left
  if (!pickedXY(entries, i, -1, 0)) {
    clip.x -= outlineWidth;
    clip.w += outlineWidth;
  }

  // Top
  if (!pickedXY(entries, i, 0, -1)) {
    clip.y -= outlineWidth;
    clip.h += outlineWidth;
  }

  // Right
  if (!pickedXY(entries, i, +1, 0))
    clip.w += outlineWidth;
  else {
    clip.w += guiscale();
    box.w += outlineWidth;
  }

  // Bottom
  if (!pickedXY(entries, i, 0, +1))
    clip.h += outlineWidth;
  else {
    clip.h += guiscale();
    box.h += outlineWidth;
  }
}

bool PaletteView::pickedXY(const doc::PalettePicks& entries, int i, int dx, int dy) const
{
  int x = (i % m_columns) + dx;
  int y = (i / m_columns) + dy;
  int lastcolor = entries.size()-1;

  if (x < 0 || x >= m_columns || y < 0 || y > lastcolor/m_columns)
    return false;

  i = x + y*m_columns;
  if (i >= 0 && i < entries.size())
    return entries[i];
  else
    return false;
}

void PaletteView::updateCopyFlag(ui::Message* msg)
{
  bool oldCopy = m_copy;
  m_copy = (msg->ctrlPressed() || msg->altPressed());
  if (oldCopy != m_copy) {
    setCursor();
    setStatusBar();
    invalidate();
  }
}

void PaletteView::setCursor()
{
  if (m_state == State::DRAGGING_OUTLINE ||
      (m_state == State::WAITING &&
       m_hot.part == Hit::OUTLINE)) {
    if (m_copy)
      ui::set_mouse_cursor(kArrowPlusCursor);
    else
      ui::set_mouse_cursor(kMoveCursor);
  }
  else if (m_state == State::RESIZING_PALETTE ||
           (m_state == State::WAITING &&
            m_hot.part == Hit::RESIZE_HANDLE)) {
    ui::set_mouse_cursor(kSizeWECursor);
  }
  else
    ui::set_mouse_cursor(kArrowCursor);
}

void PaletteView::setStatusBar()
{
  StatusBar* statusBar = StatusBar::instance();

  if (m_hot.part == Hit::NONE) {
    statusBar->showDefaultText();
    return;
  }

  switch (m_state) {

    case State::WAITING:
    case State::SELECTING_COLOR:
      if ((m_hot.part == Hit::COLOR ||
           m_hot.part == Hit::OUTLINE ||
           m_hot.part == Hit::POSSIBLE_COLOR) &&
          (m_hot.color < currentPalette()->size())) {
        int i = MAX(0, m_hot.color);

        statusBar->showColor(
          0, "", app::Color::fromIndex(i));
      }
      else {
        statusBar->showDefaultText();
      }
      break;

    case State::DRAGGING_OUTLINE:
      if (m_hot.part == Hit::COLOR) {
        const int picks = m_selectedEntries.picks();
        const int destIndex = MAX(0, m_hot.color);
        const int palSize = currentPalette()->size();
        const int newPalSize =
          (m_copy ? MAX(palSize + picks, destIndex + picks):
                    MAX(palSize,         destIndex + picks));

        statusBar->setStatusText(
          0, "%s to %d - New Palette Size %d",
          (m_copy ? "Copy": "Move"),
          destIndex, newPalSize);
      }
      else {
        statusBar->showDefaultText();
      }
      break;

    case State::RESIZING_PALETTE:
      if (m_hot.part == Hit::COLOR ||
          m_hot.part == Hit::POSSIBLE_COLOR ||
          m_hot.part == Hit::RESIZE_HANDLE) {
        int newPalSize = MAX(1, m_hot.color);
        statusBar->setStatusText(
          0, "New Palette Size %d",
          newPalSize);
      }
      else {
        statusBar->showDefaultText();
      }
      break;
  }
}

doc::Palette* PaletteView::currentPalette() const
{
  return get_current_palette();
}

int PaletteView::findExactIndex(const app::Color& color) const
{
  switch (color.getType()) {

    case Color::MaskType:
      return (current_editor ? current_editor->sprite()->transparentColor(): -1);

    case Color::RgbType:
    case Color::HsvType:
    case Color::HslType:
    case Color::GrayType:
      return currentPalette()->findExactMatch(
        color.getRed(), color.getGreen(), color.getBlue(), color.getAlpha(), -1);

    case Color::IndexType:
      return color.getIndex();
  }

  ASSERT(false);
  return -1;
}

void PaletteView::setNewPalette(doc::Palette* oldPalette,
                                doc::Palette* newPalette,
                                PaletteViewModification mod)
{
  // No differences
  if (!newPalette->countDiff(oldPalette, nullptr, nullptr))
    return;

  if (m_delegate) {
    m_delegate->onPaletteViewModification(newPalette, mod);
    if (m_currentEntry >= 0)
      m_delegate->onPaletteViewIndexChange(m_currentEntry, ui::kButtonLeft);
  }

  set_current_palette(newPalette, false);
  manager()->invalidate();
}

gfx::Color PaletteView::drawEntry(ui::Graphics* g, const gfx::Rect& box, int palIdx)
{
  doc::color_t palColor =
    (palIdx < currentPalette()->size() ? currentPalette()->getEntry(palIdx):
                                         rgba(0, 0, 0, 255));
  app::Color appColor = app::Color::fromRgb(
    rgba_getr(palColor),
    rgba_getg(palColor),
    rgba_getb(palColor),
    rgba_geta(palColor));
  gfx::Color gfxColor = gfx::rgba(
    rgba_getr(palColor),
    rgba_getg(palColor),
    rgba_getb(palColor),
    rgba_geta(palColor));

  gfx::Rect box2(box);
  box2.enlarge(1);
  for (int i=1; i<=guiscale(); ++i, box2.enlarge(1))
    g->drawRect(gfx::rgba(0, 0, 0), box2);
  draw_color(g, box, appColor, doc::ColorMode::RGB);
  return gfxColor;
}

} // namespace app
