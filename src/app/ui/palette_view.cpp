// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define PAL_TRACE(...) // TRACEARGS

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/site.h"
#include "app/ui/editor/editor.h"
#include "app/ui/palette_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/conversion_to_surface.h"
#include "app/util/pal_ops.h"
#include "base/convert_to.h"
#include "doc/image.h"
#include "doc/layer_tilemap.h"
#include "doc/palette.h"
#include "doc/remap.h"
#include "doc/tileset.h"
#include "fmt/format.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "os/font.h"
#include "os/surface.h"
#include "os/system.h"
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
#include <set>

namespace app {

using namespace ui;
using namespace app::skin;

// Interface used to adapt the PaletteView widget to see tilesets too.
class AbstractPaletteViewAdapter {
public:
  virtual ~AbstractPaletteViewAdapter() {}
  virtual int size() const = 0;
  virtual void paletteChange(doc::PalettePicks& picks) = 0;
  virtual void activeSiteChange(const Site& site, doc::PalettePicks& picks) = 0;
  virtual void clearSelection(PaletteView* paletteView, doc::PalettePicks& picks) = 0;
  virtual void selectIndex(PaletteView* paletteView, int index, ui::MouseButton button) = 0;
  virtual void resizePalette(PaletteView* paletteView, int newSize) = 0;
  virtual void dropColors(PaletteView* paletteView,
                          doc::PalettePicks& picks,
                          int& currentEntry,
                          const int beforeIndex,
                          const bool isCopy) = 0;
  virtual void showEntryInStatusBar(StatusBar* statusBar, int index) = 0;
  virtual void showDragInfoInStatusBar(StatusBar* statusBar,
                                       bool copy,
                                       int destIndex,
                                       int newSize) = 0;
  virtual void showResizeInfoInStatusBar(StatusBar* statusBar, int newSize) = 0;
  virtual void drawEntry(ui::Graphics* g,
                         SkinTheme* theme,
                         const int palIdx,
                         const int offIdx,
                         const int childSpacing,
                         gfx::Rect& box,
                         gfx::Color& negColor) = 0;
  virtual doc::Tileset* tileset() const { return nullptr; }
};

// This default adapter uses the default behavior to use the
// PaletteView as just a doc::Palette view.
class PaletteViewAdapter : public AbstractPaletteViewAdapter {
public:
  int size() const override { return palette()->size(); }
  void paletteChange(doc::PalettePicks& picks) override { picks.resize(palette()->size()); }
  void activeSiteChange(const Site& site, doc::PalettePicks& picks) override
  {
    // Do nothing
  }
  void clearSelection(PaletteView* paletteView, doc::PalettePicks& picks) override
  {
    Palette palette(*this->palette());
    Palette newPalette(palette);
    newPalette.resize(std::max(1, newPalette.size() - picks.picks()));

    Remap remap = create_remap_to_move_picks(picks, palette.size());
    for (int i = 0; i < palette.size(); ++i) {
      if (!picks[i])
        newPalette.setEntry(remap[i], palette.getEntry(i));
    }

    paletteView->setNewPalette(&palette, &newPalette, PaletteViewModification::CLEAR);
  }
  void selectIndex(PaletteView* paletteView, int index, ui::MouseButton button) override
  {
    // Emit signal
    if (paletteView->delegate())
      paletteView->delegate()->onPaletteViewIndexChange(index, button);
  }
  void resizePalette(PaletteView* paletteView, int newSize) override
  {
    Palette newPalette(*paletteView->currentPalette());
    newPalette.resize(newSize);
    paletteView->setNewPalette(paletteView->currentPalette(),
                               &newPalette,
                               PaletteViewModification::RESIZE);
  }
  void dropColors(PaletteView* paletteView,
                  doc::PalettePicks& picks,
                  int& currentEntry,
                  const int beforeIndex,
                  const bool isCopy) override
  {
    Palette palette(*paletteView->currentPalette());
    Palette newPalette(palette);
    move_or_copy_palette_colors(palette, newPalette, picks, currentEntry, beforeIndex, isCopy);
    paletteView->setNewPalette(&palette, &newPalette, PaletteViewModification::DRAGANDDROP);
  }
  void showEntryInStatusBar(StatusBar* statusBar, int index) override
  {
    statusBar->showColor(0, app::Color::fromIndex(index));
  }
  void showDragInfoInStatusBar(StatusBar* statusBar, bool copy, int destIndex, int newSize) override
  {
    statusBar->setStatusText(
      0,
      fmt::format("{} to {} - New Palette Size {}", (copy ? "Copy" : "Move"), destIndex, newSize));
  }
  void showResizeInfoInStatusBar(StatusBar* statusBar, int newSize) override
  {
    statusBar->setStatusText(0, fmt::format("New Palette Size {}", newSize));
  }
  void drawEntry(ui::Graphics* g,
                 SkinTheme* theme,
                 const int palIdx,
                 const int offIdx,
                 const int childSpacing,
                 gfx::Rect& box,
                 gfx::Color& negColor) override
  {
    doc::color_t palColor = (palIdx < palette()->size() ? palette()->getEntry(palIdx) :
                                                          rgba(0, 0, 0, 255));
    app::Color appColor = app::Color::fromRgb(rgba_getr(palColor),
                                              rgba_getg(palColor),
                                              rgba_getb(palColor),
                                              rgba_geta(palColor));

    if (childSpacing > 0) {
      gfx::Color color = theme->colors.paletteEntriesSeparator();
      g->fillRect(color, gfx::Rect(box).enlarge(childSpacing));
    }
    draw_color(g, box, appColor, doc::ColorMode::RGB);

    const gfx::Color gfxColor =
      gfx::rgba(rgba_getr(palColor), rgba_getg(palColor), rgba_getb(palColor), rgba_geta(palColor));
    negColor = color_utils::blackandwhite_neg(gfxColor);
  }

private:
  doc::Palette* palette() const { return get_current_palette(); }
};

// This adapter makes it possible to use a PaletteView to edit a
// doc::Tileset.
class TilesetViewAdapter : public AbstractPaletteViewAdapter {
public:
  int size() const override
  {
    if (auto t = tileset())
      return t->size();
    else
      return 0;
  }
  void paletteChange(doc::PalettePicks& picks) override
  {
    // Do nothing
  }
  void activeSiteChange(const Site& site, doc::PalettePicks& picks) override
  {
    if (auto tileset = this->tileset())
      picks.resize(tileset->size());
    else
      picks.clear();
  }
  void clearSelection(PaletteView* paletteView, doc::PalettePicks& picks) override
  {
    // Cannot delete the empty tile (index 0)
    int i = picks.firstPick();
    if (i == doc::notile) {
      picks[i] = false;
      if (!picks.picks()) {
        // Cannot remove empty tile
        StatusBar::instance()->showTip(1000, "Cannot delete the empty tile");
        return;
      }
    }

    paletteView->delegate()->onTilesViewClearTiles(picks);
  }
  void selectIndex(PaletteView* paletteView, int index, ui::MouseButton button) override
  {
    // Emit signal
    if (paletteView->delegate())
      paletteView->delegate()->onTilesViewIndexChange(index, button);
  }
  void resizePalette(PaletteView* paletteView, int newSize) override
  {
    paletteView->delegate()->onTilesViewResize(newSize);
  }
  void dropColors(PaletteView* paletteView,
                  doc::PalettePicks& picks,
                  int& currentEntry,
                  const int _beforeIndex,
                  const bool isCopy) override
  {
    PAL_TRACE("dropColors");

    doc::Tileset* tileset = this->tileset();
    ASSERT(tileset);
    if (!tileset)
      return;

    // Important: we create a copy because if we make the tileset
    // bigger dropping tiles outside the tileset range, the tileset
    // will be made bigger (see cmd::AddTile() inside
    // move_tiles_in_tileset() function), a
    // Doc::notifyTilesetChanged() will be generated, a
    // ColorBar::onTilesetChanged() called, and finally we'll receive a
    // PaletteView::deselect() that will clear the whole picks.
    auto newPicks = picks;
    int beforeIndex = _beforeIndex;

    // We cannot move the empty tile (index 0) no any place
    if (beforeIndex == 0)
      ++beforeIndex;
    if (!isCopy && newPicks.size() > 0 && newPicks[0])
      newPicks[0] = false;
    if (!newPicks.picks()) {
      // Cannot move empty tile
      StatusBar::instance()->showTip(1000, "Cannot move the empty tile");
      return;
    }

    paletteView->delegate()->onTilesViewDragAndDrop(tileset,
                                                    newPicks,
                                                    currentEntry,
                                                    beforeIndex,
                                                    isCopy);

    // Copy the new picks
    picks = newPicks;
  }
  void showEntryInStatusBar(StatusBar* statusBar, int index) override
  {
    statusBar->showTile(0, doc::tile(index, 0));
  }
  void showDragInfoInStatusBar(StatusBar* statusBar, bool copy, int destIndex, int newSize) override
  {
    statusBar->setStatusText(
      0,
      fmt::format("{} to {} - New Tileset Size {}", (copy ? "Copy" : "Move"), destIndex, newSize));
  }
  void showResizeInfoInStatusBar(StatusBar* statusBar, int newSize) override
  {
    statusBar->setStatusText(0, fmt::format("New Tileset Size {}", newSize));
  }
  void drawEntry(ui::Graphics* g,
                 SkinTheme* theme,
                 const int palIdx,
                 const int offIdx,
                 const int childSpacing,
                 gfx::Rect& box,
                 gfx::Color& negColor) override
  {
    if (childSpacing > 0) {
      gfx::Color color = theme->colors.paletteEntriesSeparator();
      g->fillRect(color, gfx::Rect(box).enlarge(childSpacing));
    }
    draw_color(g, box, app::Color::fromMask(), doc::ColorMode::RGB);

    doc::ImageRef tileImage;
    if (auto t = this->tileset())
      tileImage = t->get(palIdx);
    if (tileImage) {
      int w = tileImage->width();
      int h = tileImage->height();
      os::SurfaceRef surface = os::instance()->makeRgbaSurface(w, h);
      convert_image_to_surface(tileImage.get(),
                               get_current_palette(),
                               surface.get(),
                               0,
                               0,
                               0,
                               0,
                               w,
                               h);

      ui::Paint paint;
      paint.blendMode(os::BlendMode::SrcOver);

      os::Sampling sampling;
      if (w > box.w && h > box.h) {
        sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Nearest);
      }

      g->drawSurface(surface.get(), gfx::Rect(0, 0, w, h), box, sampling, &paint);
    }
    negColor = gfx::rgba(255, 255, 255);
  }
  doc::Tileset* tileset() const override
  {
    Site site = App::instance()->context()->activeSite();
    if (site.layer() && site.layer()->isTilemap()) {
      return static_cast<LayerTilemap*>(site.layer())->tileset();
    }
    else
      return nullptr;
  }

private:
};

PaletteView::PaletteView(bool editable,
                         PaletteViewStyle style,
                         PaletteViewDelegate* delegate,
                         int boxsize)
  : Widget(kGenericWidget)
  , m_state(State::WAITING)
  , m_editable(editable)
  , m_style(style)
  , m_delegate(delegate)
  , m_adapter(isTiles() ? (AbstractPaletteViewAdapter*)new TilesetViewAdapter :
                          (AbstractPaletteViewAdapter*)new PaletteViewAdapter)
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
  m_csConn = App::instance()->ColorSpaceChange.connect([this] { invalidate(); });

  {
    auto& entriesSep = Preferences::instance().colorBar.entriesSeparator;
    m_withSeparator = entriesSep();
    m_sepConn = entriesSep.AfterChange.connect([this, &entriesSep](bool newValue) {
      m_withSeparator = entriesSep();
      updateBorderAndChildSpacing();
    });
  }

  if (isTiles())
    UIContext::instance()->add_observer(this);

  InitTheme.connect([this] { updateBorderAndChildSpacing(); });
  initTheme();
}

PaletteView::~PaletteView()
{
  if (isTiles())
    UIContext::instance()->remove_observer(this);
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

  m_selectedEntries.resize(m_adapter->size());
  m_selectedEntries.clear();

  if (invalidate)
    this->invalidate();
}

void PaletteView::selectColor(int index)
{
  if (index < 0 || index >= m_adapter->size())
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

  std::fill(m_selectedEntries.begin() + std::min(index1, index2),
            m_selectedEntries.begin() + std::max(index1, index2) + 1,
            true);

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
  for (i = 0; i < (int)m_selectedEntries.size(); ++i)
    if (m_selectedEntries[i])
      break;

  // Find the first unselected entry after i
  for (i2 = i + 1; i2 < (int)m_selectedEntries.size(); ++i2)
    if (!m_selectedEntries[i2])
      break;

  // Find the last selected entry
  for (j = m_selectedEntries.size() - 1; j >= 0; --j)
    if (m_selectedEntries[j])
      break;

  if (j - i + 1 == i2 - i) {
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

void PaletteView::setSelectedEntries(const doc::PalettePicks& entries)
{
  m_selectedEntries = entries;
  m_selectedEntries.resize(m_adapter->size());
  m_currentEntry = m_selectedEntries.firstPick();

  if (entries.picks() > 0)
    requestFocus();

  invalidate();
}

doc::Tileset* PaletteView::tileset() const
{
  return m_adapter->tileset();
}

app::Color PaletteView::getColorByPosition(const gfx::Point& pos)
{
  gfx::Point relPos = pos - bounds().origin();
  for (int i = 0; i < m_adapter->size(); ++i) {
    auto box = getPaletteEntryBounds(i);
    box.inflate(childSpacing());
    if (box.contains(relPos))
      return app::Color::fromIndex(i);
  }
  return app::Color::fromMask();
}

doc::tile_t PaletteView::getTileByPosition(const gfx::Point& pos)
{
  gfx::Point relPos = pos - bounds().origin();
  for (int i = 0; i < m_adapter->size(); ++i) {
    auto box = getPaletteEntryBounds(i);
    box.inflate(childSpacing());
    if (box.contains(relPos))
      return doc::tile(i, 0);
  }
  return doc::notile;
}

void PaletteView::onActiveSiteChange(const Site& site)
{
  ASSERT(isTiles());
  m_adapter->activeSiteChange(site, m_selectedEntries);
}

int PaletteView::getBoxSize() const
{
  return int(m_boxsize);
}

void PaletteView::setBoxSize(double boxsize)
{
  if (isTiles())
    m_boxsize = std::clamp(boxsize, 4.0, 64.0);
  else
    m_boxsize = std::clamp(boxsize, 4.0, 32.0);

  if (m_delegate)
    m_delegate->onPaletteViewChangeSize(this, int(m_boxsize));

  View* view = View::getView(this);
  if (view)
    view->layout();
}

void PaletteView::clearSelection()
{
  if (!m_selectedEntries.picks())
    return;

  m_adapter->clearSelection(this, m_selectedEntries);

  m_currentEntry = m_selectedEntries.firstPick();
  m_selectedEntries.clear();
  stopMarchingAnts();
}

void PaletteView::cutToClipboard()
{
  if (!m_selectedEntries.picks())
    return;

  Clipboard::instance()->copyPalette(currentPalette(), m_selectedEntries);

  clearSelection();
}

void PaletteView::copyToClipboard()
{
  if (!m_selectedEntries.picks())
    return;

  Clipboard::instance()->copyPalette(currentPalette(), m_selectedEntries);

  startMarchingAnts();
  invalidate();
}

void PaletteView::pasteFromClipboard()
{
  auto clipboard = Clipboard::instance();
  if (clipboard->format() == ClipboardFormat::PaletteEntries) {
    if (m_delegate)
      m_delegate->onPaletteViewPasteColors(clipboard->getPalette(),
                                           clipboard->getPalettePicks(),
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
    case kFocusEnterMessage: FocusOrClick(msg); break;

    case kKeyDownMessage:
    case kKeyUpMessage:
    case kMouseEnterMessage:
      if (hasMouse())
        updateCopyFlag(msg);
      break;

    case kMouseDownMessage:
      switch (m_hot.part) {
        case Hit::COLOR:
        case Hit::POSSIBLE_COLOR:
          // Clicking outside the palette range will deselect
          if (m_hot.color >= m_adapter->size()) {
            deselect();
            break;
          }

          m_state = State::SELECTING_COLOR;

          // As we can ctrl+click color bar + timeline, now we have to
          // re-prioritize the color bar on each click.
          FocusOrClick(msg);
          break;

        case Hit::OUTLINE:       m_state = State::DRAGGING_OUTLINE; break;

        case Hit::RESIZE_HANDLE: m_state = State::RESIZING_PALETTE; break;
      }

      captureMouse();
      [[fallthrough]];

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if ((m_state == State::SELECTING_COLOR) &&
          (m_hot.part == Hit::COLOR || m_hot.part == Hit::POSSIBLE_COLOR)) {
        int idx = m_hot.color;
        idx = std::clamp(idx, 0, std::max(0, m_adapter->size() - 1));

        const MouseButton button = mouseMsg->button();

        if (hasCapture() && (idx >= 0 && idx < m_adapter->size()) &&
            ((idx != m_currentEntry) || (msg->type() == kMouseDownMessage) ||
             (button == kButtonMiddle))) {
          if (button != kButtonMiddle) {
            if (!msg->ctrlPressed() && !msg->shiftPressed())
              deselect();

            if (msg->type() == kMouseMoveMessage)
              selectRange(m_rangeAnchor, idx);
            else {
              selectColor(idx);
              m_selectedEntries[idx] = true;
            }
          }

          m_adapter->selectIndex(this, idx, button);
        }
      }

      if (m_state == State::DRAGGING_OUTLINE && m_hot.part == Hit::COLOR) {
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
            if (m_hot.part == Hit::COLOR || m_hot.part == Hit::POSSIBLE_COLOR) {
              int i = m_hot.color;
              if (!m_copy && i > m_selectedEntries.firstPick())
                i += m_selectedEntries.picks();
              dropColors(i);
            }
            break;

          case State::RESIZING_PALETTE:
            if (m_hot.part == Hit::COLOR || m_hot.part == Hit::POSSIBLE_COLOR) {
              int newSize = std::max(1, m_hot.color);
              m_adapter->resizePalette(this, newSize);
              m_selectedEntries.resize(newSize);
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

      if (msg->onlyCtrlPressed() || msg->onlyCmdPressed()) {
        int z = delta.x - delta.y;
        setBoxSize(m_boxsize + z);
      }
      else {
        gfx::Point scroll = view->viewScroll();

        if (static_cast<MouseMessage*>(msg)->preciseWheel())
          scroll += delta;
        else
          scroll += delta * 3 * boxSizePx();

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
  auto theme = SkinTheme::get(this);
  const int outlineWidth = theme->dimensions.paletteOutlineWidth();
  ui::Graphics* g = ev.graphics();
  gfx::Rect bounds = clientBounds();
  Palette* palette = currentPalette();
  int fgIndex = -1;
  int bgIndex = -1;
  int transparentIndex = -1;
  const bool hotColor = (m_hot.part == Hit::COLOR || m_hot.part == Hit::POSSIBLE_COLOR);
  const bool dragging = (m_state == State::DRAGGING_OUTLINE && hotColor);
  const bool resizing = (m_state == State::RESIZING_PALETTE && hotColor);

  if (m_delegate) {
    switch (m_style) {
      case FgBgColors: {
        fgIndex = findExactIndex(m_delegate->onPaletteViewGetForegroundIndex());
        bgIndex = findExactIndex(m_delegate->onPaletteViewGetBackgroundIndex());

        auto editor = Editor::activeEditor();
        if (editor && editor->sprite()->pixelFormat() == IMAGE_INDEXED)
          transparentIndex = editor->sprite()->transparentColor();
        break;
      }

      case FgBgTiles:
        fgIndex = m_delegate->onPaletteViewGetForegroundTile();
        bgIndex = m_delegate->onPaletteViewGetBackgroundTile();
        break;
    }
  }

  g->fillRect(theme->colors.editorFace(), bounds);

  // Draw palette/tileset entries
  int picksCount = m_selectedEntries.picks();
  int idxOffset = 0;
  int boxOffset = 0;
  int palSize = m_adapter->size();
  if (dragging && !m_copy)
    palSize -= picksCount;
  if (resizing)
    palSize = m_hot.color;

  for (int i = 0; i < palSize; ++i) {
    if (dragging) {
      if (!m_copy) {
        while (i + idxOffset < m_selectedEntries.size() && m_selectedEntries[i + idxOffset])
          ++idxOffset;
      }
      if (!boxOffset && m_hot.color == i) {
        boxOffset += picksCount;
      }
    }

    gfx::Rect box = getPaletteEntryBounds(i + boxOffset);
    gfx::Color negColor;
    m_adapter->drawEntry(g, theme, i + idxOffset, i + boxOffset, childSpacing(), box, negColor);
    const int boxsize = boxSizePx();
    const int scale = guiscale();

    switch (m_style) {
      case SelectOneColor:
        if (m_currentEntry == i)
          g->fillRect(negColor, gfx::Rect(box.center(), gfx::Size(scale, scale)));
        break;

      case FgBgColors:
      case FgBgTiles:
        if (!m_delegate || m_delegate->onIsPaletteViewActive(this)) {
          if (fgIndex == i) {
            for (int i = 0; i < int(boxsize / 2); i += scale) {
              g->fillRect(negColor, gfx::Rect(box.x, box.y + i, int(boxsize / 2) - i, scale));
            }
          }

          if (bgIndex == i) {
            for (int i = 0; i < int(boxsize / 4); i += scale) {
              g->fillRect(negColor,
                          gfx::Rect(box.x + box.w - (i + scale),
                                    box.y + box.h - int(boxsize / 4) + i,
                                    i + scale,
                                    scale));
            }
          }

          if (transparentIndex == i)
            g->fillRect(negColor, gfx::Rect(box.center(), gfx::Size(scale, scale)));
        }
        break;
    }
  }

  // Handle to resize palette

  if (m_editable && !dragging) {
    os::Surface* handle = theme->parts.palResize()->bitmap(0);
    gfx::Rect box = getPaletteEntryBounds(palSize);
    g->drawRgbaSurface(handle,
                       box.x + box.w / 2 - handle->width() / 2,
                       box.y + box.h / 2 - handle->height() / 2);
  }

  // Draw selected entries

  PaintWidgetPartInfo info;
  if (m_hot.part == Hit::OUTLINE)
    info.styleFlags |= ui::Style::Layer::kMouse;

  PalettePicks dragPicks;
  int j = 0;
  if (dragging) {
    dragPicks.resize(m_hot.color + picksCount);
    std::fill(dragPicks.begin() + m_hot.color, dragPicks.end(), true);
  }
  PalettePicks& picks = (dragging ? dragPicks : m_selectedEntries);

  const int size = m_adapter->size();
  for (int i = 0; i < size; ++i) {
    // TODO why does this fail?
    // ASSERT(i >= 0 && i < m_selectedEntries.size());
    if (i >= 0 && i < m_selectedEntries.size() && !m_selectedEntries[i]) {
      continue;
    }

    const int k = (dragging ? m_hot.color + j : i);

    gfx::Rect box, clipR;
    getEntryBoundsAndClip(k, picks, outlineWidth, box, clipR);

    IntersectClip clip(g, clipR);
    if (clip) {
      // Draw color being dragged + label
      if (dragging) {
        gfx::Rect box2 = getPaletteEntryBounds(k);
        gfx::Color negColor;
        m_adapter->drawEntry(g, theme, i, k, childSpacing(), box2, negColor);

        os::Font* minifont = theme->getMiniFont();
        const std::string text = base::convert_to<std::string>(k);
        g->setFont(AddRef(minifont));
        g->drawText(text,
                    negColor,
                    gfx::ColorNone,
                    gfx::Point(box2.x + box2.w / 2 - minifont->textLength(text) / 2,
                               box2.y + box2.h / 2 - minifont->height() / 2));
      }

      // Draw the selection
      theme->paintWidgetPart(g, theme->styles.colorbarSelection(), box, info);
    }

    ++j;
  }

  // Draw marching ants
  if ((m_state == State::WAITING) && (isMarchingAntsRunning()) &&
      (Clipboard::instance()->format() == ClipboardFormat::PaletteEntries)) {
    auto clipboard = Clipboard::instance();
    Palette* clipboardPalette = clipboard->getPalette();
    const PalettePicks& clipboardPicks = clipboard->getPalettePicks();

    if (clipboardPalette && clipboardPalette->countDiff(palette, nullptr, nullptr) == 0) {
      for (int i = 0; i < clipboardPicks.size(); ++i) {
        if (!clipboardPicks[i])
          continue;

        gfx::Rect box, clipR;
        getEntryBoundsAndClip(i, clipboardPicks, 1 * guiscale(), box, clipR);

        IntersectClip clip(g, clipR);
        if (clip) {
          ui::Paint paint;
          paint.style(ui::Paint::Stroke);
          ui::set_checkered_paint_mode(paint,
                                       getMarchingAntsOffset(),
                                       gfx::rgba(0, 0, 0, 255),
                                       gfx::rgba(255, 255, 255, 255));
          g->drawRect(box, paint);
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
      int columns = (view->viewportBounds().w - this->border().width() + this->childSpacing()) /
                    (boxSizePx() + this->childSpacing());
      setColumns(std::max(1, columns));
    }
    m_isUpdatingColumns = false;
  }

  Widget::onResize(ev);
}

void PaletteView::onSizeHint(ui::SizeHintEvent& ev)
{
  div_t d = div(m_adapter->size(), m_columns);
  int cols = m_columns;
  int rows = d.quot + ((d.rem) ? 1 : 0);

  if (m_editable) {
    ++rows;
  }

  const int boxsize = boxSizePx();
  ev.setSizeHint(gfx::Size(border().width() + cols * boxsize + (cols - 1) * childSpacing(),
                           border().height() + rows * boxsize + (rows - 1) * childSpacing()));
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
  const int boxsize = boxSizePx();

  scroll = view->viewScroll();

  d = div(m_adapter->size(), m_columns);
  cols = m_columns;

  y = (boxsize + childSpacing()) * (color / cols);
  x = (boxsize + childSpacing()) * (color % cols);

  if (scroll.x > x)
    scroll.x = x;
  else if (scroll.x + vp.w - boxsize - 2 < x)
    scroll.x = x - vp.w + boxsize + 2;

  if (scroll.y > y)
    scroll.y = y;
  else if (scroll.y + vp.h - boxsize - 2 < y)
    scroll.y = y - vp.h + boxsize + 2;

  view->setViewScroll(scroll);
}

void PaletteView::onAppPaletteChange()
{
  m_adapter->paletteChange(m_selectedEntries);

  View* view = View::getView(this);
  if (view)
    view->layout();
}

gfx::Rect PaletteView::getPaletteEntryBounds(int index) const
{
  const gfx::Rect bounds = clientChildrenBounds();
  const int col = index % m_columns;
  const int row = index / m_columns;
  const int boxsize = boxSizePx();
  const int spacing = childSpacing();

  return gfx::Rect(
    bounds.x + col * boxsize + (col - 1) * spacing + (m_withSeparator ? border().left() : 0),
    bounds.y + row * boxsize + (row - 1) * spacing + (m_withSeparator ? border().top() : 0),
    boxsize,
    boxsize);
}

PaletteView::Hit PaletteView::hitTest(const gfx::Point& pos)
{
  auto theme = SkinTheme::get(this);
  const int outlineWidth = theme->dimensions.paletteOutlineWidth();
  const int size = m_adapter->size();

  if (m_state == State::WAITING && m_editable) {
    // First check if the mouse is inside the selection outline.
    for (int i = 0; i < size; ++i) {
      if (!m_selectedEntries[i])
        continue;

      bool top = (i >= m_columns && i - m_columns >= 0 ? m_selectedEntries[i - m_columns] : false);
      bool bottom =
        (i < size - m_columns && i + m_columns < size ? m_selectedEntries[i + m_columns] : false);
      bool left = ((i % m_columns) > 0 && i - 1 >= 0 ? m_selectedEntries[i - 1] : false);
      bool right = ((i % m_columns) < m_columns - 1 && i + 1 < size ? m_selectedEntries[i + 1] :
                                                                      false);

      gfx::Rect box = getPaletteEntryBounds(i);
      box.enlarge(outlineWidth);

      if ((!top && gfx::Rect(box.x, box.y, box.w, outlineWidth).contains(pos)) ||
          (!bottom &&
           gfx::Rect(box.x, box.y + box.h - outlineWidth, box.w, outlineWidth).contains(pos)) ||
          (!left && gfx::Rect(box.x, box.y, outlineWidth, box.h).contains(pos)) ||
          (!right &&
           gfx::Rect(box.x + box.w - outlineWidth, box.y, outlineWidth, box.h).contains(pos)))
        return Hit(Hit::OUTLINE, i);
    }

    // Check if we are in the resize handle
    if (getPaletteEntryBounds(size).contains(pos)) {
      return Hit(Hit::RESIZE_HANDLE, size);
    }
  }

  // Check if we are inside a color.
  View* view = View::getView(this);
  ASSERT(view);
  gfx::Rect vp = view->viewportBounds();
  for (int i = 0;; ++i) {
    gfx::Rect box = getPaletteEntryBounds(i);
    if (i >= size && box.y2() > vp.h)
      break;

    box.w += childSpacing();
    box.h += childSpacing();
    if (box.contains(pos))
      return Hit(Hit::COLOR, i);
  }

  gfx::Rect box = getPaletteEntryBounds(0);

  int colsLimit = m_columns;
  if (m_state == State::DRAGGING_OUTLINE)
    --colsLimit;
  int i = std::clamp((pos.x - vp.x) / box.w, 0, colsLimit) + std::max(0, pos.y / box.h) * m_columns;
  return Hit(Hit::POSSIBLE_COLOR, i);
}

void PaletteView::dropColors(int beforeIndex)
{
  m_adapter->dropColors(this, m_selectedEntries, m_currentEntry, beforeIndex, m_copy);
}

void PaletteView::getEntryBoundsAndClip(int i,
                                        const PalettePicks& entries,
                                        const int outlineWidth,
                                        gfx::Rect& box,
                                        gfx::Rect& clip) const
{
  const int childSpacing = this->childSpacing();

  box = clip = getPaletteEntryBounds(i);
  box.enlarge(outlineWidth);

  gfx::Border boxBorder(0, 0, 0, 0);
  gfx::Border clipBorder(0, 0, 0, 0);

  // Left
  if (!pickedXY(entries, i, -1, 0))
    clipBorder.left(outlineWidth);
  else {
    boxBorder.left((childSpacing + 1) / 2);
    clipBorder.left((childSpacing + 1) / 2);
  }

  // Top
  if (!pickedXY(entries, i, 0, -1))
    clipBorder.top(outlineWidth);
  else {
    boxBorder.top((childSpacing + 1) / 2);
    clipBorder.top((childSpacing + 1) / 2);
  }

  // Right
  if (!pickedXY(entries, i, +1, 0))
    clipBorder.right(outlineWidth);
  else {
    boxBorder.right(childSpacing / 2);
    clipBorder.right(childSpacing / 2);
  }

  // Bottom
  if (!pickedXY(entries, i, 0, +1))
    clipBorder.bottom(outlineWidth);
  else {
    boxBorder.bottom(childSpacing / 2);
    clipBorder.bottom(childSpacing / 2);
  }

  box.enlarge(boxBorder);
  clip.enlarge(clipBorder);
}

bool PaletteView::pickedXY(const doc::PalettePicks& entries, int i, int dx, int dy) const
{
  const int x = (i % m_columns) + dx;
  const int y = (i / m_columns) + dy;
  const int lastcolor = entries.size() - 1;

  if (x < 0 || x >= m_columns || y < 0 || y > lastcolor / m_columns)
    return false;

  i = x + y * m_columns;
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
      (m_state == State::WAITING && m_hot.part == Hit::OUTLINE)) {
    if (m_copy)
      ui::set_mouse_cursor(kArrowPlusCursor);
    else
      ui::set_mouse_cursor(kMoveCursor);
  }
  else if (m_state == State::RESIZING_PALETTE ||
           (m_state == State::WAITING && m_hot.part == Hit::RESIZE_HANDLE)) {
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
      if ((m_hot.part == Hit::COLOR || m_hot.part == Hit::OUTLINE ||
           m_hot.part == Hit::POSSIBLE_COLOR) &&
          (m_hot.color < m_adapter->size())) {
        const int index = std::max(0, m_hot.color);

        m_adapter->showEntryInStatusBar(statusBar, index);
      }
      else {
        statusBar->showDefaultText();
      }
      break;

    case State::DRAGGING_OUTLINE:
      if (m_hot.part == Hit::COLOR) {
        const int picks = m_selectedEntries.picks();
        const int destIndex = std::max(0, m_hot.color);
        const int palSize = m_adapter->size();
        const int newPalSize = (m_copy ? std::max(palSize + picks, destIndex + picks) :
                                         std::max(palSize, destIndex + picks));

        m_adapter->showDragInfoInStatusBar(statusBar, m_copy, destIndex, newPalSize);
      }
      else {
        statusBar->showDefaultText();
      }
      break;

    case State::RESIZING_PALETTE:
      if (m_hot.part == Hit::COLOR || m_hot.part == Hit::POSSIBLE_COLOR ||
          m_hot.part == Hit::RESIZE_HANDLE) {
        const int newSize = std::max(1, m_hot.color);

        m_adapter->showResizeInfoInStatusBar(statusBar, newSize);
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
    case Color::MaskType: {
      auto editor = Editor::activeEditor();
      if (editor && editor->sprite()->pixelFormat() == IMAGE_INDEXED)
        return editor->sprite()->transparentColor();
      return currentPalette()->findMaskColor();
    }

    case Color::RgbType:
    case Color::HsvType:
    case Color::HslType:
    case Color::GrayType:
      return currentPalette()->findExactMatch(color.getRed(),
                                              color.getGreen(),
                                              color.getBlue(),
                                              color.getAlpha(),
                                              -1);

    case Color::IndexType: return color.getIndex();
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

int PaletteView::boxSizePx() const
{
  return m_boxsize * guiscale() + (m_withSeparator ? 0 : childSpacing());
}

void PaletteView::updateBorderAndChildSpacing()
{
  auto theme = SkinTheme::get(this);
  const int dim = theme->dimensions.paletteEntriesSeparator();
  setBorder(gfx::Border(dim));
  setChildSpacing(m_withSeparator ? dim : 0);

  View* view = View::getView(this);
  if (view)
    view->updateView();

  invalidate();
}

} // namespace app
