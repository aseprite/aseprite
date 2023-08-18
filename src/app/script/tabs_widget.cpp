// Aseprite
// Copyright (c) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/script/tabs_widget.h"

#ifdef ENABLE_UI

using namespace ui;

namespace app {
namespace script {

// static
ui::WidgetType Tabs::Type()
{
  static ui::WidgetType type = ui::kGenericWidget;
  if (type == ui::kGenericWidget)
    type = ui::register_widget_type();
  return type;
}

Tabs::Tabs(int selectorFlags) : m_selectorFlags(selectorFlags)
{
  m_sepL.setExpansive(true);
  m_sepR.setExpansive(true);
  m_pages.setExpansive(true);
  setType(Type());
  layoutChilds();
}

Tab* Tabs::addTab(Grid* content)
{
  m_pages.addChild(content);

  if (m_buttons.children().empty()) {
    m_buttonsBox.addChild(&m_buttons);
    m_buttons.ItemChange.connect([this](ButtonSet::Item* selItem) {
      int oldSelectedTabIndex = m_selectedTab;
      for (int i=0; i<m_pages.children().size(); ++i) {
        auto tab = m_pages.children()[i];
        bool isSelectedTab = selItem->text() ==  tab->text();
        tab->setVisible(isSelectedTab);
        if (isSelectedTab)
          m_selectedTab = i;
      }
      layout();
      if (oldSelectedTabIndex != m_selectedTab) {
        TabChanged();
      }
    });
  }
  else {
    m_buttons.setColumns(m_pages.children().size());
  }
  auto tab = new Tab();
  tab->setText(content->text());
  m_buttons.addItem(tab);
  // Select the first tab by default.
  if (m_buttons.children().size() == 1) {
    selectTab(0);
  }

  m_buttons.initTheme();

  return tab;
}

void Tabs::selectTab(int index)
{
  if (index >= 0 && index < m_pages.children().size()) {
    for (int i=0; i<m_pages.children().size(); ++i) {
      m_buttons.getItem(i)->setSelected(i == index);
      m_pages.children()[i]->setVisible(i == index);
    }
    m_selectedTab = index;
  }
}

void Tabs::setSelectorFlags(int selectorFlags)
{
  if (selectorFlags != m_selectorFlags) {
    m_selectorFlags = selectorFlags;
    layoutChilds();
  }
}

int Tabs::tabIndexById(const std::string& id) const
{
  for (int i=0; i<m_pages.children().size(); ++i) {
    if (m_pages.children()[i]->id() == id)
      return i;
  }
  return -1;
}

int Tabs::tabIndexByText(const std::string& text) const
{
  for (int i=0; i<m_pages.children().size(); ++i) {
    if (m_pages.children()[i]->text() == text)
      return i;
  }
  return -1;
}

std::string Tabs::tabId(int index) const
{
  return index >= 0 && index < m_pages.children().size() ?
         m_pages.children()[index]->id() :
         std::string();
}

std::string Tabs::tabText(int index) const
{
  return index >= 0 && index < m_pages.children().size() ?
         m_pages.children()[index]->text() :
         std::string();
}

int Tabs::selectedTab() const
{
  return m_selectedTab;
}

void Tabs::layoutChilds()
{
  // Put the tab selector at the bottom or at the top.
  removeAllChildren();
  if (m_selectorFlags & ui::BOTTOM) {
    addChild(&m_sepTB);
    addChild(&m_pages);
    addChild(&m_selector);
  }
  else {
    addChild(&m_selector);
    addChild(&m_pages);
    addChild(&m_sepTB);
  }

  // Align the selector buttons.
  m_selector.removeAllChildren();
  if (m_selectorFlags & ui::LEFT) {
    m_selector.addChild(&m_buttonsBox);
    m_selector.addChild(&m_sepR);
  }
  else if (m_selectorFlags & ui::RIGHT) {
    m_selector.addChild(&m_sepL);
    m_selector.addChild(&m_buttonsBox);
  }
  else {
    m_selector.addChild(&m_sepL);
    m_selector.addChild(&m_buttonsBox);
    m_selector.addChild(&m_sepR);
  }
}

void Pages::onSizeHint(ui::SizeHintEvent& ev)
{
  gfx::Size prefSize(0, 0);
  for (auto child : children()) {

    gfx::Size childSize = child->sizeHint();
    prefSize.w = std::max(childSize.w, prefSize.w);
    prefSize.h = std::max(childSize.h, prefSize.h);
  }
  prefSize.w += border().width();
  prefSize.h += border().height();

  ev.setSizeHint(prefSize);
}


void Tab::onClick()
{
  Click();
  ButtonSet::Item::onClick();
}

} // namespace script
} // namespace app

#endif // ENABLE_UI