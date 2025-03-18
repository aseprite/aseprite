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
    Popup,
  };

  FontEntry();
  ~FontEntry();

  FontInfo info() { return m_info; }
  void setInfo(const FontInfo& info, From from);

  obs::signal<void(const FontInfo&, From)> FontChange;

private:
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
    FontStyle();
  };

  class FontLigatures : public ButtonSet {
  public:
    FontLigatures();
  };

  FontInfo m_info;
  FontFace m_face;
  FontSize m_size;
  FontStyle m_style;
  FontLigatures m_ligatures;
  ui::CheckBox m_antialias;
  bool m_lockFace = false;
};

} // namespace app

#endif
