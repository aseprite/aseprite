// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>
#include <ctype.h>
#include <stdio.h>

#include "gui/gui.h"
#include "gui/intern.h"
#include "gfx/size.h"

#define TIMEOUT_TO_OPEN_SUBMENU 250

using namespace gfx;

//////////////////////////////////////////////////////////////////////
// Internal messages: to move between menus

JM_MESSAGE(open_menuitem);
JM_MESSAGE(close_menuitem);
JM_MESSAGE(close_popup);
JM_MESSAGE(exe_menuitem);

// Extra fields for the JM_OPEN_MENUITEM message:
// bool select_first = msg->user.a;
//   If this value is true, it means that after opening the menu, we
//   have to select the first item (i.e. highlighting it).
#define JM_OPEN_MENUITEM  jm_open_menuitem()

// Extra fields for the JM_CLOSE_MENUITEM message:
// bool last_of_close_chain = msg->user.a;
//   This fields is used to indicate the end of a sequence of
//   JM_OPEN_MENU and JM_CLOSE_MENUITEM messages. If it is true
//   the message is the last one of the chain, which means that no
//   more JM_OPEN_MENU or JM_CLOSE_MENUITEM messages are in the queue.
#define JM_CLOSE_MENUITEM jm_close_menuitem()

#define JM_CLOSE_POPUP    jm_close_popup()

#define JM_EXE_MENUITEM   jm_exe_menuitem()

// Data for the main jmenubar or the first popuped-jmenubox
struct MenuBaseData
{
  // True when the menu-items must be opened with the cursor movement
  bool was_clicked : 1;

  // True when there's JM_OPEN/CLOSE_MENUITEM messages in queue, to
  // avoid start processing another menuitem-request when we're
  // already working in one
  bool is_processing : 1;

  // True when the JM_BUTTONPRESSED is being filtered
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

static MenuBox* get_base_menubox(Widget* widget);
static MenuBaseData* get_base(Widget* widget);

static bool window_msg_proc(Widget* widget, Message* msg);

static MenuItem* check_for_letter(Menu* menu, int ascii);
static MenuItem* check_for_accel(Menu* menu, Message* msg);

static MenuItem* find_nextitem(Menu* menu, MenuItem* menuitem);
static MenuItem* find_previtem(Menu* menu, MenuItem* menuitem);

Menu::Menu()
  : Widget(JI_MENU)
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

MenuBox::MenuBox(int type)
 : Widget(type)
 , m_base(NULL)
{
  jwidget_focusrest(this, true);
  initTheme();
}

MenuBox::~MenuBox()
{
  if (m_base && m_base->is_filtering) {
    m_base->is_filtering = false;
    jmanager_remove_msg_filter(JM_BUTTONPRESSED, this);
  }

  delete m_base;
}

MenuBar::MenuBar()
  : MenuBox(JI_MENUBAR)
{
  createBase();
}

MenuItem::MenuItem(const char *text)
  : Widget(JI_MENUITEM)
{
  m_accel = NULL;
  m_highlighted = false;
  m_submenu = NULL;
  m_submenu_menubox = NULL;
  m_submenu_timer = -1;

  setText(text);
  initTheme();
}

MenuItem::~MenuItem()
{
  if (m_accel)
    jaccel_free(m_accel);

  if (m_submenu)
    jwidget_free(m_submenu);

  // Stop timer to open the popup
  if (m_submenu_timer >= 0) {
    jmanager_remove_timer(m_submenu_timer);
    m_submenu_timer = -1;
  }
}

Menu* MenuBox::getMenu()
{
  if (jlist_empty(children))
    return NULL;
  else
    return static_cast<Menu*>(jlist_first(children)->data);
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

JAccel MenuItem::getAccel()
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
 *          the menu-item'll receive JM_DESTROY message.
 */
void MenuItem::setAccel(JAccel accel)
{
  if (m_accel)
    jaccel_free(m_accel);

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
  return (m_submenu && !jlist_empty(m_submenu->children));
}

void Menu::showPopup(int x, int y)
{
  do {
    jmouse_poll();
  } while (jmouse_b(0));

  // New window and new menu-box
  Frame* window = new Frame(false, NULL);
  MenuBox* menubox = new MenuBox();
  MenuBaseData* base = menubox->createBase();
  base->was_clicked = true;
  base->is_filtering = true;
  jmanager_add_msg_filter(JM_BUTTONPRESSED, menubox);

  window->set_moveable(false);	 // Can't move the window

  // Set children
  menubox->setMenu(this);
  window->addChild(menubox);

  window->remap_window();

  // Menubox position
  window->position_window(MID(0, x, JI_SCREEN_W-jrect_w(window->rc)),
			  MID(0, y, JI_SCREEN_H-jrect_h(window->rc)));

  // Set the focus to the new menubox
  jmanager_set_focus(menubox);
  jwidget_magnetic(menubox, true);

  // Open the window
  window->open_window_fg();

  // Free the keyboard focus
  jmanager_free_focus();

  // Fetch the "menu" so it isn't destroyed
  menubox->setMenu(NULL);

  // Destroy the window
  jwidget_free(window);
}

bool Menu::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      requestSize(&msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      set_position(&msg->setpos.rect);
      return true;

    case JM_DRAW:
      getTheme()->draw_menu(this, &msg->draw.rect);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void Menu::requestSize(int *w, int *h)
{
  Size reqSize;
  JLink link;

  *w = *h = 0;

  JI_LIST_FOR_EACH(this->children, link) {
    reqSize = ((Widget*)link->data)->getPreferredSize();

    if (this->parent->type == JI_MENUBAR) {
      *w += reqSize.w + ((link->next != this->children->end) ?
			 this->child_spacing: 0);
      *h = MAX(*h, reqSize.h);
    }
    else {
      *w = MAX(*w, reqSize.w);
      *h += reqSize.h + ((link->next != this->children->end) ?
			 this->child_spacing: 0);
    }
  }

  *w += this->border_width.l + this->border_width.r;
  *h += this->border_width.t + this->border_width.b;
}

void Menu::set_position(JRect rect)
{
  Size reqSize;
  Widget* child;
  JRect cpos;
  JLink link;

  jrect_copy(this->rc, rect);
  cpos = jwidget_get_child_rect(this);

  JI_LIST_FOR_EACH(this->children, link) {
    child = (Widget*)link->data;

    reqSize = child->getPreferredSize();

    if (this->parent->type == JI_MENUBAR)
      cpos->x2 = cpos->x1+reqSize.w;
    else
      cpos->y2 = cpos->y1+reqSize.h;

    jwidget_set_rect(child, cpos);

    if (this->parent->type == JI_MENUBAR)
      cpos->x1 += jrect_w(cpos);
    else
      cpos->y1 += jrect_h(cpos);
  }

  jrect_free(cpos);
}

bool MenuBox::onProcessMessage(Message* msg)
{
  Menu* menu = MenuBox::getMenu();

  switch (msg->type) {

    case JM_REQSIZE:
      requestSize(&msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      set_position(&msg->setpos.rect);
      return true;

    case JM_MOTION:
      /* isn't pressing a button? */
      if (!msg->mouse.flags && !get_base(this)->was_clicked)
	break;

      // Fall though

    case JM_BUTTONPRESSED:
      if (menu) {
	if (get_base(this)->is_processing)
	  break;

	// Here we catch the filtered messages (menu-bar or the
	// popuped menu-box) to detect if the user press outside of
	// the widget
	if (msg->type == JM_BUTTONPRESSED && m_base != NULL) {
	  Widget* picked = ji_get_default_manager()->pick(msg->mouse.x, msg->mouse.y);

	  // If one of these conditions are accomplished we have to
	  // close all menus (back to menu-bar or close the popuped
	  // menubox), this is the place where we control if...
	  if (picked == NULL ||		// If the button was clicked nowhere
	      picked == this ||         // If the button was clicked in this menubox
	      // The picked widget isn't menu-related
	      (picked->type != JI_MENUBOX &&
	       picked->type != JI_MENUBAR &&
	       picked->type != JI_MENUITEM) ||
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
	  if ((picked->type == JI_MENUITEM) &&
	      !(picked->flags & JI_DISABLED)) {

	    // If the picked menu-item is not highlighted...
	    if (!static_cast<MenuItem*>(picked)->isHighlighted()) {
	      // In menu-bar always open the submenu, in other popup-menus
	      // open the submenu only if the user does click
	      bool open_submenu =
		(this->type == JI_MENUBAR) ||
		(msg->type == JM_BUTTONPRESSED);
	      
	      menu->highlightItem(static_cast<MenuItem*>(picked), false, open_submenu, false);
	    }
	    // If the user pressed in a highlighted menu-item (maybe
	    // the user was waiting for the timer to open the
	    // submenu...)
	    else if (msg->type == JM_BUTTONPRESSED &&
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

    case JM_MOUSELEAVE:
      if (menu) {
	if (get_base(this)->is_processing)
	  break;
	
	MenuItem* highlight = menu->getHighlightedItem();
	if (highlight && !highlight->hasSubmenuOpened())
	  menu->unhighlightItem();
      }
      break;

    case JM_BUTTONRELEASED:
      if (menu) {
	if (get_base(this)->is_processing)
	  break;

	// The item is highlighted and not opened (and the timer to open the submenu is stopped)
	MenuItem* highlight = menu->getHighlightedItem();
	if (highlight &&
	    !highlight->hasSubmenuOpened() &&
	    highlight->m_submenu_timer < 0) {
	  menu->closeAll();
	  highlight->executeClick();
	}
      }
      break;

    case JM_KEYPRESSED:
      if (menu) {
	MenuItem* selected;

	if (get_base(this)->is_processing)
	  break;

	get_base(this)->was_clicked = false;

	// Check for ALT+some underlined letter
	if (((this->type == JI_MENUBOX) && (msg->any.shifts == 0 || // Inside menu-boxes we can use letters without Alt modifier pressed
					    msg->any.shifts == KB_ALT_FLAG)) ||
	    ((this->type == JI_MENUBAR) && (msg->any.shifts == KB_ALT_FLAG))) {
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
	  JLink link;
	  bool used = false;

	  // Search a child with highlight or the submenu opened
	  JI_LIST_FOR_EACH(menu->children, link) {
	    Widget* child = (Widget*)link->data;
	    if (child->type != JI_MENUITEM)
	      continue;

	    if (static_cast<MenuItem*>(child)->hasSubmenuOpened())
	      child_with_submenu_opened = static_cast<MenuItem*>(child);
	  }

	  if (!highlight && child_with_submenu_opened)
	    highlight = child_with_submenu_opened;

	  switch (msg->key.scancode) {

	    case KEY_ESC:
	      // In menu-bar
	      if (this->type == JI_MENUBAR) {
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
	      if (this->type == JI_MENUBAR) {
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
	      if (this->type == JI_MENUBAR) {
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
	      if (this->type == JI_MENUBAR) {
                // Go to previous
                highlight = find_previtem(menu, highlight);
		menu->highlightItem(highlight, false, false, false);
              }
              // In menu-boxes
	      else {
                // Go to parent
                if (menu->m_menuitem) {
                  Widget* parent = menu->m_menuitem->parent->parent;

                  // Go to the previous item in the parent

                  // If the parent is the menu-bar
                  if (parent->type == JI_MENUBAR) {
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
	      if (this->type == JI_MENUBAR) {
                // Go to next
                highlight = find_nextitem(menu, highlight);
		menu->highlightItem(highlight, false, false, false);
              }
              // In menu-boxes
              else {
                // Enter in sub-menu
                if ((highlight) && highlight->hasSubmenu()) {
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
      if (msg->type == JM_CLOSE_POPUP) {
	_jmanager_close_window(this->getManager(),
			       static_cast<Frame*>(this->getRoot()), true);
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void MenuBox::requestSize(int* w, int* h)
{
  if (Menu* menu = getMenu()) {
    Size reqSize = menu->getPreferredSize();
    *w = reqSize.w;
    *h = reqSize.h;
  }
  else
    *w = *h = 0;

  *w += this->border_width.l + this->border_width.r;
  *h += this->border_width.t + this->border_width.b;
}

void MenuBox::set_position(JRect rect)
{
  jrect_copy(this->rc, rect);

  if (Menu* menu = getMenu()) {
    JRect cpos = jwidget_get_child_rect(this);
    jwidget_set_rect(menu, cpos);
    jrect_free(cpos);
  }
}

bool MenuItem::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      requestSize(&msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_DRAW:
      getTheme()->draw_menuitem(this, &msg->draw.rect);
      return true;

    case JM_MOUSEENTER:
      // TODO theme specific!!
      invalidate();

      // When a menu item receives the mouse, start a timer to open the submenu...
      if (isEnabled() && hasSubmenu()) {
	// Start the timer to open the submenu...
	startTimer();
      }
      break;

    case JM_MOUSELEAVE:
      // TODO theme specific!!
      invalidate();

      // Stop timer to open the popup
      if (m_submenu_timer >= 0) {
	jmanager_remove_timer(m_submenu_timer);
	m_submenu_timer = -1;
      }
      break;

    default:
      if (msg->type == JM_OPEN_MENUITEM) {
	MenuBaseData* base = get_base(this);
	JRect pos;
	bool select_first = msg->user.a ? true: false;

	ASSERT(base != NULL);
	ASSERT(base->is_processing);
	ASSERT(hasSubmenu());

	JRect old_pos = jwidget_get_rect(this->parent->parent);

	// New window and new menu-box
	Frame* window = new Frame(false, NULL);
	jwidget_add_hook(window, -1, window_msg_proc, NULL);

	MenuBox* menubox = new MenuBox();
	m_submenu_menubox = menubox;

	window->set_moveable(false); // Can't move the window

	// Set children
	menubox->setMenu(m_submenu);
	window->addChild(menubox);

	window->remap_window();

	// Menubox position
	pos = jwidget_get_rect(window);

	if (this->parent->parent->type == JI_MENUBAR) {
	  jrect_moveto(pos,
		       MID(0, this->rc->x1, JI_SCREEN_W-jrect_w(pos)),
		       MID(0, this->rc->y2, JI_SCREEN_H-jrect_h(pos)));
	}
	else {
	  int x_left = this->rc->x1-jrect_w(pos);
	  int x_right = this->rc->x2;
	  int x, y = this->rc->y1;
	  struct jrect r1, r2;
	  int s1, s2;

	  r1.x1 = x_left = MID(0, x_left, JI_SCREEN_W-jrect_w(pos));
	  r2.x1 = x_right = MID(0, x_right, JI_SCREEN_W-jrect_w(pos));

	  r1.y1 = r2.y1 = y = MID(0, y, JI_SCREEN_H-jrect_h(pos));

	  r1.x2 = r1.x1+jrect_w(pos);
	  r1.y2 = r1.y1+jrect_h(pos);
	  r2.x2 = r2.x1+jrect_w(pos);
	  r2.y2 = r2.y1+jrect_h(pos);

	  // Calculate both intersections
	  s1 = jrect_intersect(&r1, old_pos);
	  s2 = jrect_intersect(&r2, old_pos);

	  if (!s2)
	    x = x_right;	// Use the right because there aren't intersection with it
	  else if (!s1)
	    x = x_left;		// Use the left because there are not intersection
	  else if (jrect_w(&r2)*jrect_h(&r2) <= jrect_w(&r1)*jrect_h(&r1))
	    x = x_right;	        // Use the right because there are less intersection area
	  else
	    x = x_left;		// Use the left because there are less intersection area

	  jrect_moveto(pos, x, y);
	}

	window->position_window(pos->x1, pos->y1);
	jrect_free(pos);

	// Set the focus to the new menubox
	jwidget_magnetic(menubox, true);

	// Setup the highlight of the new menubox
	if (select_first) {
	  // Select the first child
	  Widget* child;
	  MenuItem* first_child = NULL;
	  JLink link;

	  JI_LIST_FOR_EACH(m_submenu->children, link) {
	    child = (Widget*)link->data;

	    if (child->type != JI_MENUITEM)
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
	window->open_window_bg();

	base->is_processing = false;

	jrect_free(old_pos);
	return true;
      }
      else if (msg->type == JM_CLOSE_MENUITEM) {
	MenuBaseData* base = get_base(this);
	Frame* window;
	bool last_of_close_chain = (msg->user.a ? true: false);

	ASSERT(base != NULL);
	ASSERT(base->is_processing);

	MenuBox* menubox = m_submenu_menubox;
	m_submenu_menubox = NULL;

	ASSERT(menubox != NULL);

	window = (Frame*)menubox->parent;
	ASSERT(window && window->type == JI_FRAME);

	// Fetch the "menu" to avoid free it with 'jwidget_free()'
	menubox->setMenu(NULL);

	// Destroy the window
	window->closeWindow(NULL);

	// Set the focus to this menu-box of this menu-item
	if (base->close_all)
	  jmanager_free_focus();
	else
	  jmanager_set_focus(this->parent->parent);

	// Is not necessary to free this window because it's
	// automatically destroyed by the manager
	// ... jwidget_free(window);

	if (last_of_close_chain) {
	  base->close_all = false;
	  base->is_processing = false;
	}

	// Stop timer to open the popup
	stopTimer();
	return true;
      }
      else if (msg->type == JM_EXE_MENUITEM) {
	onClick();
	return true;
      }
      break;

    case JM_TIMER:
      if (msg->timer.timer_id == m_submenu_timer) {
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

void MenuItem::onClick()
{
  // Fire old JI_SIGNAL_MENUITEM_SELECT signal
  jwidget_emit_signal(this, JI_SIGNAL_MENUITEM_SELECT);

  // Fire new Click() signal.
  Click();
}

void MenuItem::requestSize(int *w, int *h)
{
  int bar = (this->parent->parent->type == JI_MENUBAR);

  if (this->hasText()) {
    *w =
      + this->border_width.l
      + jwidget_get_text_length(this)
      + ((bar) ? this->child_spacing/4: this->child_spacing)
      + this->border_width.r;

    *h =
      + this->border_width.t
      + jwidget_get_text_height(this)
      + this->border_width.b;

    if (m_accel) {
      char buf[256];
      jaccel_to_string(m_accel, buf);
      *w += ji_font_text_len(this->getFont(), buf);
    }
  }
  else {
    *w = *h = 0;
  }
}

// Climbs the hierarchy of menus to get the most-top menubox.
static MenuBox* get_base_menubox(Widget* widget)
{
  while (widget != NULL) {
    ASSERT_VALID_WIDGET(widget);

    // We are in a menubox
    if (widget->type == JI_MENUBOX || widget->type == JI_MENUBAR) {
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
      ASSERT(widget->type == JI_MENUITEM);
      ASSERT(widget->parent != NULL);
      ASSERT(widget->parent->type == JI_MENU);

      widget = widget->parent->parent;
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
  JLink link;
  JI_LIST_FOR_EACH(this->children, link) {
    Widget* child = (Widget*)link->data;
    if (child->type != JI_MENUITEM)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);
    if (menuitem->isHighlighted())
      return menuitem;
  }
  return NULL;
}

void Menu::highlightItem(MenuItem* menuitem, bool click, bool open_submenu, bool select_first_child)
{
  Menu* menu = this;
  Widget* child;
  JLink link;

  // Find the menuitem with the highlight
  JI_LIST_FOR_EACH(menu->children, link) {
    child = (Widget*)link->data;
    if (child->type != JI_MENUITEM)
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
    if (menu->getOwnerMenuItem() != NULL) {
      static_cast<Menu*>(menu->getOwnerMenuItem()->parent)
	->highlightItem(menu->getOwnerMenuItem(), false, false, false);
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
      menu->closeAll();
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
  Widget* child;
  Message* msg;
  JLink link;

  ASSERT(hasSubmenu());

  menu = this->parent;

  // The menu item is already opened?
  ASSERT(m_submenu_menubox == NULL);

  ASSERT_VALID_WIDGET(menu);

  // Close all siblings of 'menuitem'
  if (menu->parent) {
    JI_LIST_FOR_EACH(menu->children, link) {
      child = reinterpret_cast<Widget*>(link->data);
      if (child->type != JI_MENUITEM)
	continue;

      MenuItem* childMenuItem = static_cast<MenuItem*>(child);
      if (childMenuItem != this && childMenuItem->hasSubmenuOpened()) {
	childMenuItem->closeSubmenu(false);
      }
    }
  }

  msg = jmessage_new(JM_OPEN_MENUITEM);
  msg->user.a = select_first;
  jmessage_add_dest(msg, this);
  jmanager_enqueue_message(msg);

  // Get the 'base'
  MenuBaseData* base = get_base(this);
  ASSERT(base != NULL);
  ASSERT(base->is_processing == false);

  // Reset flags
  base->close_all = false;
  base->is_processing = true;  

  // We need to add a filter of the JM_BUTTONPRESSED to intercept
  // clicks outside the menu (and close all the hierarchy in that
  // case); the widget to intercept messages is the base menu-bar or
  // popuped menu-box
  if (!base->is_filtering) {
    base->is_filtering = true;
    jmanager_add_msg_filter(JM_BUTTONPRESSED, get_base_menubox(this));
  }
}

void MenuItem::closeSubmenu(bool last_of_close_chain)
{
  Widget* menu, *child;
  Message* msg;
  JLink link;
  MenuBaseData* base;

  ASSERT(m_submenu_menubox != NULL);

  // First: recursively close the children
  menu = m_submenu_menubox->getMenu();
  ASSERT(menu != NULL);

  JI_LIST_FOR_EACH(menu->children, link) {
    child = reinterpret_cast<Widget*>(link->data);
    if (child->type != JI_MENUITEM)
      continue;
    
    if (static_cast<MenuItem*>(child)->hasSubmenuOpened())
      static_cast<MenuItem*>(child)->closeSubmenu(false);
  }

  // Second: now we can close the 'menuitem'
  msg = jmessage_new(JM_CLOSE_MENUITEM);
  msg->user.a = last_of_close_chain;
  jmessage_add_dest(msg, this);
  jmanager_enqueue_message(msg);

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
  if (m_submenu_timer < 0)
    m_submenu_timer = jmanager_add_timer(this, TIMEOUT_TO_OPEN_SUBMENU);
  jmanager_start_timer(m_submenu_timer);
}

void MenuItem::stopTimer()
{
  // Stop timer to open the popup
  if (m_submenu_timer >= 0) {
    jmanager_remove_timer(m_submenu_timer);
    m_submenu_timer = -1;
  }
}

void Menu::closeAll()
{
  Menu* menu = this;
  MenuItem* menuitem = NULL;
  while (menu->m_menuitem) {
    menuitem = menu->m_menuitem;
    menu = static_cast<Menu*>(menuitem->parent);
  }

  MenuBox* base_menubox = get_base_menubox(menu->parent);
  MenuBaseData* base = base_menubox->getBase();
  base->close_all = true;
  base->was_clicked = false;
  if (base->is_filtering) {
    base->is_filtering = false;
    jmanager_remove_msg_filter(JM_BUTTONPRESSED, base_menubox);
  }

  menu->unhighlightItem();

  if (menuitem != NULL) {
    if (menuitem->hasSubmenuOpened())
      menuitem->closeSubmenu(true);
  }
  else {
    JLink link;
    JI_LIST_FOR_EACH(menu->children, link) {
      Widget* child = reinterpret_cast<Widget*>(link->data);
      if (child->type != JI_MENUITEM)
	continue;

      menuitem = static_cast<MenuItem*>(child);
      if (menuitem->hasSubmenuOpened())
	menuitem->closeSubmenu(true);
    }
  }

  // For popuped menus
  if (base_menubox->type == JI_MENUBOX)
    base_menubox->closePopup();
}

void MenuBox::closePopup()
{
  Message* msg = jmessage_new(JM_CLOSE_POPUP);
  jmessage_add_dest(msg, this);
  jmanager_enqueue_message(msg);
}

void MenuBox::cancelMenuLoop()
{
  Menu* menu = getMenu();
  if (menu)
    menu->closeAll();

  // Lost focus
  jmanager_free_focus();
}

void MenuItem::executeClick()
{
  // Send the message
  Message* msg = jmessage_new(JM_EXE_MENUITEM);
  jmessage_add_dest(msg, this);
  jmanager_enqueue_message(msg);
}

static bool window_msg_proc(Widget* widget, Message* msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      jwidget_free_deferred(widget);
      break;

  }
  return false;
}

static MenuItem* check_for_letter(Menu* menu, int ascii)
{
  JLink link;
  JI_LIST_FOR_EACH(menu->children, link) {
    Widget* child = (Widget*)link->data;
    if (child->type != JI_MENUITEM)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);

    if (menuitem->hasText())
      for (int c=0; menuitem->getText()[c]; c++)
        if ((menuitem->getText()[c] == '&') && (menuitem->getText()[c+1] != '&'))
          if (tolower(ascii) == tolower(menuitem->getText()[c+1]))
	    return menuitem;
  }
  return NULL;
}

static MenuItem* check_for_accel(Menu* menu, Message* msg)
{
  JLink link;
  JI_LIST_FOR_EACH(menu->children, link) {
    Widget* child = (Widget*)link->data;
    if (child->type != JI_MENUITEM)
      continue;

    MenuItem* menuitem = static_cast<MenuItem*>(child);

    if (menuitem->getSubmenu()) {
      if ((menuitem = check_for_accel(menuitem->getSubmenu(), msg)))
        return menuitem;
    }
    else if (menuitem->getAccel()) {
      if ((menuitem->isEnabled()) &&
	  (jaccel_check(menuitem->getAccel(),
			msg->any.shifts,
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
  Widget* nextitem;
  JLink link;

  if (menuitem)
    link = jlist_find(menu->children, menuitem)->next;
  else
    link = jlist_first(menu->children);

  for (; link != menu->children->end; link=link->next) {
    nextitem = (Widget*)link->data;
    if ((nextitem->type == JI_MENUITEM) && nextitem->isEnabled())
      return static_cast<MenuItem*>(nextitem);
  }

  if (menuitem)
    return find_nextitem(menu, NULL);
  else
   return NULL;
}

static MenuItem* find_previtem(Menu* menu, MenuItem* menuitem)
{
  Widget* nextitem;
  JLink link;

  if (menuitem)
    link = jlist_find(menu->children, menuitem)->prev;
  else
    link = jlist_last(menu->children);

  for (; link != menu->children->end; link=link->prev) {
    nextitem = (Widget*)link->data;
    if ((nextitem->type == JI_MENUITEM) && nextitem->isEnabled())
      return static_cast<MenuItem*>(nextitem);
  }

  if (menuitem)
    return find_previtem(menu, NULL);
  else
    return NULL;
}
