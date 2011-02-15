// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_WIDGET_H_INCLUDED
#define GUI_WIDGET_H_INCLUDED

#include <string>

#include "gfx/rect.h"
#include "gfx/border.h"
#include "gui/component.h"
#include "gui/base.h"
#include "gui/rect.h"

class PreferredSizeEvent;
class PaintEvent;

#ifndef NDEBUG
#include "gui/intern.h"
#define ASSERT_VALID_WIDGET(widget) ASSERT((widget) != NULL &&		\
					   _ji_is_valid_widget((widget)))
#else
#define ASSERT_VALID_WIDGET(widget) ((void)0)
#endif

struct FONT;
struct BITMAP;

int ji_register_widget_type();

void jwidget_free(JWidget widget);
void jwidget_free_deferred(JWidget widget);

/* hooks */

void jwidget_add_hook(JWidget widget, int type,
		      JMessageFunc msg_proc, void *data);
JHook jwidget_get_hook(JWidget widget, int type);
void *jwidget_get_data(JWidget widget, int type);
 
/* behavior properties */

void jwidget_magnetic(JWidget widget, bool state);
void jwidget_expansive(JWidget widget, bool state);
void jwidget_decorative(JWidget widget, bool state);
void jwidget_focusrest(JWidget widget, bool state);

bool jwidget_is_magnetic(JWidget widget);
bool jwidget_is_expansive(JWidget widget);
bool jwidget_is_decorative(JWidget widget);
bool jwidget_is_focusrest(JWidget widget);

/* children handle */

void jwidget_add_child(JWidget widget, JWidget child);
void jwidget_add_children(JWidget widget, ...);
void jwidget_remove_child(JWidget widget, JWidget child);
void jwidget_replace_child(JWidget widget, JWidget old_child,
			   JWidget new_child);

/* position and geometry */

void jwidget_relayout(JWidget widget);
JRect jwidget_get_rect(JWidget widget);
JRect jwidget_get_child_rect(JWidget widget);
JRegion jwidget_get_region(JWidget widget);
JRegion jwidget_get_drawable_region(JWidget widget, int flags);
int jwidget_get_bg_color(JWidget widget);
int jwidget_get_text_length(JWidget widget);
int jwidget_get_text_height(JWidget widget);
void jwidget_get_texticon_info(JWidget widget,
			       JRect box, JRect text, JRect icon,
			       int icon_align, int icon_w, int icon_h);

void jwidget_noborders(JWidget widget);
void jwidget_set_border(JWidget widget, int value);
void jwidget_set_border(JWidget widget, int l, int t, int r, int b);
void jwidget_set_rect(JWidget widget, JRect rect);
void jwidget_set_min_size(JWidget widget, int w, int h);
void jwidget_set_max_size(JWidget widget, int w, int h);
void jwidget_set_bg_color(JWidget widget, int color);

/* drawing methods */

void jwidget_flush_redraw(JWidget widget);
void jwidget_scroll(JWidget widget, JRegion region, int dx, int dy);

/* signal handle */

void jwidget_signal_on(JWidget widget);
void jwidget_signal_off(JWidget widget);

bool jwidget_emit_signal(JWidget widget, int signal_num);

/* miscellaneous */

JWidget jwidget_find_name(JWidget widget, const char *name);
bool jwidget_check_underscored(JWidget widget, int scancode);

//////////////////////////////////////////////////////////////////////

class Widget : public Component
{
public:
  JID id;			/* identify code */
  int type;			/* widget's type */

  char *name;			/* widget's name */
  JRect rc;			/* position rectangle */
  struct {
    int l, t, r, b;
  } border_width;		/* border separation with the parent */
  int child_spacing;		/* separation between children */

  /* flags */
  int flags;
  int emit_signals;		/* emit signal counter */

  /* widget size limits */
  int min_w, min_h;
  int max_w, max_h;

  /* structures */
  JList children;		 /* sub-objects */
  JWidget parent;		 /* who is the parent? */

  /* virtual properties */
  JList hooks;			/* hooks with msg_proc and specific data */

  /* common widget properties */
private:
  Theme* m_theme;		// Widget's theme
  int m_align;			// Widget alignment
  std::string m_text;		// Widget text
  struct FONT *m_font;		// Text font type
  int m_bg_color;		// Background color
public:

  /* drawable cycle */
  JRegion update_region;	/* region to be redrawed */

  /* more properties... */

  // Extra data for the theme
  void *theme_data[4];

  /* for user */
  void *user_data[4];

  // ===============================================================
  // CTOR & DTOR
  // ===============================================================

  Widget(int type);
  virtual ~Widget();

  // main properties

  int getType();
  const char* getName();
  int getAlign() const;

  void setName(const char* name);
  void setAlign(int align);

  // text property

  bool hasText() { return flags & JI_NOTEXT ? false: true; }

  const char* getText() const { return m_text.c_str(); }
  int getTextInt() const;
  double getTextDouble() const;
  size_t getTextSize() const { return m_text.size(); }
  void setText(const char* text);
  void setTextf(const char* text, ...);
  void setTextQuiet(const char* text);

  // ===============================================================
  // COMMON PROPERTIES
  // ===============================================================

  bool isVisible() const;
  void setVisible(bool state);

  bool isEnabled() const;
  void setEnabled(bool state);

  bool isSelected() const;
  void setSelected(bool state);

  // ===============================================================
  // LOOK & FEEL
  // ===============================================================

  FONT* getFont();
  void setFont(FONT* font);

  // Gets the background color of the widget.
  int getBgColor() const {
    if (m_bg_color < 0 && parent)
      return parent->getBgColor();
    else
      return m_bg_color;
  }

  // Sets the background color of the widget
  void setBgColor(int bg_color) {
    m_bg_color = bg_color;
  }

  Theme* getTheme() const { return m_theme; }
  void setTheme(Theme* theme);

  void initTheme();

  // ===============================================================
  // PARENTS & CHILDREN
  // ===============================================================

  Widget* getRoot();
  Widget* getParent();
  Widget* getManager();
  JList getParents(bool ascendant);
  JList getChildren();
  Widget* pick(int x, int y);
  bool hasChild(Widget* child);
  Widget* findChild(const char* name);
  Widget* findSibling(const char* name);

  // Finds a child with the specified name and dynamic-casts it to
  // type T.
  template<class T>
  T* findChildT(const char* name) {
    return dynamic_cast<T*>(findChild(name));
  }

  // ===============================================================
  // POSITION & GEOMETRY
  // ===============================================================

  gfx::Rect getBounds() const;
  gfx::Rect getClientBounds() const;
  void setBounds(const gfx::Rect& rc);

  gfx::Border getBorder() const;
  void setBorder(const gfx::Border& border);

  // ===============================================================
  // REFRESH ISSUES
  // ===============================================================

  bool isDoubleBuffered();
  void setDoubleBuffered(bool doubleBuffered);

  void invalidate();
  void invalidateRect(const JRect rect);
  void invalidateRegion(const JRegion region);

  // ===============================================================
  // GUI MANAGER
  // ===============================================================

  bool sendMessage(JMessage msg);
  void closeWindow();

  // ===============================================================
  // SIZE & POSITION
  // ===============================================================

  gfx::Size getPreferredSize();
  gfx::Size getPreferredSize(const gfx::Size& fitIn);
  void setPreferredSize(const gfx::Size& fixedSize);
  void setPreferredSize(int fixedWidth, int fixedHeight);

  // ===============================================================
  // FOCUS & MOUSE
  // ===============================================================

  void requestFocus();
  void releaseFocus();
  void captureMouse();
  void releaseMouse();
  bool hasFocus();
  bool hasMouse();
  bool hasMouseOver();
  bool hasCapture();

protected:

  // ===============================================================
  // MESSAGE PROCESSING
  // ===============================================================

  virtual bool onProcessMessage(JMessage msg);

  // ===============================================================
  // EVENTS
  // ===============================================================

  virtual void onPreferredSize(PreferredSizeEvent& ev);
  virtual void onPaint(PaintEvent& ev);

private:
  gfx::Size* m_preferredSize;
  bool m_doubleBuffered : 1;
};

#endif
