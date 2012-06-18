// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_MENU_H_INCLUDED
#define GUI_MENU_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "base/unique_ptr.h"
#include "gui/widget.h"

namespace ui {

  class MenuItem;
  struct MenuBaseData;
  class Timer;

  class Menu : public Widget
  {
  public:
    Menu();
    ~Menu();

    void showPopup(int x, int y);

    // Returns the MenuItem that has as submenu this menu.
    MenuItem* getOwnerMenuItem() {
      return m_menuitem;
    }

  protected:
    virtual bool onProcessMessage(Message* msg) OVERRIDE;

  private:
    void setOwnerMenuItem(MenuItem* ownerMenuItem) {
      m_menuitem = ownerMenuItem;
    }

    void requestSize(int* w, int* h);
    void set_position(JRect rect);
    void closeAll();

    MenuItem* getHighlightedItem();
    void highlightItem(MenuItem* menuitem, bool click, bool open_submenu, bool select_first_child);
    void unhighlightItem();

    MenuItem* m_menuitem;         // From where the menu was open

    friend class MenuBox;
    friend class MenuItem;
  };

  class MenuBox : public Widget
  {
  public:
    MenuBox(int type = JI_MENUBOX);
    ~MenuBox();

    Menu* getMenu();
    void setMenu(Menu* menu);

    MenuBaseData* getBase() {
      return m_base;
    }

    // Closes all menu-boxes and goes back to the normal state of the
    // menu-bar.
    void cancelMenuLoop();

  protected:
    virtual bool onProcessMessage(Message* msg) OVERRIDE;
    MenuBaseData* createBase();

  private:
    void requestSize(int* w, int* h);
    void set_position(JRect rect);
    void closePopup();

    MenuBaseData* m_base;

    friend class Menu;
  };

  class MenuBar : public MenuBox
  {
  public:
    MenuBar();
  };

  class MenuItem : public Widget
  {
  public:
    MenuItem(const char *text);
    ~MenuItem();

    Menu* getSubmenu();
    void setSubmenu(Menu* submenu);

    JAccel getAccel();
    void setAccel(JAccel accel);

    bool isHighlighted() const;
    void setHighlighted(bool state);

    // Returns true if the MenuItem has a submenu.
    bool hasSubmenu() const;

    // Returns true if the submenu is opened.
    bool hasSubmenuOpened() const {
      return (m_submenu_menubox != NULL);
    }

    // Returns the menu-box where the sub-menu has been opened, or just
    // NULL if the sub-menu is closed.
    MenuBox* getSubmenuContainer() const {
      return m_submenu_menubox;
    }

    // Fired when the menu item is clicked.
    Signal0<void> Click;

  protected:
    virtual bool onProcessMessage(Message* msg) OVERRIDE;
    virtual void onClick();

  private:
    void requestSize(int* w, int* h);
    void openSubmenu(bool select_first);
    void closeSubmenu(bool last_of_close_chain);
    void startTimer();
    void stopTimer();
    void executeClick();

    JAccel m_accel;               // Hot-key
    bool m_highlighted;           // Is it highlighted?
    Menu* m_submenu;              // The sub-menu
    MenuBox* m_submenu_menubox;   // The opened menubox for this menu-item
    UniquePtr<Timer> m_submenu_timer; // Timer to open the submenu

    friend class Menu;
    friend class MenuBox;
  };

  int jm_open_menuitem();
  int jm_close_menuitem();
  int jm_close_popup();
  int jm_exe_menuitem();

} // namespace ui

#endif
