// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_SHADES_H_INCLUDED
#define APP_UI_COLOR_SHADES_H_INCLUDED
#pragma once

#include "app/shade.h"
#include "obs/signal.h"
#include "ui/mouse_button.h"
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

    ClickType clickType() const { return m_click; }

    void setMinColors(int minColors);
    void reverseShadeColors();
    doc::Remap* createShadeRemap(bool left);
    int size() const { return int(m_shade.size()); }

    const Shade& getShade() const { return m_shade; }
    void setShade(const Shade& shade);

    int getHotEntry() const { return m_hotIndex; }

    class ClickEvent {
    public:
      ClickEvent(ui::MouseButton button) : m_button(button) { }
      ui::MouseButton button() const { return m_button; }
    private:
      ui::MouseButton m_button;
    };

    obs::signal<void(ClickEvent&)> Click;

  private:
    void onInitTheme(ui::InitThemeEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    bool isHotEntryVisible() const {
      return m_click != ClickWholeShade;
    }

    ClickType m_click;
    Shade m_shade;
    int m_minColors;
    int m_hotIndex;
    int m_dragIndex;
    bool m_dropBefore;
    int m_boxSize;
  };

} // namespace app

#endif
