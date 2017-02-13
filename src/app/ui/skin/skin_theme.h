// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_THEME_H_INCLUDED
#define APP_UI_SKIN_SKIN_THEME_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "app/ui/skin/style_sheet.h"
#include "gfx/color.h"
#include "gfx/fwd.h"
#include "ui/manager.h"
#include "ui/theme.h"

#include "theme.xml.h"

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

    // This is the GUI theme used by Aseprite (which use images from
    // data/skins directory).
    class SkinTheme : public ui::Theme
                    , public app::gen::ThemeFile<SkinTheme> {
    public:
      static const char* kThemesFolderName;
      static const char* kThemeCloseButtonId;

      static SkinTheme* instance();

      SkinTheme();
      ~SkinTheme();

      she::Font* getDefaultFont() const override { return m_defaultFont; }
      she::Font* getWidgetFont(const ui::Widget* widget) const override;
      she::Font* getMiniFont() const { return m_miniFont; }

      ui::Cursor* getCursor(ui::CursorType type) override;
      void initWidget(ui::Widget* widget) override;
      void getWindowMask(ui::Widget* widget, gfx::Region& region) override;
      void setDecorativeWidgetBounds(ui::Widget* widget) override;
      int getScrollbarSize() override;
      gfx::Size getEntryCaretSize(ui::Widget* widget) override;

      void paintDesktop(ui::PaintEvent& ev) override;
      void paintBox(ui::PaintEvent& ev) override;
      void paintCheckBox(ui::PaintEvent& ev) override;
      void paintEntry(ui::PaintEvent& ev) override;
      void paintGrid(ui::PaintEvent& ev) override;
      void paintLabel(ui::PaintEvent& ev) override;
      void paintLinkLabel(ui::PaintEvent& ev) override;
      void paintListBox(ui::PaintEvent& ev) override;
      void paintListItem(ui::PaintEvent& ev) override;
      void paintMenu(ui::PaintEvent& ev) override;
      void paintMenuItem(ui::PaintEvent& ev) override;
      void paintSplitter(ui::PaintEvent& ev) override;
      void paintRadioButton(ui::PaintEvent& ev) override;
      void paintSeparator(ui::PaintEvent& ev) override;
      void paintSlider(ui::PaintEvent& ev) override;
      void paintComboBoxEntry(ui::PaintEvent& ev) override;
      void paintTextBox(ui::PaintEvent& ev) override;
      void paintView(ui::PaintEvent& ev) override;
      void paintViewScrollbar(ui::PaintEvent& ev) override;
      void paintViewViewport(ui::PaintEvent& ev) override;
      void paintWindow(ui::PaintEvent& ev) override;
      void paintPopupWindow(ui::PaintEvent& ev) override;
      void paintTooltip(ui::PaintEvent& ev) override;
      void paintWindowButton(ui::PaintEvent& ev);

      int get_button_selected_offset() const { return 0; } // TODO Configurable in xml

      she::Surface* getToolIcon(const char* toolId) const;

      // Helper functions to draw bounds/hlines with sheet parts
      void drawRect(ui::Graphics* g, const gfx::Rect& rc,
                    she::Surface* nw, she::Surface* n, she::Surface* ne,
                    she::Surface* e, she::Surface* se, she::Surface* s,
                    she::Surface* sw, she::Surface* w);
      void drawRect(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart, const bool drawCenter = true);
      void drawRect2(ui::Graphics* g, const gfx::Rect& rc, int x_mid, SkinPart* nw1, SkinPart* nw2);
      void drawHline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart);
      void drawVline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart);
      void paintProgressBar(ui::Graphics* g, const gfx::Rect& rc, double progress);

      Style* getStyle(const std::string& id) {
        return m_stylesheet.getStyle(id);
      }

      ui::Style* getNewStyle(const std::string& id) {
        return m_styles[id];
      }

      SkinPartPtr getPartById(const std::string& id) {
        return m_parts_by_id[id];
      }

      int getDimensionById(const std::string& id) {
        // Warning! Don't use ui::guiscale(), as CurrentTheme::get()
        // is still nullptr when we use this getDimensionById()
        return m_dimensions_by_id[id] * this->guiscale();
      }

      gfx::Color getColorById(const std::string& id) {
        ASSERT(m_colors_by_id.find(id) != m_colors_by_id.end());
        return m_colors_by_id[id];
      }

      void drawEntryCaret(ui::Graphics* g, ui::Entry* widget, int x, int y);

    protected:
      void onRegenerate() override;

    private:
      void loadAll(const std::string& skinId);
      void loadSheet(const std::string& skinId);
      void loadFonts(const std::string& skinId);
      void loadXml(const std::string& skinId);

      she::Surface* sliceSheet(she::Surface* sur, const gfx::Rect& bounds);
      gfx::Color getWidgetBgColor(ui::Widget* widget);
      void drawText(ui::Graphics* g, const char *t, gfx::Color fg_color, gfx::Color bg_color,
                    ui::Widget* widget, const gfx::Rect& rc,
                    int selected_offset);
      void drawEntryText(ui::Graphics* g, ui::Entry* widget);

      void paintIcon(ui::Widget* widget, ui::Graphics* g, ui::IButtonIcon* iconInterface, int x, int y);

      she::Font* loadFont(const std::string& userFont, const std::string& themeFont);
      std::string themeFileName(const std::string& skinId,
                                const std::string& fileName) const;

      she::Surface* m_sheet;
      std::map<std::string, SkinPartPtr> m_parts_by_id;
      std::map<std::string, she::Surface*> m_toolicon;
      std::map<std::string, gfx::Color> m_colors_by_id;
      std::map<std::string, int> m_dimensions_by_id;
      std::vector<ui::Cursor*> m_cursors;
      StyleSheet m_stylesheet;
      std::map<std::string, ui::Style*> m_styles;
      she::Font* m_defaultFont;
      she::Font* m_miniFont;
    };

    inline SkinPartPtr get_part_by_id(const std::string& id) {
      return static_cast<SkinTheme*>(ui::Manager::getDefault()->theme())->getPartById(id);
    }

    inline gfx::Color get_color_by_id(const std::string& id) {
      return static_cast<SkinTheme*>(ui::Manager::getDefault()->theme())->getColorById(id);
    }

  } // namespace skin
} // namespace app

#endif
