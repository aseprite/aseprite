// Aseprite
// Copyright (c) 2024-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FONT_ENTRY_H_INCLUDED
#define APP_UI_FONT_ENTRY_H_INCLUDED
#pragma once

#include "app/fonts/font_info.h"
#include "app/ui/button_set.h"
#include "app/ui/search_entry.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/int_entry.h"
#include "ui/paint.h"
#include "ui/tooltips.h"

#include <memory>
#include <string>

namespace app {
class FontPopup;

class FontEntry : public ui::HBox {
public:
  enum class From {
    Init,
    Face,
    Size,
    Style,
    Flags,
    Hinting,
    Popup,
    Paint,
  };

  FontEntry(bool withStrokeAndFill);
  ~FontEntry();

  FontInfo info() { return m_info; }
  void setInfo(const FontInfo& info, From from);
  void setReadOnly(bool readOnly)
  {
    m_face.setReadOnly(readOnly);
    m_size.setEnabled(!readOnly && isEnabled());
    m_size.getEntryWidget()->setReadOnly(readOnly);
    m_style.setEnabled(!readOnly && isEnabled());
  }
  bool isReadOnly() const { return m_face.isReadOnly(); }

  ui::Paint paint();

  obs::signal<void(const FontInfo&, From)> FontChange;

private:
  void onStyleItemClick(ButtonSet::Item* item);
  void onStrokeChange();

  class FontFace : public SearchEntry {
  public:
    FontFace();
    obs::signal<void(const FontInfo&, From)> FontChange;

  protected:
    void onInitTheme(ui::InitThemeEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;
    void onChange() override;
    os::Surface* onGetCloseIcon() const override;
    void onCloseIconPressed() override;

  private:
    FontEntry* fontEntry() const { return static_cast<FontEntry*>(parent()); }

    std::unique_ptr<FontPopup> m_popup;
    bool m_fromEntryChange = false;
  };

  class FontSize : public ui::ComboBox {
  public:
    FontSize();
    void updateForFont(const FontInfo& info);

  protected:
    void onEntryChange() override;
  };

  class FontStyle : public ButtonSet {
  public:
    FontStyle(ui::TooltipManager* tooltips);
  };

  class FontStroke : public HBox {
  public:
    FontStroke(ui::TooltipManager* tooltips);
    bool fill() const;
    float stroke() const;
    obs::signal<void()> Change;

  private:
    class WidthEntry : public ui::IntEntry,
                       public ui::SliderDelegate {
    public:
      WidthEntry();
      obs::signal<void()> ValueChange;

    private:
      void onValueChange() override;
      bool onAcceptUnicodeChar(int unicodeChar) override;
      // SliderDelegate impl
      std::string onGetTextFromValue(int value) override;
      int onGetValueFromText(const std::string& text) override;
    };
    ButtonSet m_fill;
    WidthEntry m_stroke;
  };

  ui::TooltipManager m_tooltips;
  FontInfo m_info;
  FontFace m_face;
  FontSize m_size;
  FontStyle m_style;
  std::unique_ptr<FontStroke> m_stroke;
  std::vector<text::FontStyle::Weight> m_availableWeights;
  bool m_lockFace = false;
};

} // namespace app

#endif
