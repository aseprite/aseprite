// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_BASE_H_INCLUDED
#define GUI_BASE_H_INCLUDED

// Get the system's definition of NULL
#include <stddef.h>

#ifndef TRUE
  #define TRUE         (-1)
  #define FALSE        (0)
#endif

#undef MIN
#undef MAX
#undef MID

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))

#undef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))

#undef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)

struct jaccel;
struct jhook;
struct jlink;
struct jlist;
struct jrect;
struct jregion;

class Frame;
class Theme;
class Widget;
union Message;

// Alignment.
#define JI_HORIZONTAL	0x0001
#define JI_VERTICAL	0x0002
#define JI_LEFT		0x0004
#define JI_CENTER	0x0008
#define JI_RIGHT	0x0010
#define JI_TOP		0x0020
#define JI_MIDDLE	0x0040
#define JI_BOTTOM	0x0080
#define JI_HOMOGENEOUS	0x0100
#define JI_WORDWRAP	0x0200

// Widget flags.
#define JI_HIDDEN	0x0001 // Is hidden (not visible, not clickeable).
#define JI_SELECTED	0x0002 // Is selected.
#define JI_DISABLED	0x0004 // Is disabled (not usable).
#define JI_HASFOCUS	0x0008 // Has the input focus.
#define JI_HASMOUSE	0x0010 // Has the mouse.
#define JI_HASCAPTURE	0x0020 // Captured the mouse .
#define JI_FOCUSREST	0x0040 // Want the focus (is a rest for focus).
#define JI_MAGNETIC	0x0080 // Attract the focus.
#define JI_EXPANSIVE	0x0100 // Is expansive (want more space).
#define JI_DECORATIVE	0x0200 // To decorate windows.
#define JI_INITIALIZED	0x0400 // The widget was already initialized by a theme.
#define JI_NOTEXT	0x0800 // The widget does not have text.
#define JI_DIRTY	0x1000 // The widget (or one child) is dirty (update_region != empty).

// Widget types.
enum {
  // Undefined (or anonymous) widget type.
  JI_WIDGET,

  // Known widgets.
  JI_BOX,
  JI_BUTTON,
  JI_CHECK,
  JI_COMBOBOX,
  JI_ENTRY,
  JI_GRID,
  JI_IMAGE_VIEW,
  JI_LABEL,
  JI_LISTBOX,
  JI_LISTBOX2,
  JI_LISTITEM,
  JI_MANAGER,
  JI_MENU,
  JI_MENUBAR,
  JI_MENUBOX,
  JI_MENUITEM,
  JI_PANEL,
  JI_RADIO,
  JI_SEPARATOR,
  JI_SLIDER,
  JI_TEXTBOX,
  JI_VIEW,
  JI_VIEW_SCROLLBAR,
  JI_VIEW_VIEWPORT,
  JI_FRAME,

  // User widgets.
  JI_USER_WIDGET,
};

// JINETE Message types.
enum {
  // General messages.
  JM_OPEN,			// Windows is open.
  JM_CLOSE,			// Windows is closed.
  JM_DESTROY,			// Widget is destroyed.
  JM_DRAW,			// Widget needs be repainted.
  JM_SIGNAL,			// Signal from some widget.
  JM_TIMER,			// A timer timeout.
  JM_REQSIZE,			// Request size.
  JM_SETPOS,			// Set position.
  JM_WINMOVE,			// Window movement.
  JM_DEFERREDFREE,		// Deferred jwidget_free call.
  JM_DIRTYCHILDREN,		// Dirty children.
  JM_QUEUEPROCESSING,		// Only sent to manager which indicate
				// the last message in the queue.

  // Keyboard related messages.
  JM_KEYPRESSED,		// When a any key is pressed.
  JM_KEYRELEASED,		// When a any key is released.
  JM_FOCUSENTER,		// Widget gets the focus.
  JM_FOCUSLEAVE,		// Widget losts the focus.

  // Mouse related messages.
  JM_BUTTONPRESSED,		// User makes click inside a widget.
  JM_BUTTONRELEASED,		// User releases mouse button in a widget.
  JM_DOUBLECLICK,		// User makes double click in some widget.
  JM_MOUSEENTER,		// A widget gets mouse pointer.
  JM_MOUSELEAVE,		// A widget losts mouse pointer.
  JM_MOTION,			// User moves the mouse on some widget.
  JM_SETCURSOR,			// A widget needs to setup the mouse cursor.
  JM_WHEEL,			// User moves the wheel.

  // xxx drag'n'drop operation?.
  // JM_DND_

  // Other messages.
  JM_REGISTERED_MESSAGES
};

// Signals.
enum {
  // Generic signals
  JI_SIGNAL_ENABLE,
  JI_SIGNAL_DISABLE,
  JI_SIGNAL_SELECT,
  JI_SIGNAL_DESELECT,
  JI_SIGNAL_SHOW,
  JI_SIGNAL_HIDE,
  JI_SIGNAL_ADD_CHILD,
  JI_SIGNAL_SET_TEXT,
  JI_SIGNAL_SET_FONT,
  JI_SIGNAL_INIT_THEME,

  // Special widget signals
  JI_SIGNAL_BUTTON_SELECT,
  JI_SIGNAL_CHECK_CHANGE,
  JI_SIGNAL_RADIO_CHANGE,
  JI_SIGNAL_ENTRY_CHANGE,
  JI_SIGNAL_LISTBOX_CHANGE,
  JI_SIGNAL_LISTBOX_SELECT,
  JI_SIGNAL_COMBOBOX_CHANGE,
  JI_SIGNAL_COMBOBOX_SELECT,
  JI_SIGNAL_MANAGER_EXTERNAL_CLOSE,
  JI_SIGNAL_MANAGER_ADD_WINDOW,
  JI_SIGNAL_MANAGER_REMOVE_WINDOW,
  JI_SIGNAL_MANAGER_LOSTCHAR,
  JI_SIGNAL_MENUITEM_SELECT,
  JI_SIGNAL_WINDOW_RESIZE,
};

// Flags for jwidget_get_drawable_region.
#define JI_GDR_CUTTOPWINDOWS	1 // Cut areas where are windows on top.
#define JI_GDR_USECHILDAREA	2 // Use areas where are children.

typedef unsigned int		JID;

typedef void*			JThread;

typedef struct jaccel*		JAccel;
typedef struct jhook*		JHook;
typedef struct jlink*		JLink;
typedef struct jlist*		JList;
typedef struct jstream*		JStream;
typedef struct jrect*		JRect;
typedef struct jregion*		JRegion;
typedef class  Widget*		JWidget;
typedef struct jxml*		JXml;
typedef struct jxmlattr*	JXmlAttr;
typedef struct jxmlnode*	JXmlNode;
typedef struct jxmlelem*	JXmlElem;
typedef struct jxmltext*	JXmlText;

typedef bool (*MessageFunc)	 (JWidget widget, Message* msg);
typedef void (*JDrawFunc)	 (JWidget widget, JRect clip);

class Jinete
{
public:
  Jinete();
  ~Jinete();
};

#endif	// GUI_BASE_H_INCLUDED
