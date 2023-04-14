// Aseprite UI Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/menu.h"

#include "base/scoped_value.h"
#include "gfx/size.h"
#include "os/font.h"
#include "ui/display.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <algorithm>
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

static void choose_side(gfx::Rect& bounds,
                        const gfx::Rect& workarea,
                        const gfx::Rect& parentBounds)
{
  int scale = guiscale();
  if (get_multiple_displays())
    scale = Manager::getDefault()->display()->scale();

  int x_left = parentBounds.x - bounds.w + 1*scale;
  int x_right = parentBounds.x2() - 1*scale;
  int x, y = bounds.y;
  Rect r1(0, 0, bounds.w, bounds.h);
  Rect r2(0, 0, bounds.w, bounds.h);

  r1.x = x_left = std::clamp(x_left, workarea.x, std::max(workarea.x, workarea.x2()-bounds.w));
  r2.x = x_right = std::clamp(x_right, workarea.x, std::max(workarea.x, workarea.x2()-bounds.w));
  r1.y = r2.y = y = std::clamp(y, workarea.y, std::max(workarea.y, workarea.y2()-bounds.h));

  // Calculate both intersections
  const gfx::Rect s1 = r1.createIntersection(parentBounds);
  const gfx::Rect s2 = r2.createIntersection(parentBounds);

  if (s2.isEmpty())
    x = x_right;        // Use the right because there aren't intersection with it
  else if (s1.isEmpty())
    x = x_left;         // Use the left because there are not intersection
  else if (s2.w*s2.h <= s1.w*s1.h)
    x = x_right;        // Use the right because there are less intersection area
  else
    x = x_left;         // Use the left because there are less intersection area

  bounds.x = x;
  bounds.y = y;
}

static void add_scrollbars_if_needed(MenuBoxWindow* window,
                                     const gfx::Rect& workarea,
                                     gfx::Rect& bounds)
{
  gfx::Rect rc = bounds;

  if (rc.x < workarea.x) {
    rc.w -= (workarea.x - rc.x);
    rc.x = workarea.x;
  }
  if (rc.x2() > workarea.x2()) {
    rc.w = workarea.x2() - rc.x;
  }

  bool vscrollbarsAdded = false;
  if (rc.y < workarea.y) {
    rc.h -= (workarea.y - rc.y);
    rc.y = workarea.y;
    vscrollbarsAdded = true;
  }
  if (rc.y2() > workarea.y2()) {
    rc.h = workarea.y2() - rc.y;
    vscrollbarsAdded = true;
  }

  if (rc == bounds)
    return;

  MenuBox* menubox = window->menubox();
  View* view = new View;
  view->InitTheme.connect([view]{ view->noBorderNoChildSpacing(); });
  view->initTheme();

  if (vscrollbarsAdded) {
    int barWidth = view->verticalBar()->getBarWidth();;
    if (get_multiple_displays())
      barWidth *= window->display()->scale();

    rc.w += 2*barWidth;
    if (rc.x2() > workarea.x2()) {
      rc.x = workarea.x2() - rc.w;
      if (rc.x < workarea.x) {
        rc.x = workarea.x;
        rc.w = workarea.w;
      }
    }
  }

  // New bounds
  bounds = rc;

  window->removeChild(menubox);
  view->attachToView(menubox);
  window->addChild(view);
}

//////////////////////////////////////////////////////////////////////
// Menu

Menu::Menu()
  : Widget(kMenuWidget)
  , m_menuitem(nullptr)
{
  enableFlags(IGNORE_MOUSE);
  initTheme();
}

Menu::~Menu()
{
  if (m_menuitem) {
    if (m_menuitem->getSubmenu() == this) {
      m_menuitem->setSubmenu(nullptr);
    }
    else {
      ASSERT(m_menuitem->getSubmenu() == nullptr);
    }
  }
}

void Menu::onOpenPopup()
{
  OpenPopup();
}

//////////////////////////////////////////////////////////////////////
// MenuBox

MenuBox::MenuBox(WidgetType type)
 : Widget(type)
 , m_base(nullptr)
{
  this->setFocusStop(true);
  initTheme();
}

MenuBox::~MenuBox()
{
  stopFilteringMouseDown();
}

//////////////////////////////////////////////////////////////////////
// MenuBar

bool MenuBar::m_expandOnMouseover = false;

MenuBar::MenuBar(ProcessTopLevelShortcuts processShortcuts)
  : MenuBox(kMenuBarWidget)
  , m_processTopLevelShortcuts(processShortcuts == ProcessTopLevelShortcuts::kYes)
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
  m_submenu = nullptr;
  m_submenu_menubox = nullptr;

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
  m_base.reset(new MenuBaseData);
  return m_base.get();
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
    m_submenu->setOwnerMenuItem(nullptr);

  m_submenu = menu;

  if (m_submenu) {
    ASSERT_VALID_WIDGET(m_submenu);
    m_submenu->setOwnerMenuItem(this);
  }
}

void MenuItem::openSubmenu()
{
  if (auto menu = static_cast<Menu*>(parent()))
    menu->highlightItem(this, true, true, true);
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

void Menu::showPopup(const gfx::Point& pos,
                     Display* parentDisplay)
{
  // Set the owner menu item to nullptr temporarily in case that we
  // are re-using a menu from the root menu as popup menu (e.g. like
  // "animation_menu", that is used when right-cliking a Play button)
  base::ScopedValue<MenuItem*> restoreOwner(m_menuitem, nullptr);

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
  MenuBoxWindow window;
  window.Open.connect([this]{ this->onOpenPopup(); });

  // Set the native parent (without this the menubox will appear in
  // the same screen of the main window/manager and not the
  // parentDisplay's screen)
  if (parentDisplay)
    window.setParentDisplay(parentDisplay);

  MenuBox* menubox = window.menubox();
  MenuBaseData* base = menubox->createBase();
  base->was_clicked = true;

  // Set children
  menubox->setMenu(this);
  menubox->startFilteringMouseDown();

  window.remapWindow();
  fit_bounds(parentDisplay,
             &window,
             gfx::Rect(pos, window.size()),
             [&window](const gfx::Rect& workarea,
                       gfx::Rect& bounds,
                       std::function<gfx::Rect(Widget*)> getWidgetBounds) {
               choose_side(bounds, workarea, gfx::Rect(bounds.x-1, bounds.y, 1, 1));
               add_scrollbars_if_needed(&window, workarea, bounds);
             });

  // Set the focus to the new menubox
  Manager* manager = Manager::getDefault();
  manager->setFocus(menubox);

  // Open the window
  window.openWindowInForeground();

  // Free the keyboard focus if it's in the menu popup, in other case
  // it means that the user set the focus to other specific widget
  // before we closed the popup.
  Widget* focus = manager->getFocus();
  if (focus && focus->window() == &window)
    focus->releaseFocus();

  menubox->stopFilteringMouseDown();
}

Widget* Menu::findItemById(const char* id) const
{
  Widget* result = findChild(id);
  if (result)
    return result;
  for (auto child : children()) {
    if (child->type() == kMenuItemWidget) {
      if (Menu* submenu = static_cast<MenuItem*>(child)->getSubmenu()) {
        result = submenu->findItemById(id);
        if (result)
          return result;
      }
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

  for (auto it=children().begin(),
         end=children().end();
         it!=end; ) {
    auto next = it;
    ++next;

    reqSize = (*it)->sizeHint();

    if (parent() &&
        parent()->type() == kMenuBarWidget) {
      size.w += reqSize.w + ((next != end) ? childSpacing(): 0);
      size.h = std::max(size.h, reqSize.h);
    }
    else {
      size.w = std::max(size.w, reqSize.w);
      size.h += reqSize.h + ((next != end) ? childSpacing(): 0);
    }

    it = next;
  }

  size.w += border().width();
  size.h += border().height();

  ev.setSizeHint(size);
}

bool MenuBox::onProcessMessage(Message* msg)
{
  Menu* menu = MenuBox::getMenu();

  switch (msg->type()) {

    case kMouseMoveMessage: {
      MenuBaseData* base = get_base(this);
      ASSERT(base);
      if (!base)
        break;

      if (!base->was_clicked)
        break;

      [[fallthrough]];
    }

    case kMouseDownMessage:
    case kDoubleClickMessage:
      if (menu && msg->display()) {
        ASSERT(menu->parent() == this);

        MenuBaseData* base = get_base(this);
        ASSERT(base);
        if (!base)
          break;

        if (base->is_processing)
          break;

        const gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        const gfx::Point screenPos = msg->display()->nativeWindow()->pointToScreen(mousePos);

        // Get the widget below the mouse cursor
        auto mgr = manager();
        if (!mgr)
          break;

        Widget* picked = mgr->pickFromScreenPos(screenPos);

        // Here we catch the filtered messages (menu-bar or the
        // popuped menu-box) to detect if the user press outside of
        // the widget
        if (msg->type() == kMouseDownMessage && m_base != nullptr) {
          // If one of these conditions are accomplished we have to
          // close all menus (back to menu-bar or close the popuped
          // menubox), this is the place where we control if...
          if (picked == nullptr ||         // If the button was clicked nowhere
              picked == this ||         // If the button was clicked in this menubox
              // The picked widget isn't from the same tree of menus
              (get_base_menubox(picked) != this ||
               (this->type() == kMenuBarWidget &&
                picked->type() == kMenuWidget))) {
            // The user click outside all the menu-box/menu-items, close all
            menu->closeAll();

            // Change to "return false" if you want to send the click
            // to the window after closing all menus.
            return true;
          }
        }

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
                base->was_clicked = false;
              }
            }
          }
          else if (!base->was_clicked) {
            menu->unhighlightItem();
          }
        }
      }
      break;

    case kMouseLeaveMessage:
      if (menu) {
        MenuBaseData* base = get_base(this);
        ASSERT(base);
        if (!base)
          break;

        if (base->is_processing)
          break;

        MenuItem* highlight = menu->getHighlightedItem();
        if (highlight && !highlight->hasSubmenuOpened())
          menu->unhighlightItem();
      }
      break;

    case kMouseUpMessage:
      if (menu) {
        MenuBaseData* base = get_base(this);
        ASSERT(base);
        if (!base)
          break;

        if (base->is_processing)
          break;

        // The item is highlighted and not opened (and the timer to open the submenu is stopped)
        MenuItem* highlight = menu->getHighlightedItem();
        if (highlight &&
            !highlight->hasSubmenuOpened() &&
            highlight->m_submenu_timer == nullptr) {
          menu->closeAll();
          highlight->executeClick();
        }
      }
      break;

    case kKeyDownMessage:
      if (menu) {
        MenuItem* selected;

        MenuBaseData* base = get_base(this);
        ASSERT(base);
        if (!base)
          break;

        if (base->is_processing)
          break;

        base->was_clicked = false;

        // Check for ALT+some underlined letter
        if (((this->type() == kMenuBoxWidget) && (msg->modifiers() == kKeyNoneModifier || // <-- Inside menu-boxes we can use letters without Alt modifier pressed
                                                  msg->modifiers() == kKeyAltModifier)) ||
            ((this->type() == kMenuBarWidget) && (msg->modifiers() == kKeyAltModifier) &&
             static_cast<MenuBar*>(this)->processTopLevelShortcuts())) {
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
          MenuItem* child_with_submenu_opened = nullptr;
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
                  ASSERT(root);
                  if (!root)
                    break;
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

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        auto mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point scroll = view->viewScroll();

        if (mouseMsg->preciseWheel())
          scroll += mouseMsg->wheelDelta();
        else
          scroll += mouseMsg->wheelDelta() * textHeight()*3;

        view->setViewScroll(scroll);
      }
      break;
    }

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
        ASSERT(base);
        if (!base)
          break;

        bool select_first = static_cast<OpenMenuItemMessage*>(msg)->select_first();

        ASSERT(base->is_processing);
        ASSERT(hasSubmenu());

        // New window that will be automatically deleted
        auto window = new MenuBoxWindow(this);
        window->Close.connect([window]{
          window->deferDelete();
        });

        auto parentDisplay = display();
        if (parentDisplay)
          window->setParentDisplay(parentDisplay);

        MenuBox* menubox = window->menubox();
        m_submenu_menubox = menubox;
        menubox->setMenu(m_submenu);

        window->remapWindow();
        fit_bounds(
          parentDisplay, window, window->bounds(),
          [this, window, parentDisplay](const gfx::Rect& workarea,
                                        gfx::Rect& bounds,
                                        std::function<gfx::Rect(Widget*)> getWidgetBounds){
            const gfx::Rect itemBounds = getWidgetBounds(this);
            if (inBar()) {
              bounds.x = std::clamp(itemBounds.x, workarea.x, std::max(workarea.x, workarea.x2()-bounds.w));
              bounds.y = std::max(workarea.y, itemBounds.y2());
            }
            else {
              int scale = guiscale();
              if (get_multiple_displays())
                scale = parentDisplay->scale();

              const gfx::Rect parentBounds = getWidgetBounds(this->window());
              bounds.y = itemBounds.y-3*scale;
              choose_side(bounds, workarea, parentBounds);
            }

            add_scrollbars_if_needed(window, workarea, bounds);
          });

        // Setup the highlight of the new menubox
        if (select_first) {
          // Select the first child
          MenuItem* first_child = nullptr;

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
        ASSERT(base);
        if (!base)
          break;

        ASSERT(base->is_processing);

        ASSERT(m_submenu_menubox);
        Window* window = m_submenu_menubox->window();
        ASSERT(window && window->type() == kWindowWidget);
        m_submenu_menubox = nullptr;

        // Destroy the window
        window->closeWindow(nullptr);

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
        ASSERT(base);
        if (!base)
          break;

        ASSERT(hasSubmenu());

        // Stop timer to open the popup
        stopTimer();

        // If the submenu is closed, and we are not processing messages, open it
        if (m_submenu_menubox == nullptr && !base->is_processing)
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

        ASSERT(menu != nullptr);
        ASSERT(menu->getOwnerMenuItem() != nullptr);

        // We have received a crash report where the "menu" variable
        // can be nullptr in the kMouseDownMessage message processing
        // from MenuBox::onProcessMessage().
        if (menu == nullptr)
          return nullptr;

        widget = menu->getOwnerMenuItem();
      }
    }
    // This is useful for menuboxes inside a viewport (so we can scroll a viewport clicking scrollbars)
    else if (widget->type() == kViewScrollbarWidget &&
             widget->parent() &&
             widget->parent()->type() == kViewWidget &&
             static_cast<View*>(widget->parent())->attachedWidget() &&
             static_cast<View*>(widget->parent())->attachedWidget()->type() == kMenuBoxWidget) {
      widget = static_cast<View*>(widget->parent())->attachedWidget();
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
  return nullptr;
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

      // Scroll
      View* view = nullptr;
      if (menuitem->parent() &&
          menuitem->parent()->parent()) {
        view = View::getView(menuitem->parent()->parent());
      }
      if (view) {
        gfx::Rect itemBounds = menuitem->bounds();
        itemBounds.y -= menuitem->parent()->origin().y;

        gfx::Point scroll = view->viewScroll();
        gfx::Size visSize = view->visibleSize();

        if (itemBounds.y < scroll.y)
          scroll.y = itemBounds.y;
        else if (itemBounds.y2() > scroll.y+visSize.h)
          scroll.y = itemBounds.y2()-visSize.h;

        view->setViewScroll(scroll);
      }
    }

    // Highlight parents
    if (getOwnerMenuItem() != nullptr) {
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
        MenuBaseData* base = get_base(menuitem);
        ASSERT(base);
        if (base)
          base->was_clicked = true;
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
  highlightItem(nullptr, false, false, false);
}

bool MenuItem::inBar() const
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
  ASSERT(m_submenu_menubox == nullptr);

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
  ASSERT(base);
  if (!base)
    return;
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

  ASSERT(m_submenu_menubox != nullptr);

  // First: recursively close the children
  menu = m_submenu_menubox->getMenu();
  ASSERT(menu != nullptr);

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
    ASSERT(base);
    if (base) {
      ASSERT(base->is_processing == false);

      // Start processing
      base->is_processing = true;
    }
  }
}

void MenuItem::startTimer()
{
  if (m_submenu_timer == nullptr)
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
  MenuItem* menuitem = nullptr;
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

  if (menuitem != nullptr) {
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
  if (!menu)
    return;

  MenuBaseData* base = get_base(this);
  ASSERT(base);
  if (!base)
    return;

  // Do not close the popup menus if we're already processing
  // open/close popup messages.
  if (base->is_processing)
    return;

  menu->closeAll();

  // Lost focus
  Manager::getDefault()->freeFocus();
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
  return nullptr;
}

// Finds the next item of `menuitem', if `menuitem' is nullptr searchs
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
    return find_nextitem(menu, nullptr);
  else
   return nullptr;
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
    return find_previtem(menu, nullptr);
  else
    return nullptr;
}

//////////////////////////////////////////////////////////////////////
// MenuBoxWindow

MenuBoxWindow::MenuBoxWindow(MenuItem* menuitem)
  : Window(WithoutTitleBar, "")
  , m_menuitem(menuitem)
{
  setMoveable(false); // Can't move the window
  setSizeable(false); // Can't resize the window

  m_menubox.setFocusMagnet(true);

  addChild(&m_menubox);
}

MenuBoxWindow::~MenuBoxWindow()
{
  // The menu of the menubox should already be nullptr because it
  // was reset in kCloseMessage.
  ASSERT(m_menubox.getMenu() == nullptr);
  m_menubox.setMenu(nullptr);

  // This can fail in case that add_scrollbars_if_needed() replaced
  // the MenuBox widget with a View, and now the MenuBox is inside the
  // viewport.
  if (hasChild(&m_menubox)) {
    removeChild(&m_menubox);
  }
  else {
    ASSERT(firstChild() != nullptr &&
           firstChild()->type() == kViewWidget);
  }
}

bool MenuBoxWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      if (m_menuitem) {
        MenuBaseData* base = get_base(m_menuitem);

        // If this window was closed using the OS close button
        // (e.g. on Linux we can Super key+right click to show the
        // popup menu and close the window)
        if (base && !base->is_processing) {
          if (m_menuitem->hasSubmenuOpened())
            m_menuitem->closeSubmenu(true);
        }
      }

      // Fetch the "menu" to avoid destroy it with 'delete'.
      m_menubox.setMenu(nullptr);
      break;

  }
  return Window::onProcessMessage(msg);
}

} // namespace ui
