// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_SKIN_SKIN_THEME_H_INCLUDED
#define APP_UI_SKIN_SKIN_THEME_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "app/ui/skin/skin_parts.h"
#include "app/ui/skin/style_sheet.h"
#include "gfx/color.h"
#include "gfx/fwd.h"
#include "ui/manager.h"
#include "ui/theme.h"

#include <map>
#include <string>

namespace ui {
  class Entry;
  class Graphics;
  class IButtonIcon;
}

namespace she {
  class Surface;
}

namespace app {
  namespace skin {

    namespace ThemeColor {
      enum Type {
        Text,
        Disabled,
        Face,
        HotFace,
        Selected,
        Background,
        TextBoxText,
        TextBoxFace,
        EntrySuffix,
        LinkText,
        ButtonNormalText,
        ButtonNormalFace,
        ButtonHotText,
        ButtonHotFace,
        ButtonSelectedText,
        ButtonSelectedFace,
        CheckHotFace,
        CheckFocusFace,
        RadioHotFace,
        RadioFocusFace,
        MenuItemNormalText,
        MenuItemNormalFace,
        MenuItemHotText,
        MenuItemHotFace,
        MenuItemHighlightText,
        MenuItemHighlightFace,
        EditorFace,
        EditorSpriteBorder,
        EditorSpriteBottomBorder,
        ListItemNormalText,
        ListItemNormalFace,
        ListItemSelectedText,
        ListItemSelectedFace,
        SliderEmptyText,
        SliderEmptyFace,
        SliderFullText,
        SliderFullFace,
        TabNormalText,
        TabNormalFace,
        TabSelectedText,
        TabSelectedFace,
        SplitterNormalFace,
        ScrollBarBgFace,
        ScrollBarThumbFace,
        PopupWindowBorder,
        TooltipText,
        TooltipFace,
        FileListEvenRowText,
        FileListEvenRowFace,
        FileListOddRowText,
        FileListOddRowFace,
        FileListSelectedRowText,
        FileListSelectedRowFace,
        FileListDisabledRowText,
        Workspace,
        MaxColors
      };
    }

    extern const char* kWindowFaceColorId;

    // This is the GUI theme used by Aseprite (which use images from
    // data/skins directory).
    class SkinTheme : public ui::Theme {
    public:
      static const char* kThemeCloseButtonId;

      SkinTheme();
      ~SkinTheme();

      gfx::Color getColor(ThemeColor::Type k) const {
        return m_colors[k];
      }

      she::Font* getMiniFont() const { return m_minifont; }

      void reload_skin();
      void reload_fonts();

      ui::Cursor* getCursor(ui::CursorType type);
      void initWidget(ui::Widget* widget);
      void getWindowMask(ui::Widget* widget, gfx::Region& region);
      void setDecorativeWidgetBounds(ui::Widget* widget);

      void paintDesktop(ui::PaintEvent& ev);
      void paintBox(ui::PaintEvent& ev);
      void paintButton(ui::PaintEvent& ev);
      void paintCheckBox(ui::PaintEvent& ev);
      void paintEntry(ui::PaintEvent& ev);
      void paintGrid(ui::PaintEvent& ev);
      void paintLabel(ui::PaintEvent& ev);
      void paintLinkLabel(ui::PaintEvent& ev);
      void paintListBox(ui::PaintEvent& ev);
      void paintListItem(ui::PaintEvent& ev);
      void paintMenu(ui::PaintEvent& ev);
      void paintMenuItem(ui::PaintEvent& ev);
      void paintSplitter(ui::PaintEvent& ev);
      void paintRadioButton(ui::PaintEvent& ev);
      void paintSeparator(ui::PaintEvent& ev);
      void paintSlider(ui::PaintEvent& ev);
      void paintComboBoxEntry(ui::PaintEvent& ev);
      void paintComboBoxButton(ui::PaintEvent& ev);
      void paintTextBox(ui::PaintEvent& ev);
      void paintView(ui::PaintEvent& ev);
      void paintViewScrollbar(ui::PaintEvent& ev);
      void paintViewViewport(ui::PaintEvent& ev);
      void paintWindow(ui::PaintEvent& ev);
      void paintPopupWindow(ui::PaintEvent& ev);
      void paintWindowButton(ui::PaintEvent& ev);
      void paintTooltip(ui::PaintEvent& ev);

      int get_button_selected_offset() const { return 0; } // TODO Configurable in xml

      she::Surface* get_part(int part_i) const { return m_part[part_i]; }
      she::Surface* get_part(const std::string& id) const;
      she::Surface* get_toolicon(const char* tool_id) const;
      gfx::Size get_part_size(int part_i) const;

      // Helper functions to draw bounds/hlines with sheet parts
      void draw_bounds_array(ui::Graphics* g, const gfx::Rect& rc, int parts[8]);
      void draw_bounds_nw(ui::Graphics* g, const gfx::Rect& rc, int nw, gfx::Color bg = gfx::ColorNone);
      void draw_bounds_nw(ui::Graphics* g, const gfx::Rect& rc, const SkinPartPtr skinPart, gfx::Color bg = gfx::ColorNone);
      void draw_bounds_nw2(ui::Graphics* g, const gfx::Rect& rc, int x_mid, int nw1, int nw2, gfx::Color bg1, gfx::Color bg2);
      void draw_part_as_hline(ui::Graphics* g, const gfx::Rect& rc, int part);
      void draw_part_as_vline(ui::Graphics* g, const gfx::Rect& rc, int part);
      void paintProgressBar(ui::Graphics* g, const gfx::Rect& rc, float progress);

      Style* getStyle(const std::string& id) {
        return m_stylesheet.getStyle(id);
      }

      SkinPartPtr getPartById(const std::string& id) {
        return m_parts_by_id[id];
      }

      gfx::Color getColorById(const std::string& id) {
        return m_colors_by_id[id];
      }

    protected:
      void onRegenerate() override;

    private:
      void draw_bounds_template(ui::Graphics* g, const gfx::Rect& rc,
                                int nw, int n, int ne, int e, int se, int s, int sw, int w);
      void draw_bounds_template(ui::Graphics* g, const gfx::Rect& rc, const SkinPartPtr& skinPart);
      void draw_bounds_template(ui::Graphics* g, const gfx::Rect& rc,
        she::Surface* nw, she::Surface* n, she::Surface* ne,
        she::Surface* e, she::Surface* se, she::Surface* s,
        she::Surface* sw, she::Surface* w);

      she::Surface* sliceSheet(she::Surface* sur, const gfx::Rect& bounds);
      gfx::Color getWidgetBgColor(ui::Widget* widget);
      void drawTextString(ui::Graphics* g, const char *t, gfx::Color fg_color, gfx::Color bg_color,
                          ui::Widget* widget, const gfx::Rect& rc,
                          int selected_offset);
      void drawEntryCaret(ui::Graphics* g, ui::Entry* widget, int x, int y);

      void paintIcon(ui::Widget* widget, ui::Graphics* g, ui::IButtonIcon* iconInterface, int x, int y);

      she::Font* loadFont(const char* userFont, const std::string& path);

      std::string m_selected_skin;
      she::Surface* m_sheet;
      std::vector<she::Surface*> m_part;
      std::map<std::string, SkinPartPtr> m_parts_by_id;
      std::map<std::string, she::Surface*> m_toolicon;
      std::map<std::string, gfx::Color> m_colors_by_id;
      std::vector<ui::Cursor*> m_cursors;
      std::vector<gfx::Color> m_colors;
      StyleSheet m_stylesheet;
      she::Font* m_minifont;
    };

    inline Style* get_style(const std::string& id) {
      return static_cast<SkinTheme*>(ui::Manager::getDefault()->getTheme())->getStyle(id);
    }

    inline SkinPartPtr get_part_by_id(const std::string& id) {
      return static_cast<SkinTheme*>(ui::Manager::getDefault()->getTheme())->getPartById(id);
    }

    inline gfx::Color get_color_by_id(const std::string& id) {
      return static_cast<SkinTheme*>(ui::Manager::getDefault()->getTheme())->getColorById(id);
    }

  } // namespace skin
} // namespace app

#endif
