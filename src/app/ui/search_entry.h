// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SEARCH_ENTRY_H_INCLUDED
#define APP_UI_SEARCH_ENTRY_H_INCLUDED
#pragma once

#include "ui/entry.h"

namespace ui {
class Timer;
}

namespace app {

class SearchEntry : public ui::Entry {
public:
  SearchEntry();

  void setClearOnEsc(bool clearOnEsc) { m_clearOnEsc = clearOnEsc; }
  bool clearOnEsc() const { return m_clearOnEsc; }

  void clear() { onCloseIconPressed(); }

  void setDebounce(int ms);
  int debounceMs() const { return m_debounceMs; }

protected:
  bool onProcessMessage(ui::Message* msg) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onChange() override;
  gfx::Rect onGetEntryTextBounds() const override;

  virtual os::Surface* onGetCloseIcon() const;
  virtual void onCloseIconPressed();

private:
  gfx::Rect getCloseIconBounds() const;
  bool m_clearOnEsc;
  int m_debounceMs;
  std::unique_ptr<ui::Timer> m_debounceTimer;
};

} // namespace app

#endif
