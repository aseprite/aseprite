// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/script/tabs_widget.h"
#include "tabs_widget.h"

#define TAB_CONTENT_ID(tabid)  (tabid + "_content")

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

Tab* Tabs::addTab(const std::string& id, const std::string& text)
{
  auto content = new ui::Grid(2, false);
  content->setExpansive(true);
  content->setVisible(false);
  content->setId(TAB_CONTENT_ID(id).c_str());
  m_pages.addChild(content);

  if (m_buttons.children().empty()) {
    m_buttonsBox.addChild(&m_buttons);
    m_buttons.ItemChange.connect([this](ButtonSet::Item* tab) {
      int oldSelectedTabIndex = m_selectedTab;
      for (int i=0; i<m_pages.children().size(); ++i) {
        auto tabContent = m_pages.children()[i];
        bool isSelectedTab = (TAB_CONTENT_ID(tab->id()) == tabContent->id());
        tabContent->setVisible(isSelectedTab);
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
  auto tab = new Tab(content);
  tab->setText(text);
  tab->setId(id.c_str());
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
  for (int i=0; i<m_buttons.children().size(); ++i) {
    if (m_buttons.children()[i]->id() == id)
      return i;
  }
  return -1;
}

int Tabs::tabIndexByText(const std::string& text) const
{
  for (int i=0; i<m_buttons.children().size(); ++i) {
    if (m_buttons.children()[i]->text() == text)
      return i;
  }
  return -1;
}

std::string Tabs::tabId(int index) const
{
  return index >= 0 && index < m_buttons.children().size() ?
         m_buttons.children()[index]->id() :
         std::string();
}

std::string Tabs::tabText(int index) const
{
  return index >= 0 && index < m_buttons.children().size() ?
         m_buttons.children()[index]->text() :
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

Tab::Tab(ui::Grid* content) : m_content(content)
{
}

void Tab::onClick()
{
  Click();
  ButtonSet::Item::onClick();
}

} // namespace script
} // namespace app

#endif // ENABLE_UI