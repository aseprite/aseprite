// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>
#include <ctype.h>
#include <stdio.h>

#include "gfx/size.h"
#include "ui/gui.h"
#include "ui/intern.h"

static const int kTimeoutToOpenSubmenu = 250;

using namespace gfx;

namespace ui {

//////////////////////////////////////////////////////////////////////
// Internal messages: to move between menus

// Extra fields for the kOpenMenuItemMessage message:
// bool select_first = msg->user.a;
//   If this value is true, it means that after opening the menu, we
//   have to select the first item (i.e. highlighting it).
RegisterMessage kOpenMenuItemMessage;

// Extra fields for the kCloseMenuItemMessage message:
// bool last_of_close_chain = msg->user.a;
//   This fields is used to indicate the end of a sequence of
//   kOpenMenuItemMessage and kCloseMenuItemMessage messages. If it is true
//   the message is the last one of the chain, which means that no
//   more kOpenMenuItemMessage or kCloseMenuItemMessage messages are in the queue.
RegisterMessage kCloseMenuItemMessage;

RegisterMessage kClosePopupMessage;

RegisterMessage kExecuteMenuItemMessage;

// Data for the main jmenubar or the first popuped-jmenubox
struct MenuBaseData
{
  // True when the menu-items must be opened with the cursor movement
  bool was_clicked : 1;

  // True when there's kOpen/CloseMenuItemMessage messages in queue, to
  // avoid start processing another menuitem-request when we're
  // already working in one
  bool is_processing : 1;

  // True when the kMouseDownMessage is being filtered
  bool is_filtering : 1;

  bool close_all : 1;

  MenuBaseData()
  {
    was_clicked = false;
    is_filtering = false;
    is_processing = false;
    close_all = false;
  }

};

class CustomizedWindowForMenuBox : public Window
{
public:
  CustomizedWindowForMenuBox(MenuBox* menubox)
    : Window(false, NULL)
  {
    setMoveable(false); // Can't move the window
    addChild(menubox);
    remapWindow();
  }

protected:
  bool onProcessMessage(Message* msg) OVERRIDE
  {
    switch (msg->type) {

      case kCloseMessage:
        // Delete this window automatically
        deferDelete();
        break;

    }
    return Window::onProcessMessage(msg);
  }
};

static MenuBox* get_base_menubox(Widget* widget);
static MenuBaseData* get_base(Widget* widget);

static bool window_msg_proc(Widget* widget, Message* msg);

static MenuItem* check_for_letter(Menu* menu, int ascii);
static MenuItem* check_for_accel(Menu* menu, Message* msg);

static MenuItem* find_nextitem(Menu* menu, MenuItem* menuitem);
static MenuItem* find_previtem(Menu* menu, MenuItem* menuitem);

Menu::Menu()
  : Widget(kMenuWidget)
  , m_menuitem(NULL)
{
  initTheme();
}

Menu::~Menu()
{
  if (m_menuitem) {
    if (m_menuitem->getSubmenu() == this) {
      m_menuitem->setSubmenu(NULL);
    }
    else {
      ASSERT(m_menuitem->getSubmenu() == NULL);
    }
  }
}

MenuBox::MenuBox(WidgetType type)
 : Widget(type)
 , m_base(NULL)
{
  this->setFocusStop(true);
  initTheme();
}

MenuBox::~MenuBox()
{
  if (m_base && m_base->is_filtering) {
    m_base->is_filtering = false;
    Manager::getDefault()->removeMessageFilter(kMouseDownMessage, this);
  }

  delete m_base;
}

MenuBar::MenuBar()
  : MenuBox(kMenuBarWidget)
{
  createBase();
}

MenuItem::MenuItem(const char *text)
  : Widget(kMenuItemWidget)
{
  m_accel = NULL;
  m_highlighted = false;
  m_submenu = NULL;
  m_submenu_menubox = NULL;

  setText(text);
  initTheme();
}

MenuItem::~MenuItem()
{
  if (m_accel)
    delete m_accel;

  if (m_submenu)
    delete m_submenu;
}

Menu* MenuBox::getMenu()
{
  if (getChildren().empty())
    return NULL;
  else
    return static_cast<Menu*>(getChildren().front());
}

MenuBaseData* MenuBox::createBase()
{
  delete m_base;
  m_base = new MenuBaseData();
  return m_base;
}

Menu* MenuItem::getSubmenu()
{
  return m_submenu;
}

Accelerator* MenuItem::getAccel()
{
  return m_accel;
}

void MenuBox::setMenu(Menu* menu)
{
  if (Menu* oldMenu = getMenu())
    removeChild(oldMenu);

  if (menu) {
    ASSERT_VALID_WIDGET(menu);
    addChild(menu);
  }
}

void MenuItem::setSubmenu(Menu* menu)
{
  if (m_submenu)
    m_submenu->setOwnerMenuItem(NULL);

  m_submenu = menu;

  if (m_submenu) {
    ASSERT_VALID_WIDGET(m_submenu);
    m_submenu->setOwnerMenuItem(this);
  }
}

/**
 * Changes the keyboard shortcuts (accelerators) for the specified
 * widget (a menu-item).
 *
 * @warning The specified @a accel will be freed automatically when
 *          the menu-item is deleted.
 */
void MenuItem::setAccel(Accelerator* accel)
{
  if (m_accel)
    delete m_accel;

  m_accel = accel;
}

bool MenuItem::isHighlighted() const
{
  return m_highlighted;
}

void MenuItem::setHighlighted(bool state)
{
  m_highlighted = state;
}

bool MenuItem::hasSubmenu() const
{
  return (m_submenu && !m_submenu->getChildren().empty());
}

void Menu::showPopup(int x, int y)
{
  do {
    jmouse_poll();
  } while (jmouse_b(0));

  // New window and new menu-box
  Window* window = new Window(false, NULL);
  MenuBox* menubox = new MenuBox();
  MenuBaseData* base = menubox->createBase();
  base->was_clicked = true;
  base->is_filtering = true;
  Manager::getDefault()->addMessageFilter(kMouseDownMessage, menubox);

  window->setMoveable(false);   // Can't move the window

  // Set children
  menubox->setMenu(this);
  window->addChild(menubox);

  window->remapWindow();

  // Menubox position
  window->positionWindow(MID(0, x, JI_SCREEN_W-jrect_w(window->rc)),
                         MID(0, y, JI_SCREEN_H-jrect_h(window->rc)));

  // Set the focus to the new menubox
  Manager::getDefault()->setFocus(menubox);
  menubox->setFocusMagnet(true);

  // Open the window
  window->openWindowInForeground();

  // Free the keyboard focus
  Manager::getDefault()->freeFocus();

  // Fetch the "menu" so it isn't destroyed
  menubox->setMenu(NULL);

  // Destroy the window
  delete window;
}

void Menu::onPaint(PaintEvent& ev)
{
  getTheme()->paintMenu(ev);
}

void Menu::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  Rect cpos = getChildrenBounds();
  bool isBar = (getParent()->type == kMenuBarWidget);

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    Size reqSize = child->getPreferredSize();

    if (isBar)
      cpos.w = reqSize.w;
    else
      cpos.h = reqSize.h;

    child->setBounds(cpos);

    if (isBar)
      cpos.x += cpos.w;
    else
      cpos.y += cpos.h;
  }
}

void Menu::onPreferredSize(PreferredSizeEvent& ev)
{
  Size size(0, 0);
  Size reqSize;

  UI_FOREACH_WIDGET_WITH_END(getChildren(), it, end) {
    reqSize = (*it)->getPreferredSize();

    if (this->getParent()->type == kMenuBarWidget) {
      size.w += reqSize.w + ((it+1 != end) ? this->child_spacing: 0);
      size.h = MAX(size.h, reqSize.h);
    }
    else {
      size.w = MAX(size.w, reqSize.w);
      size.h += reqSize.h + ((it+1 != end) ? this->child_spacing: 0);
    }
  }

  size.w += this->border_width.l + this->border_width.r;
  size.h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(size);
}

bool MenuBox::onProcessMessage(Message* msg)
{
  Menu* menu = MenuBox::getMenu();

  switch (msg->type) {

    case kMouseMoveMessage:
      /* isn't pressing a button? */
      if (!msg->mouse.flags && !get_base(this)->was_clicked)
        break;

      // Fall though

    case kMouseDownMessage:
      if (menu) {
        if (get_base(this)->is_processing)
          break;

        // Here we catch the filtered messages (menu-bar or the
        // popuped menu-box) to detect if the user press outside of
        // the widget
        if (msg->type == kMouseDownMessage && m_base != NULL) {
          Widget* picked = getManager()->pick(msg->mouse.x, msg->mouse.y);

          // If one of these conditions are accomplished we have to
          // close all menus (back to menu-bar or close the popuped
          // menubox), this is the place where we control if...
          if (picked == NULL ||         // If the button was clicked nowhere
              picked == this ||         // If the button was clicked in this menubox
              // The picked widget isn't menu-related
              (picked->type != kMenuBoxWidget &&
               picked->type != kMenuBarWidget &&
               picked->type != kMenuItemWidget) ||
              // The picked widget (that is menu-related) isn't from
              // the same tree of menus
              (get_base_menubox(picked) != this)) {

            // The user click outside all the menu-box/menu-items, close all
            menu->closeAll();
            return true;
          }
        }

        // Get the widget below the mouse cursor
        Widget* picked = menu->pick(msg->mouse.x, msg->mouse.y);
        if (picked) {
          if ((picked->type == kMenuItemWidget) &&
              !(picked->flags & JI_DISABLED)) {

            // If the picked menu-item is not highlighted...
            if (!static_cast<MenuItem*>(picked)->isHighlighted()) {
              // In menu-bar always open the submenu, in other popup-menus
              // open the submenu only if the user does click
              bool open_submenu =
                (this->type == kMenuBarWidget) ||
                (msg->type == kMouseDownMessage);

              menu->highlightItem(static_cast<MenuItem*>(picked), false, open_submenu, false);
            }
            // If the user pressed in a highlighted menu-item (maybe
            // the user was waiting for the timer to open the
            // submenu...)
            else if (msg->type == kMouseDownMessage &&
                     static_cast<MenuItem*>(picked)->hasSubmenu()) {
              static_cast<MenuItem*>(picked)->stopTimer();

              // If the submenu is closed, open it
              if (!static_cast<MenuItem*>(picked)->hasSubmenuOpened())
                static_cast<MenuItem*>(picked)->openSubmenu(false);
            }
          }
          else if (!get_base(this)->was_clicked) {
            menu->unhighlightItem();
          }
        }
      }
      break;

    case kMouseLeaveMessage:
      if (menu) {
        if (get_base(this)->is_processing)
          break;

        MenuItem* highlight = menu->getHighlightedItem();
        if (highlight && !highlight->hasSubmenuOpened())
          menu->unhighlightItem();
      }
      break;

    case kMouseUpMessage:
      if (menu) {
        if (get_base(this)->is_processing)
          break;

        // The item is highlighted and not opened (and the timer to open the submenu is stopped)
        MenuItem* highlight = menu->getHighlightedItem();
        if (highlight &&
            !highlight->hasSubmenuOpened() &&
            highlight->m_submenu_timer == NULL) {
          menu->closeAll();
          highlight->executeClick();
        }
      }
      break;

    case kKeyDownMessage:
      if (menu) {
        MenuItem* selected;

        if (get_base(this)->is_processing)
          break;

        get_base(this)->was_clicked = false;

        // Check for ALT+some underlined letter
        if (((this->type == kMenuBoxWidget) && (msg->any.shifts == 0 || // Inside menu-boxes we can use letters without Alt modifier pressed
                                                msg->any.shifts == KB_ALT_FLAG)) ||
            ((this->type == kMenuBarWidget) && (msg->any.shifts == KB_ALT_FLAG))) {
          selected = check_for_letter(menu, scancode_to_ascii(msg->key.scancode));
          if (selected) {
            menu->highlightItem(selected, true, true, true);
            return true;
          }
        }

        // Highlight movement with keyboard
        if (this->hasFocus()) {
          MenuItem* highlight = menu->getHighlightedItem();
          MenuItem* child_with_submenu_opened = NULL;
          bool used = false;

          // Search a child with highlight or the submenu opened
          UI_FOREACH_WIDGET(menu->getChildren(), it) {
            Widget* child = *it;
            if (child->type != kMenuItemWidget)
              continue;

            if (static_cast<MenuItem*>(child)->hasSubmenuOpened())
              child_with_submenu_opened = static_cast<MenuItem*>(child);
          }

          if (!highlight && child_with_submenu_opened)
            highlight = child_with_submenu_opened;

          switch (msg->key.scancode) {

            case KEY_ESC:
              // In menu-bar
              if (this->type == kMenuBarWidget) {
                if (highlight) {
                  cancelMenuLoop();
                  used = true;
                }
              }
              // In menu-boxes
              else {
                if (child_with_submenu_opened) {
                  child_with_submenu_opened->closeSubmenu(true);
                  used = true;
                }
                // Go to parent
                else if (menu->m_menuitem) {
                  // Just retrogress one parent-level
                  menu->m_menuitem->closeSubmenu(true);
                  used = true;
                }
              }
              break;

            case KEY_UP:
              // In menu-bar
              if (this->type == kMenuBarWidget) {
                if (child_with_submenu_opened)
                  child_with_submenu_opened->closeSubmenu(true);
              }
              // In menu-boxes
              else {
                // Go to previous
                highlight = find_previtem(menu, highlight);
                menu->highlightItem(highlight, false, false, false);
              }
              used = true;
              break;

            case KEY_DOWN:
              // In menu-bar
              if (this->type == kMenuBarWidget) {
                // Select the active menu
                menu->highlightItem(highlight, true, true, true);
              }
              // In menu-boxes
              else {
                // Go to next
                highlight = find_nextitem(menu, highlight);
                menu->highlightItem(highlight, false, false, false);
              }
              used = true;
              break;

            case KEY_LEFT:
              // In menu-bar
              if (this->type == kMenuBarWidget) {
                // Go to previous
                highlight = find_previtem(menu, highlight);
                menu->highlightItem(highlight, false, false, false);
              }
              // In menu-boxes
              else {
                // Go to parent
                if (menu->m_menuitem) {
                  Widget* parent = menu->m_menuitem->getParent()->getParent();

                  // Go to the previous item in the parent

                  // If the parent is the menu-bar
                  if (parent->type == kMenuBarWidget) {
                    menu = static_cast<MenuBar*>(parent)->getMenu();
                    MenuItem* menuitem = find_previtem(menu, menu->getHighlightedItem());

                    // Go to previous item in the parent
                    menu->highlightItem(menuitem, false, true, true);
                  }
                  // If the parent isn't the menu-bar
                  else {
                    // Just retrogress one parent-level
                    menu->m_menuitem->closeSubmenu(true);
                  }
                }
              }
              used = true;
              break;

            case KEY_RIGHT:
              // In menu-bar
              if (this->type == kMenuBarWidget) {
                // Go to next
                highlight = find_nextitem(menu, highlight);
                menu->highlightItem(highlight, false, false, false);
              }
              // In menu-boxes
              else {
                // Enter in sub-menu
                if (highlight && highlight->hasSubmenu()) {
                  menu->highlightItem(highlight, true, true, true);
                }
                // Go to parent
                else if (menu->m_menuitem) {
                  // Get the root menu
                  MenuBox* root = get_base_menubox(this);
                  menu = root->getMenu();

                  // Go to the next item in the root
                  MenuItem* menuitem = find_nextitem(menu, menu->getHighlightedItem());

                  // Open the sub-menu
                  menu->highlightItem(menuitem, false, true, true);
                }
              }
              used = true;
              break;

            case KEY_ENTER:
            case KEY_ENTER_PAD:
              if (highlight)
                menu->highlightItem(highlight, true, true, true);
              used = true;
              break;
          }

          // Return true if we've already consumed the key.
          if (used) {
            return true;
          }
          // If the user presses the ALT key we close everything.
          else if (msg->key.scancode == KEY_ALT) {
            cancelMenuLoop();
          }
        }
      }
      break;

    default:
      if (msg->type == kClosePopupMessage) {
        this->getManager()->_closeWindow(this->getRoot(), true);
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void MenuBox::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  if (Menu* menu = getMenu())
    menu->setBounds(getChildrenBounds());
}

void MenuBox::onPreferredSize(PreferredSizeEvent& ev)
{
  Size size(0, 0);

  if (Menu* menu = getMenu())
    size = menu->getPreferredSize();

  size.w += this->border_width.l + this->border_width.r;
  size.h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(size);
}

bool MenuItem::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case kMouseEnterMessage:
      // TODO theme specific!!
      invalidate();

      // When a menu item receives the mouse, start a timer to open the submenu...
      if (isEnabled() && hasSubmenu()) {
        // Start the timer to open the submenu...
        startTimer();
      }
      break;

    case kMouseLeaveMessage:
      // TODO theme specific!!
      invalidate();

      // Stop timer to open the popup
      if (m_submenu_timer)
        m_submenu_timer.reset();
      break;

    default:
      if (msg->type == kOpenMenuItemMessage) {
        MenuBaseData* base = get_base(this);
        bool select_first = msg->user.a ? true: false;

        ASSERT(base != NULL);
        ASSERT(base->is_processing);
        ASSERT(hasSubmenu());

        Rect old_pos = getParent()->getParent()->getBounds();

        MenuBox* menubox = new MenuBox();
        m_submenu_menubox = menubox;
        menubox->setMenu(m_submenu);

        // New window and new menu-box
        Window* window = new CustomizedWindowForMenuBox(menubox);

        // Menubox position
        Rect pos = window->getBounds();

        if (this->getParent()->getParent()->type == kMenuBarWidget) {
          pos.x = MID(0, this->rc->x1, JI_SCREEN_W-pos.w);
          pos.y = MID(0, this->rc->y2, JI_SCREEN_H-pos.h);
        }
        else {
          int x_left = this->rc->x1-pos.w;
          int x_right = this->rc->x2;
          int x, y = this->rc->y1;
          Rect r1(0, 0, pos.w, pos.h), r2(0, 0, pos.w, pos.h);

          r1.x = x_left = MID(0, x_left, JI_SCREEN_W-pos.w);
          r2.x = x_right = MID(0, x_right, JI_SCREEN_W-pos.w);
          r1.y = r2.y = y = MID(0, y, JI_SCREEN_H-pos.h);

          // Calculate both intersections
          gfx::Rect s1 = r1.createIntersect(old_pos);
          gfx::Rect s2 = r2.createIntersect(old_pos);

          if (s2.isEmpty())
            x = x_right;        // Use the right because there aren't intersection with it
          else if (s1.isEmpty())
            x = x_left;         // Use the left because there are not intersection
          else if (r2.w*r2.h <= r1.w*r1.h)
            x = x_right;                // Use the right because there are less intersection area
          else
            x = x_left;         // Use the left because there are less intersection area

          pos.x = x;
          pos.y = y;
        }

        window->positionWindow(pos.x, pos.y);

        // Set the focus to the new menubox
        menubox->setFocusMagnet(true);

        // Setup the highlight of the new menubox
        if (select_first) {
          // Select the first child
          MenuItem* first_child = NULL;

          UI_FOREACH_WIDGET(m_submenu->getChildren(), it) {
            Widget* child = *it;

            if (child->type != kMenuItemWidget)
              continue;

            if (child->isEnabled()) {
              first_child = static_cast<MenuItem*>(child);
              break;
            }
          }

          if (first_child)
            m_submenu->highlightItem(first_child, false, false, false);
          else
            m_submenu->unhighlightItem();
        }
        else
          m_submenu->unhighlightItem();

        // Run in background
        window->openWindow();

        base->is_processing = false;

        return true;
      }
      else if (msg->type == kCloseMenuItemMessage) {
        MenuBaseData* base = get_base(this);
        Window* window;
        bool last_of_close_chain = (msg->user.a ? true: false);

        ASSERT(base != NULL);
        ASSERT(base->is_processing);

        MenuBox* menubox = m_submenu_menubox;
        m_submenu_menubox = NULL;

        ASSERT(menubox != NULL);

        window = (Window*)menubox->getParent();
        ASSERT(window && window->type == kWindowWidget);

        // Fetch the "menu" to avoid destroy it with 'delete'.
        menubox->setMenu(NULL);

        // Destroy the window
        window->closeWindow(NULL);

        // Set the focus to this menu-box of this menu-item
        if (base->close_all)
          getManager()->freeFocus();
        else
          getManager()->setFocus(this->getParent()->getParent());

        // It is not necessary to delete this window because it's
        // automatically destroyed by the manager
        // ... delete window;

        if (last_of_close_chain) {
          base->close_all = false;
          base->is_processing = false;
        }

        // Stop timer to open the popup
        stopTimer();
        return true;
      }
      else if (msg->type == kExecuteMenuItemMessage) {
        onClick();
        return true;
      }
      break;

    case kTimerMessage:
      if (msg->timer.timer == m_submenu_timer.get()) {
        MenuBaseData* base = get_base(this);

        ASSERT(hasSubmenu());

        // Stop timer to open the popup
        stopTimer();

        // If the submenu is closed, and we are not processing messages, open it
        if (m_submenu_menubox == NULL && !base->is_processing)
          openSubmenu(false);
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void MenuItem::onPaint(PaintEvent& ev)
{
  getTheme()->paintMenuItem(ev);
}

void MenuItem::onClick()
{
  // Fire new Click() signal.
  Click();
}

void MenuItem::onPreferredSize(PreferredSizeEvent& ev)
{
  Size size(0, 0);
  int bar = (this->getParent()->getParent()->type == kMenuBarWidget);

  if (this->hasText()) {
    size.w =
      + this->border_width.l
      + jwidget_get_text_length(this)
      + ((bar) ? this->child_spacing/4: this->child_spacing)
      + this->border_width.r;

    size.h =
      + this->border_width.t
      + jwidget_get_text_height(this)
      + this->border_width.b;

    if (m_accel && !m_accel->isEmpty()) {
      size.w += ji_font_text_len(this->getFont(), m_accel->toString().c_str());
    }
  }

  ev.setPreferredSize(size);
}

// Climbs the hierarchy of menus to get the most-top menubox.
static MenuBox* get_base_menubox(Widget* widget)
{
  while (widget != NULL) {
    ASSERT_VALID_WIDGET(widget);

    // We are in a menubox
    if (widget->type == kMenuBoxWidget || widget->type == kMenuBarWidget) {
      if (static_cast<MenuBox*>(widget)->getBase()) {
        return static_cast<MenuBox*>(widget);
      }
      else {
        Menu* menu = static_cast<MenuBox*>(widget)->getMenu();

        ASSERT(menu != NULL);
        ASSERT(menu->getOwnerMenuItem() != NULL);

        widget = menu->getOwnerMenuItem();
      }
    }
    // We are in a menuitem
    else {
      ASSERT(widget->type == kMenuItemWidget);
      ASSERT(widget->getParent() != NULL);
      ASSERT(widget->getParent()->type == kMenuWidget);

      widget = widget->getParent()->getParent();
    }
  }

  ASSERT(false);
  return NULL;
}

static MenuBaseData* get_base(Widget* widget)
{
  MenuBox* menubox = get_base_menubox(widget);
  return menubox->getBase();
}

MenuItem* Menu::getHighlightedItem()
{
  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    if (child->type != kMenuItemWidget)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);
    if (menuitem->isHighlighted())
      return menuitem;
  }
  return NULL;
}

void Menu::highlightItem(MenuItem* menuitem, bool click, bool open_submenu, bool select_first_child)
{
  // Find the menuitem with the highlight
  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    if (child->type != kMenuItemWidget)
      continue;

    if (child != menuitem) {
      // Is it?
      if (static_cast<MenuItem*>(child)->isHighlighted()) {
        static_cast<MenuItem*>(child)->setHighlighted(false);
        child->invalidate();
      }
    }
  }

  if (menuitem) {
    if (!menuitem->isHighlighted()) {
      menuitem->setHighlighted(true);
      menuitem->invalidate();
    }

    // Highlight parents
    if (getOwnerMenuItem() != NULL) {
      static_cast<Menu*>(getOwnerMenuItem()->getParent())
        ->highlightItem(getOwnerMenuItem(), false, false, false);
    }

    // Open submenu of the menitem
    if (menuitem->hasSubmenu()) {
      if (open_submenu) {
        // If the submenu is closed, open it
        if (!menuitem->hasSubmenuOpened())
          menuitem->openSubmenu(select_first_child);

        // The mouse was clicked
        get_base(menuitem)->was_clicked = true;
      }
    }
    // Execute menuitem action
    else if (click) {
      closeAll();
      menuitem->executeClick();
    }
  }
}

void Menu::unhighlightItem()
{
  highlightItem(NULL, false, false, false);
}

void MenuItem::openSubmenu(bool select_first)
{
  Widget* menu;
  Message* msg;

  ASSERT(hasSubmenu());

  menu = this->getParent();

  // The menu item is already opened?
  ASSERT(m_submenu_menubox == NULL);

  ASSERT_VALID_WIDGET(menu);

  // Close all siblings of 'menuitem'
  if (menu->getParent()) {
    UI_FOREACH_WIDGET(menu->getChildren(), it) {
      Widget* child = *it;
      if (child->type != kMenuItemWidget)
        continue;

      MenuItem* childMenuItem = static_cast<MenuItem*>(child);
      if (childMenuItem != this && childMenuItem->hasSubmenuOpened()) {
        childMenuItem->closeSubmenu(false);
      }
    }
  }

  msg = jmessage_new(kOpenMenuItemMessage);
  msg->user.a = select_first;
  jmessage_add_dest(msg, this);
  Manager::getDefault()->enqueueMessage(msg);

  // Get the 'base'
  MenuBaseData* base = get_base(this);
  ASSERT(base != NULL);
  ASSERT(base->is_processing == false);

  // Reset flags
  base->close_all = false;
  base->is_processing = true;

  // We need to add a filter of the kMouseDownMessage to intercept
  // clicks outside the menu (and close all the hierarchy in that
  // case); the widget to intercept messages is the base menu-bar or
  // popuped menu-box
  if (!base->is_filtering) {
    base->is_filtering = true;
    Manager::getDefault()->addMessageFilter(kMouseDownMessage, get_base_menubox(this));
  }
}

void MenuItem::closeSubmenu(bool last_of_close_chain)
{
  Widget* menu;
  Message* msg;
  MenuBaseData* base;

  ASSERT(m_submenu_menubox != NULL);

  // First: recursively close the children
  menu = m_submenu_menubox->getMenu();
  ASSERT(menu != NULL);

  UI_FOREACH_WIDGET(menu->getChildren(), it) {
    Widget* child = *it;
    if (child->type != kMenuItemWidget)
      continue;

    if (static_cast<MenuItem*>(child)->hasSubmenuOpened())
      static_cast<MenuItem*>(child)->closeSubmenu(false);
  }

  // Second: now we can close the 'menuitem'
  msg = jmessage_new(kCloseMenuItemMessage);
  msg->user.a = last_of_close_chain;
  jmessage_add_dest(msg, this);
  Manager::getDefault()->enqueueMessage(msg);

  // If this is the last message of the chain, here we have the
  // responsibility to set is_processing flag to true.
  if (last_of_close_chain) {
    // Get the 'base'
    base = get_base(this);
    ASSERT(base != NULL);
    ASSERT(base->is_processing == false);

    // Start processing
    base->is_processing = true;
  }
}

void MenuItem::startTimer()
{
  if (m_submenu_timer == NULL)
    m_submenu_timer.reset(new Timer(kTimeoutToOpenSubmenu, this));

  m_submenu_timer->start();
}

void MenuItem::stopTimer()
{
  // Stop timer to open the popup
  if (m_submenu_timer)
    m_submenu_timer.reset();
}

void Menu::closeAll()
{
  Menu* menu = this;
  MenuItem* menuitem = NULL;
  while (menu->m_menuitem) {
    menuitem = menu->m_menuitem;
    menu = static_cast<Menu*>(menuitem->getParent());
  }

  MenuBox* base_menubox = get_base_menubox(menu->getParent());
  MenuBaseData* base = base_menubox->getBase();
  base->close_all = true;
  base->was_clicked = false;
  if (base->is_filtering) {
    base->is_filtering = false;
    Manager::getDefault()->removeMessageFilter(kMouseDownMessage, base_menubox);
  }

  menu->unhighlightItem();

  if (menuitem != NULL) {
    if (menuitem->hasSubmenuOpened())
      menuitem->closeSubmenu(true);
  }
  else {
    UI_FOREACH_WIDGET(menu->getChildren(), it) {
      Widget* child = *it;
      if (child->type != kMenuItemWidget)
        continue;

      menuitem = static_cast<MenuItem*>(child);
      if (menuitem->hasSubmenuOpened())
        menuitem->closeSubmenu(true);
    }
  }

  // For popuped menus
  if (base_menubox->type == kMenuBoxWidget)
    base_menubox->closePopup();
}

void MenuBox::closePopup()
{
  Message* msg = jmessage_new(kClosePopupMessage);
  jmessage_add_dest(msg, this);
  Manager::getDefault()->enqueueMessage(msg);
}

void MenuBox::cancelMenuLoop()
{
  Menu* menu = getMenu();
  if (menu) {
    // Do not close the popup menus if we're already processing
    // open/close popup messages.
    if (get_base(this)->is_processing)
      return;

    menu->closeAll();

    // Lost focus
    Manager::getDefault()->freeFocus();
  }
}

void MenuItem::executeClick()
{
  // Send the message
  Message* msg = jmessage_new(kExecuteMenuItemMessage);
  jmessage_add_dest(msg, this);
  Manager::getDefault()->enqueueMessage(msg);
}

static MenuItem* check_for_letter(Menu* menu, int ascii)
{
  UI_FOREACH_WIDGET(menu->getChildren(), it) {
    Widget* child = *it;
    if (child->type != kMenuItemWidget)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);
    int mnemonic = menuitem->getMnemonicChar();
    if (mnemonic > 0 && mnemonic == tolower(ascii))
      return menuitem;
  }
  return NULL;
}

static MenuItem* check_for_accel(Menu* menu, Message* msg)
{
  UI_FOREACH_WIDGET(menu->getChildren(), it) {
    Widget* child = *it;
    if (child->type != kMenuItemWidget)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);

    if (menuitem->getSubmenu()) {
      if ((menuitem = check_for_accel(menuitem->getSubmenu(), msg)))
        return menuitem;
    }
    else if (menuitem->getAccel()) {
      if ((menuitem->isEnabled()) &&
          (menuitem->getAccel()->check(msg->any.shifts,
                                       msg->key.ascii,
                                       msg->key.scancode)))
        return menuitem;
    }
  }
  return NULL;
}

// Finds the next item of `menuitem', if `menuitem' is NULL searchs
// from the first item in `menu'
static MenuItem* find_nextitem(Menu* menu, MenuItem* menuitem)
{
  WidgetsList::const_iterator begin = menu->getChildren().begin();
  WidgetsList::const_iterator it, end = menu->getChildren().end();

  if (menuitem) {
    it = std::find(begin, end, menuitem);
    if (it != end)
      ++it;
  }
  else
    it = begin;

  for (; it != end; ++it) {
    Widget* nextitem = *it;
    if ((nextitem->type == kMenuItemWidget) && nextitem->isEnabled())
      return static_cast<MenuItem*>(nextitem);
  }

  if (menuitem)
    return find_nextitem(menu, NULL);
  else
   return NULL;
}

static MenuItem* find_previtem(Menu* menu, MenuItem* menuitem)
{
  WidgetsList::const_reverse_iterator begin = menu->getChildren().rbegin();
  WidgetsList::const_reverse_iterator it, end = menu->getChildren().rend();

  if (menuitem) {
    it = std::find(begin, end, menuitem);
    if (it != end)
      ++it;
  }
  else
    it = begin;

  for (; it != end; ++it) {
    Widget* nextitem = *it;
    if ((nextitem->type == kMenuItemWidget) && nextitem->isEnabled())
      return static_cast<MenuItem*>(nextitem);
  }

  if (menuitem)
    return find_previtem(menu, NULL);
  else
    return NULL;
}

} // namespace ui
