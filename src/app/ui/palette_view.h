// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PALETTE_VIEW_H_INCLUDED
#define APP_UI_PALETTE_VIEW_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_source.h"
#include "app/ui/marching_ants.h"
#include "doc/palette_picks.h"
#include "obs/connection.h"
#include "obs/signal.h"
#include "ui/event.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

#include <vector>

namespace doc {
  class Palette;
}

namespace app {

  enum class PaletteViewModification {
    CLEAR,
    DRAGANDDROP,
    RESIZE,
  };

  class PaletteViewDelegate {
  public:
    virtual ~PaletteViewDelegate() { }
    virtual void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) { }
    virtual void onPaletteViewModification(const doc::Palette* newPalette, PaletteViewModification mod) { }
    virtual void onPaletteViewChangeSize(int boxsize) { }
    virtual void onPaletteViewPasteColors(
      const doc::Palette* fromPal, const doc::PalettePicks& from, const doc::PalettePicks& to) { }
    virtual app::Color onPaletteViewGetForegroundIndex() { return app::Color::fromMask(); }
    virtual app::Color onPaletteViewGetBackgroundIndex() { return app::Color::fromMask(); }
  };

  class PaletteView : public ui::Widget
                    , public MarchingAnts
                    , public IColorSource {
  public:
    enum PaletteViewStyle {
      SelectOneColor,
      FgBgColors
    };

    PaletteView(bool editable, PaletteViewStyle style, PaletteViewDelegate* delegate, int boxsize);

    bool isEditable() const { return m_editable; }

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

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

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

      Hit(Part part, int color = -1) : part(part), color(color) {
      }

      bool operator==(const Hit& hit) const {
        return (
          part == hit.part &&
          color == hit.color);
      }
      bool operator!=(const Hit& hit) const {
        return !operator==(hit);
      }
    };

    void update_scroll(int color);
    void onAppPaletteChange();
    gfx::Rect getPaletteEntryBounds(int index) const;
    Hit hitTest(const gfx::Point& pos);
    void dropColors(int beforeIndex);
    void getEntryBoundsAndClip(int i, const doc::PalettePicks& entries,
                               gfx::Rect& box, gfx::Rect& clip,
                               int outlineWidth) const;
    bool pickedXY(const doc::PalettePicks& entries, int i, int dx, int dy) const;
    void updateCopyFlag(ui::Message* msg);
    void setCursor();
    void setStatusBar();
    doc::Palette* currentPalette() const;
    int findExactIndex(const app::Color& color) const;
    void setNewPalette(doc::Palette* oldPalette, doc::Palette* newPalette,
                       PaletteViewModification mod);
    gfx::Color drawEntry(ui::Graphics* g, const gfx::Rect& box, int palIdx);

    State m_state;
    bool m_editable;
    PaletteViewStyle m_style;
    PaletteViewDelegate* m_delegate;
    int m_columns;
    double m_boxsize;
    int m_currentEntry;
    int m_rangeAnchor;
    doc::PalettePicks m_selectedEntries;
    bool m_isUpdatingColumns;
    obs::scoped_connection m_palConn;
    obs::scoped_connection m_csConn;
    Hit m_hot;
    bool m_copy;
  };

} // namespace app

#endif
