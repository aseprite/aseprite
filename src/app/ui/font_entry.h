// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FONT_ENTRY_H_INCLUDED
#define APP_UI_FONT_ENTRY_H_INCLUDED
#pragma once

#include "app/font_info.h"
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
      User,
      Face,
      Size,
      Style,
      Flags,
    };

    FontEntry();
    ~FontEntry();

    FontInfo info() { return m_info; }
    void setInfo(const FontInfo& info, From from = From::User);

    obs::signal<void(const FontInfo&)> FontChange;

  private:

    class FontFace : public SearchEntry {
    public:
      FontFace();
      obs::signal<void(const FontInfo&)> FontChange;
    protected:
      bool onProcessMessage(ui::Message* msg) override;
      void onChange() override;
    private:
      std::unique_ptr<FontPopup> m_popup;
    };

    class FontSize : public ui::ComboBox {
    public:
      FontSize();
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
  };

} // namespace app

#endif
