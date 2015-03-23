// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_PALETTE_VIEW_H_INCLUDED
#define APP_UI_PALETTE_VIEW_H_INCLUDED
#pragma once

#include "base/connection.h"
#include "ui/event.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

#include <vector>

namespace doc {
  class Palette;
  class Remap;
}

namespace app {

  class PaletteViewDelegate {
  public:
    virtual ~PaletteViewDelegate() { }
    virtual void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) { }
    virtual void onPaletteViewRemapColors(const doc::Remap& remap, const doc::Palette* newPalette) { }
    virtual void onPaletteViewChangeSize(int boxsize) { }
  };

  class PaletteView : public ui::Widget {
  public:
    typedef std::vector<bool> SelectedEntries;

    PaletteView(bool editable, PaletteViewDelegate* delegate, int boxsize);

    int getColumns() const { return m_columns; }
    void setColumns(int columns);

    void clearSelection();
    void selectColor(int index);
    void selectRange(int index1, int index2);

    int getSelectedEntry() const;
    bool getSelectedRange(int& index1, int& index2) const;
    void getSelectedEntries(SelectedEntries& entries) const;

    app::Color getColorByPosition(const gfx::Point& pos);

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;

  private:

    enum class State {
      WAITING,
      SELECTING_COLOR,
      DRAGGING_OUTLINE,
    };

    struct Hit {
      enum Part {
        NONE,
        COLOR,
        OUTLINE
      };
      Part part;
      int color;
      bool after;

      Hit(Part part, int color = -1) : part(part), color(color), after(false) {
      }

      bool operator==(const Hit& hit) const {
        return (
          part == hit.part &&
          color == hit.color &&
          after == hit.after);
      }
      bool operator!=(const Hit& hit) const {
        return !operator==(hit);
      }
    };

    void request_size(int* w, int* h);
    void update_scroll(int color);
    void onAppPaletteChange();
    gfx::Rect getPaletteEntryBounds(int index);
    Hit hitTest(const gfx::Point& pos);
    void dropColors(int beforeIndex);

    State m_state;
    bool m_editable;
    PaletteViewDelegate* m_delegate;
    int m_columns;
    int m_boxsize;
    int m_currentEntry;
    int m_rangeAnchor;
    SelectedEntries m_selectedEntries;
    bool m_isUpdatingColumns;
    ScopedConnection m_conn;
    Hit m_hot;
  };

  ui::WidgetType palette_view_type();

} // namespace app

#endif
