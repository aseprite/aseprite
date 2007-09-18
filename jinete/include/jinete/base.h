/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_BASE_H
#define JINETE_BASE_H

/* get the system's definition of NULL */
#include <stddef.h>

/* guard C code in headers, while including them from C++ */
#ifdef __cplusplus
  #define JI_BEGIN_DECLS	extern "C" {
  #define JI_END_DECLS		}
#else
  #define JI_BEGIN_DECLS
  #define JI_END_DECLS
#endif

/* get bool, false and true definitions for C */
#ifndef __cplusplus
  #ifdef HAVE_STDBOOL_H
    #include <stdbool.h>
  #endif

  #undef bool
  #define bool unsigned char
#endif

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

JI_BEGIN_DECLS

struct jaccel;
struct jhook;
struct jquickmenu;
struct jlink;
struct jlist;
union  jmessage;
struct jrect;
struct jregion;
struct jtheme;
struct jwidget;

/* alignment */
#define JI_HORIZONTAL	1
#define JI_VERTICAL	2
#define JI_LEFT		4
#define JI_CENTER	8
#define JI_RIGHT	16
#define JI_TOP		32
#define JI_MIDDLE	64
#define JI_BOTTOM	128
#define JI_HOMOGENEOUS	256
#define JI_WORDWRAP	512

/* widget flags */
/* #define JI_DIRTY	0x000001 /\* need be redraw *\/ */
#define JI_HIDDEN	0x000002 /* is hidden (not visible, not clickeable) */
#define JI_SELECTED	0x000004 /* is selected */
#define JI_DISABLED	0x000008 /* is disabled (not usable) */
#define JI_HASFOCUS	0x000010 /* has the input focus */
#define JI_HASMOUSE	0x000020 /* has the mouse */
#define JI_HASCAPTURE	0x000040 /* captured the mouse  */
#define JI_FOCUSREST	0x000080 /* want the focus (is a rest for focus) */
#define JI_MAGNETIC	0x000100 /* attract the focus */
#define JI_EXPANSIVE	0x000200 /* is expansive (want more space) */
#define JI_DECORATIVE	0x000400 /* to decorate windows */
#define JI_AUTODESTROY	0x000800 /* destroyed by the manager */
#define JI_HARDCAPTURE	0x001000 /* only windows use hard capture */
#define JI_INITIALIZED	0x002000 /* the widget was already initialized by a theme */

/* widget types */
enum {
  /* undefined (or anonymous) widget type */
  JI_WIDGET,

  /* known widgets */
  JI_BOX,
  JI_BUTTON,
  JI_CHECK,
  JI_COMBOBOX,
  JI_ENTRY,
  JI_IMAGE,
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
  JI_WINDOW,

  /* user widgets */
  JI_USER_WIDGET,
};

/* Jinete Message types */
enum {
  /* general messages */
  JM_OPEN,			/* windows is open */
  JM_CLOSE,			/* windows is closed */
  JM_DESTROY,			/* widget is destroyed */
  JM_DRAW,			/* widget needs be repainted */
  JM_IDLE,			/* is sent when the manager does nothing */
  JM_SIGNAL,			/* signal from some widget */
  JM_REQSIZE,			/* request size */
  JM_SETPOS,			/* set position */
  JM_WINMOVE,			/* window movement */
  JM_DRAWRGN,			/* redraw region */
  JM_DIRTYCHILDREN,		/* dirty children */

  /* keyboard related messages */
  JM_CHAR,			/* a new character in the buffer */
  JM_KEYPRESSED,		/* when a any key is pressed */
  JM_KEYRELEASED,		/* when a any key is released */
  JM_FOCUSENTER,		/* widget gets the focus */
  JM_FOCUSLEAVE,		/* widget losts the focus */

  /* mouse related messages */
  JM_BUTTONPRESSED,		/* user makes click inside a widget */
  JM_BUTTONRELEASED,		/* user releases mouse button in a widget */
  JM_DOUBLECLICK,		/* user makes double click in some widget */
  JM_MOUSEENTER,		/* a widget gets mouse pointer */
  JM_MOUSELEAVE,		/* a widget losts mouse pointer */
  JM_MOTION,			/* user moves the mouse on some widget */
  JM_WHEEL,			/* user moves the wheel */

  /* XXX drag'n'drop operation? */
  /* JM_DND_, */
};

/* signals */
enum {
  /* generic signals */
  JI_SIGNAL_DIRTY,
  JI_SIGNAL_ENABLE,
  JI_SIGNAL_DISABLE,
  JI_SIGNAL_SELECT,
  JI_SIGNAL_DESELECT,
  JI_SIGNAL_SHOW,
  JI_SIGNAL_HIDE,
  JI_SIGNAL_ADD_CHILD,
  JI_SIGNAL_REMOVE_CHILD,
  JI_SIGNAL_NEW_PARENT,
  JI_SIGNAL_GET_TEXT,
  JI_SIGNAL_SET_TEXT,
  JI_SIGNAL_INIT_THEME,

  /* special widget signals */
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
  JI_SIGNAL_SLIDER_CHANGE,
  JI_SIGNAL_WINDOW_CLOSE,
  JI_SIGNAL_WINDOW_RESIZE,
};

/* flags for jwidget_get_drawable_region */
#define JI_GDR_CUTTOPWINDOWS	1 /* cut areas where are windows on top */
#define JI_GDR_USECHILDAREA	2 /* use areas where are children */

typedef unsigned int		JID;

typedef struct jaccel		*JAccel;
typedef struct jhook		*JHook;
typedef struct jquickmenu	*JQuickMenu;
typedef struct jlink		*JLink;
typedef struct jlist		*JList;
typedef union  jmessage		*JMessage;
typedef struct jrect		*JRect;
typedef struct jregion		*JRegion;
typedef struct jtheme		*JTheme;
typedef struct jwidget		*JWidget;

typedef void (*JFunc)            (void *data, void *user_data);
typedef int  (*JCompareFunc)     (const void *a, const void *b);
typedef int  (*JCompareDataFunc) (const void *a, const void *b, void *user_data);
typedef bool (*JMessageFunc)	 (JWidget widget, JMessage msg);
typedef void (*JDrawFunc)	 (JWidget widget);

/* memory routines */
void *jmalloc  (unsigned long n_bytes);
void *jmalloc0 (unsigned long n_bytes);
void *jrealloc (void *mem, unsigned long n_bytes);
void  jfree    (void *mem);

char *jstrdup  (const char *string);

#define jnew(struct_type, n_structs)					\
    ((struct_type *)jmalloc (sizeof (struct_type) * (n_structs)))
#define jnew0(struct_type, n_structs)					\
    ((struct_type *)jmalloc0 (sizeof (struct_type) * (n_structs)))
#define jrenew(struct_type, mem, n_structs)				\
    ((struct_type *)jrealloc ((mem), (sizeof (struct_type) * (n_structs))))

JI_END_DECLS

#endif /* JINETE_BASE_H */
