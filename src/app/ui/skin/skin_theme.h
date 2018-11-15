// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_THEME_H_INCLUDED
#define APP_UI_SKIN_SKIN_THEME_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "gfx/color.h"
#include "gfx/fwd.h"
#include "ui/manager.h"
#include "ui/scale.h"
#include "ui/theme.h"

#include "theme.xml.h"

#include <map>
#include <string>
#include <vector>

namespace ui {
  class Entry;
  class Graphics;
}

namespace os {
  class Surface;
}

namespace app {
  namespace skin {

    class FontData;

    // This is the GUI theme used by Aseprite (which use images from
    // data/skins directory).
    class SkinTheme : public ui::Theme
                    , public app::gen::ThemeFile<SkinTheme> {
    public:
      static const char* kThemesFolderName;

      static SkinTheme* instance();

      SkinTheme();
      ~SkinTheme();

      const std::string& path() { return m_path; }
      int preferredScreenScaling() { return m_preferredScreenScaling; }
      int preferredUIScaling() { return m_preferredUIScaling; }

      os::Font* getDefaultFont() const override { return m_defaultFont; }
      os::Font* getWidgetFont(const ui::Widget* widget) const override;
      os::Font* getMiniFont() const { return m_miniFont; }

      ui::Cursor* getStandardCursor(ui::CursorType type) override;
      void initWidget(ui::Widget* widget) override;
      void getWindowMask(ui::Widget* widget, gfx::Region& region) override;
      int getScrollbarSize() override;
      gfx::Size getEntryCaretSize(ui::Widget* widget) override;

      void paintEntry(ui::PaintEvent& ev) override;
      void paintListBox(ui::PaintEvent& ev) override;
      void paintMenu(ui::PaintEvent& ev) override;
      void paintMenuItem(ui::PaintEvent& ev) override;
      void paintSlider(ui::PaintEvent& ev) override;
      void paintComboBoxEntry(ui::PaintEvent& ev) override;
      void paintTextBox(ui::PaintEvent& ev) override;
      void paintViewViewport(ui::PaintEvent& ev) override;

      SkinPartPtr getToolPart(const char* toolId) const;
      os::Surface* getToolIcon(const char* toolId) const;

      // Helper functions to draw bounds/hlines with sheet parts
      void drawRect(ui::Graphics* g, const gfx::Rect& rc,
                    os::Surface* nw, os::Surface* n, os::Surface* ne,
                    os::Surface* e, os::Surface* se, os::Surface* s,
                    os::Surface* sw, os::Surface* w);
      void drawRect(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart, const bool drawCenter = true);
      void drawRect2(ui::Graphics* g, const gfx::Rect& rc, int x_mid, SkinPart* nw1, SkinPart* nw2);
      void drawHline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart);
      void drawVline(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart);
      void paintProgressBar(ui::Graphics* g, const gfx::Rect& rc, double progress);

      ui::Style* getStyleById(const std::string& id) const {
        auto it = m_styles.find(id);
        if (it != m_styles.end())
          return it->second;
        else
          return nullptr;
      }

      SkinPartPtr getPartById(const std::string& id) const {
        auto it = m_parts_by_id.find(id);
        if (it != m_parts_by_id.end())
          return it->second;
        else
          return SkinPartPtr(nullptr);
      }

      ui::Cursor* getCursorById(const std::string& id) const {
        auto it = m_cursors.find(id);
        if (it != m_cursors.end())
          return it->second;
        else
          return nullptr;
      }

      int getDimensionById(const std::string& id) const {
        auto it = m_dimensions_by_id.find(id);
        if (it != m_dimensions_by_id.end())
          return it->second * ui::guiscale();
        else
          return 0;
      }

      gfx::Color getColorById(const std::string& id) const {
        auto it = m_colors_by_id.find(id);
        if (it != m_colors_by_id.end())
          return it->second;
        else
          return gfx::ColorNone;
      }

      void drawEntryCaret(ui::Graphics* g, ui::Entry* widget, int x, int y);

    protected:
      void onRegenerateTheme() override;

    private:
      void loadFontData();
      void loadAll(const std::string& themeId);
      void loadSheet();
      void loadXml();

      os::Surface* sliceSheet(os::Surface* sur, const gfx::Rect& bounds);
      gfx::Color getWidgetBgColor(ui::Widget* widget);
      void drawText(ui::Graphics* g, const char *t, gfx::Color fg_color, gfx::Color bg_color,
                    ui::Widget* widget, const gfx::Rect& rc,
                    int selected_offset, int mnemonic);
      void drawEntryText(ui::Graphics* g, ui::Entry* widget);

      std::string findThemePath(const std::string& themeId) const;

      std::string m_path;
      os::Surface* m_sheet;
      std::map<std::string, SkinPartPtr> m_parts_by_id;
      std::map<std::string, gfx::Color> m_colors_by_id;
      std::map<std::string, int> m_dimensions_by_id;
      std::map<std::string, ui::Cursor*> m_cursors;
      std::vector<ui::Cursor*> m_standardCursors;
      std::map<std::string, ui::Style*> m_styles;
      std::map<std::string, FontData*> m_fonts;
      std::map<std::string, os::Font*> m_themeFonts;
      os::Font* m_defaultFont;
      os::Font* m_miniFont;
      int m_preferredScreenScaling;
      int m_preferredUIScaling;
    };

  } // namespace skin
} // namespace app

#endif
