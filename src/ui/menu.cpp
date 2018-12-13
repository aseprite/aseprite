// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/menu.h"

#include "gfx/size.h"
#include "os/font.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <cctype>
#include <memory>

static const int kTimeoutToOpenSubmenu = 250;

namespace ui {

using namespace gfx;

//////////////////////////////////////////////////////////////////////
// Internal messages: to move between menus

RegisterMessage kOpenMenuItemMessage;
RegisterMessage kCloseMenuItemMessage;
RegisterMessage kClosePopupMessage;
RegisterMessage kExecuteMenuItemMessage;

class OpenMenuItemMessage : public Message {
public:
  OpenMenuItemMessage(bool select_first) :
    Message(kOpenMenuItemMessage),
    m_select_first(select_first) {
  }

  // If this value is true, it means that after opening the menu, we
  // have to select the first item (i.e. highlighting it).
  bool select_first() const { return m_select_first; }

private:
  bool m_select_first;
};

class CloseMenuItemMessage : public Message {
public:
  CloseMenuItemMessage(bool last_of_close_chain) :
    Message(kCloseMenuItemMessage),
    m_last_of_close_chain(last_of_close_chain) {
  }

  // This fields is used to indicate the end of a sequence of
  // kOpenMenuItemMessage and kCloseMenuItemMessage messages. If it is true
  // the message is the last one of the chain, which means that no
  // more kOpenMenuItemMessage or kCloseMenuItemMessage messages are in the queue.
  bool last_of_close_chain() const { return m_last_of_close_chain; }

private:
  bool m_last_of_close_chain;
};

// Data for the main jmenubar or the first popuped-jmenubox
struct MenuBaseData {
  // True when the menu-items must be opened with the cursor movement
  bool was_clicked;

  // True when there's kOpen/CloseMenuItemMessage messages in queue, to
  // avoid start processing another menuitem-request when we're
  // already working in one
  bool is_processing;

  // True when the kMouseDownMessage is being filtered
  bool is_filtering;

  bool close_all;

  MenuBaseData() {
    was_clicked = false;
    is_filtering = false;
    is_processing = false;
    close_all = false;
  }

};

static MenuBox* get_base_menubox(Widget* widget);
static MenuBaseData* get_base(Widget* widget);

static MenuItem* check_for_letter(Menu* menu, const KeyMessage* keymsg);

static MenuItem* find_nextitem(Menu* menu, MenuItem* menuitem);
static MenuItem* find_previtem(Menu* menu, MenuItem* menuitem);

//////////////////////////////////////////////////////////////////////
// Menu

Menu::Menu()
  : Widget(kMenuWidget)
  , m_menuitem(NULL)
{
  enableFlags(IGNORE_MOUSE);
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

//////////////////////////////////////////////////////////////////////
// MenuBox

MenuBox::MenuBox(WidgetType type)
 : Widget(type)
 , m_base(NULL)
{
  this->setFocusStop(true);
  initTheme();
}

MenuBox::~MenuBox()
{
  stopFilteringMouseDown();
  delete m_base;
}

//////////////////////////////////////////////////////////////////////
// MenuBar

bool MenuBar::m_expandOnMouseover = false;

MenuBar::MenuBar()
  : MenuBox(kMenuBarWidget)
{
  createBase();
}

// static
bool MenuBar::expandOnMouseover()
{
  return m_expandOnMouseover;
}

// static
void MenuBar::setExpandOnMouseover(bool state)
{
  m_expandOnMouseover = state;
}

//////////////////////////////////////////////////////////////////////
// MenuItem

MenuItem::MenuItem(const std::string& text)
  : Widget(kMenuItemWidget)
{
  m_highlighted = false;
  m_submenu = NULL;
  m_submenu_menubox = NULL;

  setText(text);
  initTheme();
}

MenuItem::~MenuItem()
{
  delete m_submenu;
}

Menu* MenuBox::getMenu()
{
  if (children().empty())
    return nullptr;
  else
    return static_cast<Menu*>(children().front());
}

MenuBaseData* MenuBox::createBase()
{
  delete m_base;
  m_base = new MenuBaseData;
  return m_base;
}

Menu* MenuItem::getSubmenu()
{
  return m_submenu;
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
  return (m_submenu && !m_submenu->children().empty());
}

void Menu::showPopup(const gfx::Point& pos)
{
  // Generally, when we call showPopup() the menu shouldn't contain a
  // parent menu-box, because we're filtering kMouseDownMessage to
  // close the popup automatically when we click outside the menubox.
  // Anyway there is one specific case were a clicked widget might
  // call showPopup() when it's clicked the first time, and a second
  // click could generate a kDoubleClickMessage which is then
  // converted to kMouseDownMessage to finally call showPopup() again.
  // In this case, the menu is already in a menubox.
  if (parent()) {
    static_cast<MenuBox*>(parent())->cancelMenuLoop();
    return;
  }

  // New window and new menu-box
  std::unique_ptr<Window> window(new Window(Window::WithoutTitleBar));
  MenuBox* menubox = new MenuBox();
  MenuBaseData* base = menubox->createBase();
  base->was_clicked = true;
  window->setMoveable(false);   // Can't move the window

  // Set children
  menubox->setMenu(this);
  menubox->startFilteringMouseDown();
  window->addChild(menubox);

  window->remapWindow();

  // Menubox position
  window->positionWindow(
    MID(0, pos.x, ui::display_w() - window->bounds().w),
    MID(0, pos.y, ui::display_h() - window->bounds().h));

  // Set the focus to the new menubox
  Manager* manager = Manager::getDefault();
  manager->setFocus(menubox);
  menubox->setFocusMagnet(true);

  // Open the window
  window->openWindowInForeground();

  // Free the keyboard focus if it's in the menu popup, in other case
  // it means that the user set the focus to other specific widget
  // before we closed the popup.
  Widget* focus = manager->getFocus();
  if (focus && focus->window() == window.get())
    focus->releaseFocus();

  // Fetch the "menu" so it isn't destroyed
  menubox->setMenu(nullptr);
  menubox->stopFilteringMouseDown();

}

Widget* Menu::findItemById(const char* id)
{
  Widget* result = findChild(id);
  if (result)
    return result;
  for (auto child : children()) {
    if (child->type() == kMenuItemWidget) {
      result = static_cast<MenuItem*>(child)
        ->getSubmenu()->findItemById(id);
      if (result)
        return result;
    }
  }
  return nullptr;
}

void Menu::onPaint(PaintEvent& ev)
{
  theme()->paintMenu(ev);
}

void Menu::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  Rect cpos = childrenBounds();
  bool isBar = (parent()->type() == kMenuBarWidget);

  for (auto child : children()) {
    Size reqSize = child->sizeHint();

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

void Menu::onSizeHint(SizeHintEvent& ev)
{
  Size size(0, 0);
  Size reqSize;

  UI_FOREACH_WIDGET_WITH_END(children(), it, end) {
    reqSize = (*it)->sizeHint();

    if (parent() &&
        parent()->type() == kMenuBarWidget) {
      size.w += reqSize.w + ((it+1 != end) ? childSpacing(): 0);
      size.h = MAX(size.h, reqSize.h);
    }
    else {
      size.w = MAX(size.w, reqSize.w);
      size.h += reqSize.h + ((it+1 != end) ? childSpacing(): 0);
    }
  }

  size.w += border().width();
  size.h += border().height();

  ev.setSizeHint(size);
}

bool MenuBox::onProcessMessage(Message* msg)
{
  Menu* menu = MenuBox::getMenu();

  switch (msg->type()) {

    case kMouseMoveMessage:
      if (!get_base(this)->was_clicked)
        break;

      // Fall through

    case kMouseDownMessage:
    case kDoubleClickMessage:
      if (menu) {
        ASSERT(menu->parent() == this);

        if (get_base(this)->is_processing)
          break;

        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();

        // Here we catch the filtered messages (menu-bar or the
        // popuped menu-box) to detect if the user press outside of
        // the widget
        if (msg->type() == kMouseDownMessage && m_base != NULL) {
          Widget* picked = manager()->pick(mousePos);

          // If one of these conditions are accomplished we have to
          // close all menus (back to menu-bar or close the popuped
          // menubox), this is the place where we control if...
          if (picked == NULL ||         // If the button was clicked nowhere
              picked == this ||         // If the button was clicked in this menubox
              // The picked widget isn't from the same tree of menus
              (get_base_menubox(picked) != this ||
               (this->type() == kMenuBarWidget &&
                picked->type() == kMenuWidget))) {

            // The user click outside all the menu-box/menu-items, close all
            menu->closeAll();
            return true;
          }
        }

        // Get the widget below the mouse cursor
        Widget* picked = menu->pick(mousePos);
        if (picked) {
          if ((picked->type() == kMenuItemWidget) &&
              !(picked->hasFlags(DISABLED))) {
            MenuItem* pickedItem = static_cast<MenuItem*>(picked);

            // If the picked menu-item is not highlighted...
            if (!pickedItem->isHighlighted()) {
              // In menu-bar always open the submenu, in other popup-menus
              // open the submenu only if the user does click
              bool open_submenu =
                (this->type() == kMenuBarWidget) ||
                (msg->type() == kMouseDownMessage);

              menu->highlightItem(pickedItem, false, open_submenu, false);
            }
            // If the user pressed in a highlighted menu-item (maybe
            // the user was waiting for the timer to open the
            // submenu...)
            else if (msg->type() == kMouseDownMessage &&
                     pickedItem->hasSubmenu()) {
              pickedItem->stopTimer();

              // If the submenu is closed, open it
              if (!pickedItem->hasSubmenuOpened())
                pickedItem->openSubmenu(false);
              else if (pickedItem->inBar()) {
                pickedItem->getSubmenu()->closeAll();

                // Set this flag to false so the submenu is not open
                // again on kMouseMoveMessage.
                get_base(this)->was_clicked = false;
              }
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
        if (((this->type() == kMenuBoxWidget) && (msg->modifiers() == kKeyNoneModifier || // <-- Inside menu-boxes we can use letters without Alt modifier pressed
                                                  msg->modifiers() == kKeyAltModifier)) ||
            ((this->type() == kMenuBarWidget) && (msg->modifiers() == kKeyAltModifier))) {
          auto keymsg = static_cast<KeyMessage*>(msg);
          selected = check_for_letter(menu, keymsg);
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
          for (auto child : menu->children()) {
            if (child->type() != kMenuItemWidget)
              continue;

            if (static_cast<MenuItem*>(child)->hasSubmenuOpened())
              child_with_submenu_opened = static_cast<MenuItem*>(child);
          }

          if (!highlight && child_with_submenu_opened)
            highlight = child_with_submenu_opened;

          switch (static_cast<KeyMessage*>(msg)->scancode()) {

            case kKeyEsc:
              // In menu-bar
              if (this->type() == kMenuBarWidget) {
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

            case kKeyUp:
              // In menu-bar
              if (this->type() == kMenuBarWidget) {
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

            case kKeyDown:
              // In menu-bar
              if (this->type() == kMenuBarWidget) {
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

            case kKeyLeft:
              // In menu-bar
              if (this->type() == kMenuBarWidget) {
                // Go to previous
                highlight = find_previtem(menu, highlight);
                menu->highlightItem(highlight, false, false, false);
              }
              // In menu-boxes
              else {
                // Go to parent
                if (menu->m_menuitem) {
                  Widget* parent = menu->m_menuitem->parent()->parent();

                  // Go to the previous item in the parent

                  // If the parent is the menu-bar
                  if (parent->type() == kMenuBarWidget) {
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

            case kKeyRight:
              // In menu-bar
              if (this->type() == kMenuBarWidget) {
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

            case kKeyEnter:
            case kKeyEnterPad:
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
          else if (static_cast<KeyMessage*>(msg)->scancode() == kKeyAlt) {
            cancelMenuLoop();
          }
        }
      }
      break;

    default:
      if (msg->type() == kClosePopupMessage) {
        window()->closeWindow(nullptr);
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void MenuBox::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  if (Menu* menu = getMenu())
    menu->setBounds(childrenBounds());
}

void MenuBox::onSizeHint(SizeHintEvent& ev)
{
  Size size(0, 0);

  if (Menu* menu = getMenu())
    size = menu->sizeHint();

  size.w += border().width();
  size.h += border().height();

  ev.setSizeHint(size);
}

bool MenuItem::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseEnterMessage:
      // TODO theme specific!!
      invalidate();

      // When a menu item receives the mouse, start a timer to open the submenu...
      if (isEnabled() && hasSubmenu()) {
        // Start the timer to open the submenu...
        if (!inBar() || MenuBar::expandOnMouseover())
          startTimer();
      }
      break;

    case kMouseLeaveMessage:
      // Unhighlight this item if its submenu isn't opened
      if (isHighlighted() &&
          !m_submenu_menubox &&
          parent() &&
          parent()->type() == kMenuWidget) {
        static_cast<Menu*>(parent())->unhighlightItem();
      }

      // TODO theme specific!!
      invalidate();

      // Stop timer to open the popup
      if (m_submenu_timer)
        m_submenu_timer.reset();
      break;

    default:
      if (msg->type() == kOpenMessage) {
        validateItem();
      }
      else if (msg->type() == kOpenMenuItemMessage) {
        validateItem();

        MenuBaseData* base = get_base(this);
        bool select_first = static_cast<OpenMenuItemMessage*>(msg)->select_first();

        ASSERT(base != NULL);
        ASSERT(base->is_processing);
        ASSERT(hasSubmenu());

        Rect old_pos = window()->bounds();
        old_pos.w -= 1*guiscale();

        MenuBox* menubox = new MenuBox();
        m_submenu_menubox = menubox;
        menubox->setMenu(m_submenu);

        // New window and new menu-box
        Window* window = new MenuBoxWindow(menubox);

        // Menubox position
        Rect pos = window->bounds();

        if (inBar()) {
          pos.x = MID(0, bounds().x, ui::display_w()-pos.w);
          pos.y = MAX(0, bounds().y2());
        }
        else {
          int x_left = old_pos.x - pos.w;
          int x_right = old_pos.x2();
          int x, y = bounds().y-3*guiscale();
          Rect r1(0, 0, pos.w, pos.h), r2(0, 0, pos.w, pos.h);

          r1.x = x_left = MID(0, x_left, ui::display_w()-pos.w);
          r2.x = x_right = MID(0, x_right, ui::display_w()-pos.w);
          r1.y = r2.y = y = MID(0, y, ui::display_h()-pos.h);

          // Calculate both intersections
          gfx::Rect s1 = r1.createIntersection(old_pos);
          gfx::Rect s2 = r2.createIntersection(old_pos);

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

          for (auto child : m_submenu->children()) {
            if (child->type() != kMenuItemWidget)
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
      else if (msg->type() == kCloseMenuItemMessage) {
        bool last_of_close_chain = static_cast<CloseMenuItemMessage*>(msg)->last_of_close_chain();
        MenuBaseData* base = get_base(this);
        Window* window;

        ASSERT(base != NULL);
        ASSERT(base->is_processing);

        MenuBox* menubox = m_submenu_menubox;
        m_submenu_menubox = NULL;

        ASSERT(menubox != NULL);

        window = (Window*)menubox->parent();
        ASSERT(window && window->type() == kWindowWidget);

        // Fetch the "menu" to avoid destroy it with 'delete'.
        menubox->setMenu(NULL);

        // Destroy the window
        window->closeWindow(NULL);

        // Set the focus to this menu-box of this menu-item
        if (base->close_all)
          manager()->freeFocus();
        else
          manager()->setFocus(this->parent()->parent());

        // Do not call "delete window" here, because it
        // (MenuBoxWindow) will be deferDelete() on
        // kCloseMessage.

        if (last_of_close_chain) {
          base->close_all = false;
          base->is_processing = false;
        }

        // Stop timer to open the popup
        stopTimer();
        return true;
      }
      else if (msg->type() == kExecuteMenuItemMessage) {
        onClick();
        return true;
      }
      break;

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer() == m_submenu_timer.get()) {
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

void MenuItem::onInitTheme(InitThemeEvent& ev)
{
  if (m_submenu)
    m_submenu->initTheme();
  if (m_submenu_menubox)
    m_submenu_menubox->initTheme();
  Widget::onInitTheme(ev);
}

void MenuItem::onPaint(PaintEvent& ev)
{
  theme()->paintMenuItem(ev);
}

void MenuItem::onClick()
{
  // Fire new Click() signal.
  Click();
}

void MenuItem::onValidate()
{
  // Here the user can customize the automatic validation of the menu
  // item before it's shown.
}

void MenuItem::onSizeHint(SizeHintEvent& ev)
{
  Size size(0, 0);

  if (hasText()) {
    size.w =
      + textWidth()
      + (inBar() ? childSpacing()/4: childSpacing())
      + border().width();

    size.h =
      + textHeight()
      + border().height();
  }

  ev.setSizeHint(size);
}

// Climbs the hierarchy of menus to get the most-top menubox.
static MenuBox* get_base_menubox(Widget* widget)
{
  while (widget) {
    ASSERT_VALID_WIDGET(widget);

    // We are in a menubox
    if (widget->type() == kMenuBoxWidget ||
        widget->type() == kMenuBarWidget) {
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
    else {
      widget = widget->parent();
    }
  }
  return nullptr;
}

static MenuBaseData* get_base(Widget* widget)
{
  MenuBox* menubox = get_base_menubox(widget);
  ASSERT(menubox);
  if (menubox)
    return menubox->getBase();
  else
    return nullptr;
}

MenuItem* Menu::getHighlightedItem()
{
  for (auto child : children()) {
    if (child->type() != kMenuItemWidget)
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
  for (auto child : children()) {
    if (child->type() != kMenuItemWidget)
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
      static_cast<Menu*>(getOwnerMenuItem()->parent())
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

bool MenuItem::inBar()
{
  return
    (parent() &&
     parent()->parent() &&
     parent()->parent()->type() == kMenuBarWidget);
}

void MenuItem::openSubmenu(bool select_first)
{
  Widget* menu;
  Message* msg;

  ASSERT(hasSubmenu());

  menu = this->parent();

  // The menu item is already opened?
  ASSERT(m_submenu_menubox == NULL);

  ASSERT_VALID_WIDGET(menu);

  // Close all siblings of 'menuitem'
  if (menu->parent()) {
    for (auto child : menu->children()) {
      if (child->type() != kMenuItemWidget)
        continue;

      MenuItem* childMenuItem = static_cast<MenuItem*>(child);
      if (childMenuItem != this && childMenuItem->hasSubmenuOpened()) {
        childMenuItem->closeSubmenu(false);
      }
    }
  }

  msg = new OpenMenuItemMessage(select_first);
  msg->setRecipient(this);
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
  MenuBox* base_menubox = get_base_menubox(this);
  if (base_menubox)
    base_menubox->startFilteringMouseDown();
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

  for (auto child : menu->children()) {
    if (child->type() != kMenuItemWidget)
      continue;

    if (static_cast<MenuItem*>(child)->hasSubmenuOpened())
      static_cast<MenuItem*>(child)->closeSubmenu(false);
  }

  // Second: now we can close the 'menuitem'
  msg = new CloseMenuItemMessage(last_of_close_chain);
  msg->setRecipient(this);
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
    menu = static_cast<Menu*>(menuitem->parent());
  }

  MenuBox* base_menubox = get_base_menubox(menu->parent());
  ASSERT(base_menubox);
  if (!base_menubox)
    return;

  MenuBaseData* base = base_menubox->getBase();
  ASSERT(base);
  if (!base)
    return;

  base->close_all = true;
  base->was_clicked = false;
  base_menubox->stopFilteringMouseDown();

  menu->unhighlightItem();

  if (menuitem != NULL) {
    if (menuitem->hasSubmenuOpened())
      menuitem->closeSubmenu(true);
  }
  else {
    for (auto child : menu->children()) {
      if (child->type() != kMenuItemWidget)
        continue;

      menuitem = static_cast<MenuItem*>(child);
      if (menuitem->hasSubmenuOpened())
        menuitem->closeSubmenu(true);
    }
  }

  // For popuped menus
  if (base_menubox->type() == kMenuBoxWidget)
    base_menubox->closePopup();
}

void MenuBox::closePopup()
{
  Message* msg = new Message(kClosePopupMessage);
  msg->setRecipient(this);
  Manager::getDefault()->enqueueMessage(msg);
}

void MenuBox::startFilteringMouseDown()
{
  if (m_base && !m_base->is_filtering) {
    m_base->is_filtering = true;
    Manager::getDefault()->addMessageFilter(kMouseDownMessage, this);
    Manager::getDefault()->addMessageFilter(kDoubleClickMessage, this);
  }
}

void MenuBox::stopFilteringMouseDown()
{
  if (m_base && m_base->is_filtering) {
    m_base->is_filtering = false;
    Manager::getDefault()->removeMessageFilter(kMouseDownMessage, this);
    Manager::getDefault()->removeMessageFilter(kDoubleClickMessage, this);
  }
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
  Message* msg = new Message(kExecuteMenuItemMessage);
  msg->setRecipient(this);
  Manager::getDefault()->enqueueMessage(msg);
}

void MenuItem::validateItem()
{
  onValidate();
}

static MenuItem* check_for_letter(Menu* menu, const KeyMessage* keymsg)
{
  for (auto child : menu->children()) {
    if (child->type() != kMenuItemWidget)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);
    if (menuitem->isMnemonicPressed(keymsg))
      return menuitem;
  }
  return NULL;
}

// Finds the next item of `menuitem', if `menuitem' is NULL searchs
// from the first item in `menu'
static MenuItem* find_nextitem(Menu* menu, MenuItem* menuitem)
{
  WidgetsList::const_iterator begin = menu->children().begin();
  WidgetsList::const_iterator it, end = menu->children().end();

  if (menuitem) {
    it = std::find(begin, end, menuitem);
    if (it != end)
      ++it;
  }
  else
    it = begin;

  for (; it != end; ++it) {
    Widget* nextitem = *it;
    if ((nextitem->type() == kMenuItemWidget) && nextitem->isEnabled())
      return static_cast<MenuItem*>(nextitem);
  }

  if (menuitem)
    return find_nextitem(menu, NULL);
  else
   return NULL;
}

static MenuItem* find_previtem(Menu* menu, MenuItem* menuitem)
{
  WidgetsList::const_reverse_iterator begin = menu->children().rbegin();
  WidgetsList::const_reverse_iterator it, end = menu->children().rend();

  if (menuitem) {
    it = std::find(begin, end, menuitem);
    if (it != end)
      ++it;
  }
  else
    it = begin;

  for (; it != end; ++it) {
    Widget* nextitem = *it;
    if ((nextitem->type() == kMenuItemWidget) && nextitem->isEnabled())
      return static_cast<MenuItem*>(nextitem);
  }

  if (menuitem)
    return find_previtem(menu, NULL);
  else
    return NULL;
}

//////////////////////////////////////////////////////////////////////
// MenuBoxWindow

MenuBoxWindow::MenuBoxWindow(MenuBox* menubox)
  : Window(WithoutTitleBar, "")
{
  setMoveable(false); // Can't move the window
  addChild(menubox);
  remapWindow();
}

bool MenuBoxWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      // Delete this window automatically
      deferDelete();
      break;

  }
  return Window::onProcessMessage(msg);
}

} // namespace ui
