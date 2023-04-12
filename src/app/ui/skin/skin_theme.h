// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
#include "ui/cursor.h"
#include "ui/cursor_type.h"
#include "ui/manager.h"
#include "ui/scale.h"
#include "ui/theme.h"

#include "theme.xml.h"

#include <array>
#include <map>
#include <string>

namespace ui {
  class Entry;
  class Graphics;
}

namespace app {
  namespace skin {

    class FontData;

    class ThemeFont {
      public:
        ThemeFont() {}
        ThemeFont(os::FontRef font, bool mnemonics) : m_font(font), m_mnemonics(mnemonics) {}
        os::FontRef font() { return m_font; }
        bool mnemonics() { return m_mnemonics; }
      private:
        os::FontRef m_font;
        bool m_mnemonics;
    };

    // This is the GUI theme used by Aseprite (which use images from
    // data/skins directory).
    class SkinTheme : public ui::Theme
                    , public app::gen::ThemeFile<SkinTheme> {
    public:
      static const char* kThemesFolderName;

      static SkinTheme* instance();
      static SkinTheme* get(const ui::Widget* widget);

      SkinTheme();
      ~SkinTheme();

      const std::string& path() { return m_path; }
      int preferredScreenScaling() { return m_preferredScreenScaling; }
      int preferredUIScaling() { return m_preferredUIScaling; }

      os::Font* getDefaultFont() const override { return m_defaultFont.get(); }
      os::Font* getWidgetFont(const ui::Widget* widget) const override;
      os::Font* getMiniFont() const { return m_miniFont.get(); }
      os::Font* getUnscaledFont(os::Font* font) const {
        auto it = m_unscaledFonts.find(font);
        if (it != m_unscaledFonts.end())
          return it->second.get();
        else
          return font;
      }

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
      void drawRectUsingUnscaledSheet(ui::Graphics* g, const gfx::Rect& rc, SkinPart* skinPart, const bool drawCenter = true);
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

      SkinPartPtr getUnscaledPartById(const std::string& id) const {
        auto it = m_unscaledParts_by_id.find(id);
        if (it != m_unscaledParts_by_id.end())
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
      class BackwardCompatibility;

      void loadFontData();
      void loadAll(const std::string& themeId,
                   BackwardCompatibility* backward = nullptr);
      void loadSheet();
      void loadXml(BackwardCompatibility* backward);

      os::SurfaceRef sliceSheet(os::SurfaceRef sur, const gfx::Rect& bounds);
      os::SurfaceRef sliceUnscaledSheet(os::SurfaceRef sur, const gfx::Rect& bounds);
      gfx::Color getWidgetBgColor(ui::Widget* widget);
      void drawText(ui::Graphics* g,
                    const char* t,
                    const gfx::Color fgColor,
                    const gfx::Color bgColor,
                    const ui::Widget* widget,
                    const gfx::Rect& rc,
                    const int textAlign,
                    const int mnemonic);
      void drawEntryText(ui::Graphics* g, ui::Entry* widget);

      std::string findThemePath(const std::string& themeId) const;

      std::string m_path;
      os::SurfaceRef m_sheet;
      // Contains the sheet surface as is, without any scale.
      os::SurfaceRef m_unscaledSheet;
      std::map<std::string, SkinPartPtr> m_parts_by_id;
      // Stores the same SkinParts as m_parts_by_id but unscaled, using the same keys.
      std::map<std::string, SkinPartPtr> m_unscaledParts_by_id;
      std::map<std::string, gfx::Color> m_colors_by_id;
      std::map<std::string, int> m_dimensions_by_id;
      std::map<std::string, ui::Cursor*> m_cursors;
      std::array<ui::Cursor*, ui::kCursorTypes> m_standardCursors;
      std::map<std::string, ui::Style*> m_styles;
      std::map<std::string, FontData*> m_fonts;
      std::map<std::string, ThemeFont> m_themeFonts;
      // Stores the unscaled font version of the Font pointer used as a key.
      std::map<os::Font*, os::FontRef> m_unscaledFonts;
      os::FontRef m_defaultFont;
      os::FontRef m_miniFont;
      int m_preferredScreenScaling;
      int m_preferredUIScaling;
    };

  } // namespace skin
} // namespace app

#endif
