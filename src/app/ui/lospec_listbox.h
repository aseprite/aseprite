// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_LOSPEC_LISTBOX_H_INCLUDED
#define APP_UI_LOSPEC_LISTBOX_H_INCLUDED
#pragma once

#include "app/lospec.h"
#include "obs/signal.h"
#include "ui/listbox.h"
#include "ui/timer.h"

#include <string>

namespace app {

class HttpLoader;

// A list box that queries the Lospec palette-list online and shows the
// resulting palettes (with a small color preview). All available pages
// are fetched automatically (one after another) so the whole
// palette-list becomes browsable. Double-clicking an item fires the
// ImportPalette() signal so the palette can be downloaded and saved as
// a local preset.
class LospecListBox : public ui::ListBox {
public:
  LospecListBox();
  ~LospecListBox();

  // Starts an online search with the given term (empty term = all
  // palettes, most popular first) and optional color-count filter.
  // Cancels any in-flight request and begins loading from the first
  // page.
  void search(const std::string& query,
              lospec::ColorFilter filter = lospec::ColorFilter::Any,
              int colorNumber = 0);

  // Returns the palette info for the currently selected item, or
  // nullptr if nothing valid is selected.
  const lospec::PaletteInfo* selectedPalette() const;

  // Fired when the user picks a palette to import.
  obs::signal<void(const lospec::PaletteInfo&)> ImportPalette;
  // Fired when the whole search finished loading (all pages, or an
  // error/empty result).
  obs::signal<void()> FinishLoading;

private:
  bool onProcessMessage(ui::Message* msg) override;
  void onTick();
  void onChange() override;

  void startLoadingPage(int page);
  void stopLoading();
  // Parses one downloaded page. Returns the number of palettes added,
  // or -1 on a parse error.
  int parsePage(const std::string& filename);
  void clearItems();
  void updateProgress();

  std::string m_query;
  lospec::ColorFilter m_filter = lospec::ColorFilter::Any;
  int m_colorNumber = 0;
  ui::Timer m_timer;
  HttpLoader* m_loader;

  // Pagination state.
  int m_page = 0;           // Page currently being downloaded.
  int m_loadedCount = 0;    // Number of palettes added so far.
  int m_totalCount = -1;    // Total palettes reported by the API (-1 = unknown).
  bool m_loading = false;   // True while we are still fetching pages.
  ui::ListItem* m_progressItem = nullptr; // Shown at the bottom while loading.
};

} // namespace app

#endif
