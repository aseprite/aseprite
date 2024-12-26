// Aseprite
// Copyright (C) 2023-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_TABS_H_INCLUDED
#define APP_SCRIPT_TABS_H_INCLUDED
#pragma once

#include "app/ui/button_set.h"
#include "ui/box.h"
#include "ui/grid.h"
#include "ui/separator.h"
#include "ui/size_hint_event.h"
#include "ui/widget.h"

namespace app { namespace script {

class Tab : public app::ButtonSet::Item {
public:
  Tab(ui::Grid* content);
  ui::Grid* content() { return m_content; }

  obs::signal<void()> Click;

protected:
  virtual void onClick();

private:
  ui::Grid* m_content;
};

class Pages : public ui::VBox {
protected:
  void onSizeHint(ui::SizeHintEvent& ev);
};

class Tabs : public ui::VBox {
public:
  static ui::WidgetType Type();

  Tabs(int selectorFlags);

  Tab* addTab(const std::string& id, const std::string& text);
  void selectTab(int index);
  void setSelectorFlags(int selectorFlags);

  int tabIndexById(const std::string& text) const;
  int tabIndexByText(const std::string& text) const;
  std::string tabId(int index) const;
  std::string tabText(int index) const;
  int selectedTab() const;
  int size() const { return m_pages.children().size(); };

  obs::signal<void()> TabChanged;

private:
  void layoutChilds();

  ui::HBox m_selector;
  ui::Separator m_sepL = ui::Separator(std::string(), ui::HORIZONTAL);
  ui::Separator m_sepR = ui::Separator(std::string(), ui::HORIZONTAL);
  ui::Separator m_sepTB = ui::Separator(std::string(), ui::HORIZONTAL);
  ui::HBox m_buttonsBox;
  app::ButtonSet m_buttons = app::ButtonSet(1);
  Pages m_pages;
  int m_selectedTab = 0;
  int m_selectorFlags;
};

}} // namespace app::script

#endif
