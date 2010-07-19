/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JBASE_H_INCLUDED
#define JINETE_JBASE_H_INCLUDED

/* get the system's definition of NULL */
#include <stddef.h>

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

struct jaccel;
struct jhook;
struct jquickmenu;
struct jlink;
struct jlist;
union  jmessage;
struct jrect;
struct jregion;
class  jtheme;

class  Widget;
class  Frame;

/* alignment */
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

/* widget flags */
#define JI_HIDDEN	0x0001 /* is hidden (not visible, not clickeable) */
#define JI_SELECTED	0x0002 /* is selected */
#define JI_DISABLED	0x0004 /* is disabled (not usable) */
#define JI_HASFOCUS	0x0008 /* has the input focus */
#define JI_HASMOUSE	0x0010 /* has the mouse */
#define JI_HASCAPTURE	0x0020 /* captured the mouse  */
#define JI_FOCUSREST	0x0040 /* want the focus (is a rest for focus) */
#define JI_MAGNETIC	0x0080 /* attract the focus */
#define JI_EXPANSIVE	0x0100 /* is expansive (want more space) */
#define JI_DECORATIVE	0x0200 /* to decorate windows */
#define JI_INITIALIZED	0x0400 /* the widget was already initialized by a theme */
#define JI_NOTEXT	0x0800 /* the widget does not have text */

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
  JI_GRID,
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
  JI_FRAME,

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
  JM_SIGNAL,			/* signal from some widget */
  JM_TIMER,			/* a timer timeout */
  JM_REQSIZE,			/* request size */
  JM_SETPOS,			/* set position */
  JM_WINMOVE,			/* window movement */
  JM_DEFERREDFREE,		/* deferred jwidget_free call */
  JM_DIRTYCHILDREN,		/* dirty children */
  JM_QUEUEPROCESSING,		/* only sent to manager which indicate
				   the last message in the queue */

  /* keyboard related messages */
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
  JM_SETCURSOR,			/* a widget needs to setup the mouse cursor */
  JM_WHEEL,			/* user moves the wheel */

  /* XXX drag'n'drop operation? */
  /* JM_DND_, */

  /* other messages */
  JM_REGISTERED_MESSAGES
};

/* signals */
enum {
  /* generic signals */
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
  JI_SIGNAL_WINDOW_RESIZE,
};

/* flags for jwidget_get_drawable_region */
#define JI_GDR_CUTTOPWINDOWS	1 /* cut areas where are windows on top */
#define JI_GDR_USECHILDAREA	2 /* use areas where are children */

typedef unsigned int		JID;

typedef void*			JThread;

typedef struct jaccel*		JAccel;
typedef struct jhook*		JHook;
typedef struct jquickmenu*	JQuickMenu;
typedef struct jlink*		JLink;
typedef struct jlist*		JList;
typedef union  jmessage*	JMessage;
typedef struct jstream*		JStream;
typedef struct jrect*		JRect;
typedef struct jregion*		JRegion;
typedef class  jtheme*		JTheme;
typedef class  Widget*		JWidget;
typedef struct jxml*		JXml;
typedef struct jxmlattr*	JXmlAttr;
typedef struct jxmlnode*	JXmlNode;
typedef struct jxmlelem*	JXmlElem;
typedef struct jxmltext*	JXmlText;

typedef bool (*JMessageFunc)	 (JWidget widget, JMessage msg);
typedef void (*JDrawFunc)	 (JWidget widget, JRect clip);

/* without leak detection */
void* jmalloc (unsigned long n_bytes);
void* jmalloc0(unsigned long n_bytes);
void* jrealloc(void* mem, unsigned long n_bytes);
void  jfree   (void* mem);
char* jstrdup (const char* string);

#define jnew(struct_type, n_structs)					\
    ((struct_type*)jmalloc(sizeof(struct_type) * (n_structs)))
#define jnew0(struct_type, n_structs)					\
    ((struct_type*)jmalloc0(sizeof(struct_type) * (n_structs)))
#define jrenew(struct_type, mem, n_structs)				\
    ((struct_type*)jrealloc((mem), (sizeof(struct_type) * (n_structs))))

class Jinete
{
public:
  Jinete();
  ~Jinete();
};

#endif
