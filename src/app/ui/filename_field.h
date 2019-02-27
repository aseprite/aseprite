// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FILENAME_FIELD_H_INCLUDED
#define APP_UI_FILENAME_FIELD_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/entry.h"

#include <string>

namespace app {

  class FilenameField : public ui::HBox {
  public:
    enum Type { EntryAndButton, ButtonOnly };

    FilenameField(const Type type,
                  const std::string& pathAndFilename);

    std::string filename() const;
    void setFilename(const std::string& fn);

    obs::signal<std::string()> SelectFile;
    obs::signal<void()> Change;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onInitTheme(ui::InitThemeEvent& ev) override;

  private:
    void updateWidgets();

    std::string m_path;
    std::string m_file;
    ui::Entry* m_entry;
    ui::Button m_button;
  };

} // namespace app

#endif
