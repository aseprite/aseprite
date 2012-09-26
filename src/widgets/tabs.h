/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef WIDGETS_TABS_H_INCLUDED
#define WIDGETS_TABS_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <vector>

class Tabs;

// Interface used to control notifications from the Tabs widget.
class TabsDelegate
{
public:
  virtual ~TabsDelegate() { }

  // Called when the user presses a mouse button over a tab
  // button & 1  => left click
  // button & 2  => right click
  // button & 4  => middle click
  virtual void clickTab(Tabs* tabs, void* data, int button) = 0;

  // Called when the mouse is over a tab (the data can be null if the
  // mouse just leave all tabs)
  virtual void mouseOverTab(Tabs* tabs, void* data) = 0;
};

// Tabs control.
//
// Used to show opened files/sprites.
class Tabs : public ui::Widget
{
  struct Tab
  {
    std::string text;           // Label in the tab
    void* data;                 // Opaque pointer to user data
    int width;                  // Width of the tab

    Tab(const char* text, void* data)
    {
      this->text = text;
      this->data = data;
      this->width = 0;
    }
  };

  typedef std::vector<Tab*> TabsList;
  typedef TabsList::iterator TabsListIterator;

  enum Ani { ANI_NONE,
             ANI_ADDING_TAB,
             ANI_REMOVING_TAB,
             ANI_SCROLL,
             ANI_SMOOTH_SCROLL };

public:
  Tabs(TabsDelegate* delegate);
  ~Tabs();

  void addTab(const char* text, void* data);
  void removeTab(void* data);

  void setTabText(const char* text, void* data);

  void selectTab(void* data);
  void selectNextTab();
  void selectPreviousTab();
  void* getSelectedTab();

  void startScrolling();
  void stopScrolling();

protected:
  bool onProcessMessage(ui::Message* msg) OVERRIDE;
  void onPreferredSize(ui::PreferredSizeEvent& ev) OVERRIDE;
  void onInitTheme(ui::InitThemeEvent& ev) OVERRIDE;
  void onSetText() OVERRIDE;

private:
  void startAni(Ani ani);
  void stopAni();

  void selectTabInternal(Tab* tab);
  void drawTab(BITMAP* bmp, ui::JRect box, Tab* tab, int y_delta, bool selected);
  TabsListIterator getTabIteratorByData(void* data);
  Tab* getTabByData(void* data);
  int getMaxScrollX();
  void makeTabVisible(Tab* tab);
  void setScrollX(int scroll_x);
  void calculateHot();
  int calcTabWidth(Tab* tab);

  TabsList m_list_of_tabs;
  Tab *m_hot;
  Tab *m_selected;
  int m_scrollX;

  // Variables for animation purposes
  ui::Timer m_timer;
  int m_begScrollX;             // Initial X position of scroll in the animation when you scroll with mouse wheel
  int m_endScrollX;             // Final X position of scroll in the animation when you scroll with mouse wheel
  Ani m_ani;                    // Current animation
  int m_ani_t;                  // Number of ticks from the beginning of the animation
  Tab* m_removedTab;
  Tab* m_nextTabOfTheRemovedOne;

  // Buttons to scroll tabs (useful when there are more tabs than visible area)
  class ScrollButton;
  ScrollButton* m_button_left;
  ScrollButton* m_button_right;

  // Delegate of notifications
  TabsDelegate* m_delegate;
};

#endif
