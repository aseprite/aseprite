// Aseprite
// Copyright (C) 2018-2021  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PALETTE_VIEW_H_INCLUDED
#define APP_UI_PALETTE_VIEW_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/context_observer.h"
#include "app/ui/color_source.h"
#include "app/ui/marching_ants.h"
#include "app/ui/tile_source.h"
#include "doc/palette_picks.h"
#include "doc/tile.h"
#include "obs/connection.h"
#include "obs/signal.h"
#include "ui/event.h"
#include "ui/mouse_button.h"
#include "ui/widget.h"

#include <memory>
#include <vector>

namespace doc {
class Palette;
class Tileset;
} // namespace doc

namespace app {

enum class PaletteViewModification {
  CLEAR,
  DRAGANDDROP,
  RESIZE,
};

class PaletteView;

class PaletteViewDelegate {
public:
  virtual ~PaletteViewDelegate() {}
  virtual bool onIsPaletteViewActive(PaletteView* paletteView) const { return false; }
  virtual void onPaletteViewIndexChange(int index, ui::MouseButton button) {}
  virtual void onPaletteViewModification(const doc::Palette* newPalette,
                                         PaletteViewModification mod)
  {
  }
  virtual void onPaletteViewChangeSize(PaletteView* paletteView, int boxsize) {}
  virtual void onPaletteViewPasteColors(const doc::Palette* fromPal,
                                        const doc::PalettePicks& from,
                                        const doc::PalettePicks& to)
  {
  }
  virtual app::Color onPaletteViewGetForegroundIndex() { return app::Color::fromMask(); }
  virtual app::Color onPaletteViewGetBackgroundIndex() { return app::Color::fromMask(); }
  virtual doc::tile_index onPaletteViewGetForegroundTile() { return -1; }
  virtual doc::tile_index onPaletteViewGetBackgroundTile() { return -1; }
  virtual void onTilesViewClearTiles(const doc::PalettePicks& tiles) {}
  virtual void onTilesViewResize(const int newSize) {}
  virtual void onTilesViewDragAndDrop(doc::Tileset* tileset,
                                      doc::PalettePicks& picks,
                                      int& currentEntry,
                                      const int beforeIndex,
                                      const bool isCopy)
  {
  }
  virtual void onTilesViewIndexChange(int index, ui::MouseButton button) {}
};

class AbstractPaletteViewAdapter;
class PaletteViewAdapter;
class TilesetViewAdapter;

class PaletteView : public ui::Widget,
                    public MarchingAnts,
                    public IColorSource,
                    public ITileSource,
                    public ContextObserver {
  friend class PaletteViewAdapter;
  friend class TilesetViewAdapter;

public:
  enum PaletteViewStyle {
    SelectOneColor,
    FgBgColors,
    FgBgTiles,
  };

  PaletteView(bool editable, PaletteViewStyle style, PaletteViewDelegate* delegate, int boxsize);
  ~PaletteView();

  PaletteViewDelegate* delegate() { return m_delegate; }

  bool isEditable() const { return m_editable; }
  bool isPalette() const { return m_style != FgBgTiles; }
  bool isTiles() const { return m_style == FgBgTiles; }

  int getColumns() const { return m_columns; }
  void setColumns(int columns);

  void deselect();
  void selectColor(int index);
  void selectExactMatchColor(const app::Color& color);
  void selectRange(int index1, int index2);

  int getSelectedEntry() const;
  bool getSelectedRange(int& index1, int& index2) const;
  void getSelectedEntries(doc::PalettePicks& entries) const;
  int getSelectedEntriesCount() const;
  void setSelectedEntries(const doc::PalettePicks& entries);

  doc::Tileset* tileset() const;

  // IColorSource
  app::Color getColorByPosition(const gfx::Point& pos) override;

  // ITileSource
  doc::tile_t getTileByPosition(const gfx::Point& pos) override;

  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override;

  int getBoxSize() const;
  void setBoxSize(double boxsize);

  void clearSelection();
  void cutToClipboard();
  void copyToClipboard();
  void pasteFromClipboard();
  void discardClipboardSelection();

  obs::signal<void(ui::Message*)> FocusOrClick;

protected:
  bool onProcessMessage(ui::Message* msg) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onDrawMarchingAnts() override;

private:
  enum class State {
    WAITING,
    SELECTING_COLOR,
    DRAGGING_OUTLINE,
    RESIZING_PALETTE,
  };

  struct Hit {
    enum Part {
      NONE,
      COLOR,
      OUTLINE,
      RESIZE_HANDLE,
      POSSIBLE_COLOR,
    };
    Part part;
    int color;

    Hit(Part part, int color = -1) : part(part), color(color) {}

    bool operator==(const Hit& hit) const { return (part == hit.part && color == hit.color); }
    bool operator!=(const Hit& hit) const { return !operator==(hit); }
  };

  void update_scroll(int color);
  void onAppPaletteChange();
  gfx::Rect getPaletteEntryBounds(int index) const;
  Hit hitTest(const gfx::Point& pos);
  void dropColors(int beforeIndex);
  void getEntryBoundsAndClip(int i,
                             const doc::PalettePicks& entries,
                             const int outlineWidth,
                             gfx::Rect& box,
                             gfx::Rect& clip) const;
  bool pickedXY(const doc::PalettePicks& entries, int i, int dx, int dy) const;
  void updateCopyFlag(ui::Message* msg);
  void setCursor();
  void setStatusBar();
  doc::Palette* currentPalette() const;
  int findExactIndex(const app::Color& color) const;
  void setNewPalette(doc::Palette* oldPalette,
                     doc::Palette* newPalette,
                     PaletteViewModification mod);
  int boxSizePx() const;
  void updateBorderAndChildSpacing();

  State m_state;
  bool m_editable;
  PaletteViewStyle m_style;
  PaletteViewDelegate* m_delegate;
  std::unique_ptr<AbstractPaletteViewAdapter> m_adapter;
  int m_columns;
  double m_boxsize;
  int m_currentEntry;
  int m_rangeAnchor;
  doc::PalettePicks m_selectedEntries;
  bool m_isUpdatingColumns;
  obs::scoped_connection m_palConn;
  obs::scoped_connection m_csConn;
  obs::scoped_connection m_sepConn;
  Hit m_hot;
  bool m_copy;
  bool m_withSeparator;
};

} // namespace app

#endif
