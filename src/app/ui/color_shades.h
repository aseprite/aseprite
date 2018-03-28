// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SHADES_H_INCLUDED
#define APP_UI_COLOR_SHADES_H_INCLUDED
#pragma once

#include "app/shade.h"
#include "obs/signal.h"
#include "ui/widget.h"

namespace doc {
  class Remap;
}

namespace app {

  class ColorShades : public ui::Widget {
  public:
    enum ClickType {
      ClickEntries,
      DragAndDropEntries,
      ClickWholeShade
    };

    ColorShades(const Shade& colors, ClickType click);

    void reverseShadeColors();
    doc::Remap* createShadeRemap(bool left);
    int size() const;

    Shade getShade() const;
    void setShade(const Shade& shade);

    void updateShadeFromColorBarPicks();

    int getHotEntry() const { return m_hotIndex; }

    obs::signal<void()> Click;

  private:
    void onInitTheme(ui::InitThemeEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onChangeColorBarSelection();
    bool isHotEntryVisible() const {
      return m_click != ClickWholeShade;
    }

    ClickType m_click;
    Shade m_shade;
    int m_hotIndex;
    int m_dragIndex;
    bool m_dropBefore;
    int m_boxSize;
    obs::scoped_connection m_conn;
  };

} // namespace app

#endif
