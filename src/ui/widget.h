// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_WIDGET_H_INCLUDED
#define UI_WIDGET_H_INCLUDED

#include <string>

#include "gfx/border.h"
#include "gfx/rect.h"
#include "gfx/size.h"
#include "ui/base.h"
#include "ui/component.h"
#include "ui/list.h"
#include "ui/rect.h"
#include "ui/widgets_list.h"

#define ASSERT_VALID_WIDGET(widget) ASSERT((widget) != NULL)

struct FONT;
struct BITMAP;

namespace ui {

  union Message;

  class InitThemeEvent;
  class Manager;
  class PaintEvent;
  class PreferredSizeEvent;
  class Theme;
  class Window;

  int ji_register_widget_type();

  // Position and geometry

  JRect jwidget_get_rect(Widget* widget);
  JRect jwidget_get_child_rect(Widget* widget);
  JRegion jwidget_get_region(Widget* widget);
  JRegion jwidget_get_drawable_region(Widget* widget, int flags);
  int jwidget_get_bg_color(Widget* widget);
  int jwidget_get_text_length(const Widget* widget);
  int jwidget_get_text_height(const Widget* widget);
  void jwidget_get_texticon_info(Widget* widget,
                                 JRect box, JRect text, JRect icon,
                                 int icon_align, int icon_w, int icon_h);

  void jwidget_noborders(Widget* widget);
  void jwidget_set_border(Widget* widget, int value);
  void jwidget_set_border(Widget* widget, int l, int t, int r, int b);
  void jwidget_set_rect(Widget* widget, JRect rect);
  void jwidget_set_min_size(Widget* widget, int w, int h);
  void jwidget_set_max_size(Widget* widget, int w, int h);
  void jwidget_set_bg_color(Widget* widget, int color);

  //////////////////////////////////////////////////////////////////////

  class Widget : public Component
  {
  public:
    int type;                     /* widget's type */

    JRect rc;                     /* position rectangle */
    struct {
      int l, t, r, b;
    } border_width;               /* border separation with the parent */
    int child_spacing;            /* separation between children */

    /* flags */
    int flags;

    /* widget size limits */
    int min_w, min_h;
    int max_w, max_h;

    /* structures */
    JList children;                /* sub-objects */
    Widget* parent;                /* who is the parent? */

  private:
    std::string m_id;             // Widget's id
    Theme* m_theme;               // Widget's theme
    int m_align;                  // Widget alignment
    std::string m_text;           // Widget text
    struct FONT *m_font;          // Text font type
    int m_bg_color;               // Background color
    JRegion m_update_region;      // Region to be redrawed.

  public:
    // Extra data for the theme
    void *theme_data[4];

    /* for user */
    void *user_data[4];

    // ===============================================================
    // CTOR & DTOR
    // ===============================================================

    Widget(int type);
    virtual ~Widget();

    // Safe way to delete a widget when it is not in the manager message
    // queue anymore.
    void deferDelete();

    // Main properties.

    int getType() const { return this->type; }

    const std::string& getId() const { return m_id; }
    void setId(const char* id) { m_id = id; }

    int getAlign() const { return m_align; }
    void setAlign(int align) { m_align = align; }

    // Text property.

    bool hasText() const { return flags & JI_NOTEXT ? false: true; }

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

    // True if this widget and all its ancestors are visible.
    bool isVisible() const;
    void setVisible(bool state);

    // True if this widget can receive user input (is not disabled).
    bool isEnabled() const;
    void setEnabled(bool state);

    // True if this widget is selected (pushed in case of a button, or
    // checked in the case of a check-box).
    bool isSelected() const;
    void setSelected(bool state);

    // True if this widget wants more space when it's inside a Box
    // parent.
    bool isExpansive() const;
    void setExpansive(bool state);

    // True if this is a decorative widget created by the current
    // theme. Decorative widgets are arranged by the theme instead that
    // the parent's widget.
    bool isDecorative() const;
    void setDecorative(bool state);

    // True if this widget can receive the keyboard focus.
    bool isFocusStop() const;
    void setFocusStop(bool state);

    // True if this widget wants the focus by default when it's shown by
    // first time (e.g. when its parent window is opened).
    void setFocusMagnet(bool state);
    bool isFocusMagnet() const;

    // ===============================================================
    // LOOK & FEEL
    // ===============================================================

    FONT* getFont() const;
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

    Window* getRoot();
    Widget* getParent();
    Manager* getManager();

    // Returns a list of parents (you must free the list), if
    // "ascendant" is true the list is build from child to parents, else
    // the list is from parent to children.
    JList getParents(bool ascendant);

    // Returns a list of children (you must free the list).
    JList getChildren();

    // Returns the next or previous siblings.
    Widget* getNextSibling();
    Widget* getPreviousSibling();

    Widget* pick(int x, int y);
    bool hasChild(Widget* child);
    Widget* findChild(const char* id);

    // Returns a widget in the same window that is located "sibling".
    Widget* findSibling(const char* id);

    // Finds a child with the specified ID and dynamic-casts it to type
    // T.
    template<class T>
    T* findChildT(const char* id) {
      return dynamic_cast<T*>(findChild(id));
    }

    template<class T>
    T* findFirstChildByType() {
      JLink link;
      JI_LIST_FOR_EACH(children, link) {
        Widget* child = (Widget*)link->data;
        if (T* specificChild = dynamic_cast<T*>(child))
          return specificChild;
      }
      return NULL;
    }

    void addChild(Widget* child);
    void removeChild(Widget* child);
    void replaceChild(Widget* oldChild, Widget* newChild);
    void insertChild(int index, Widget* child);

    // ===============================================================
    // LAYOUT & CONSTRAINT
    // ===============================================================

    void layout();

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
    void invalidateRect(const gfx::Rect& rect);
    void invalidateRect(const JRect rect);
    void invalidateRegion(const JRegion region);

    void flushRedraw();

    void scrollRegion(JRegion region, int dx, int dy);

    // ===============================================================
    // GUI MANAGER
    // ===============================================================

    bool sendMessage(Message* msg);
    void closeWindow();

    void broadcastMouseMessage(WidgetsList& targets);

    // ===============================================================
    // SIZE & POSITION
    // ===============================================================

    gfx::Size getPreferredSize();
    gfx::Size getPreferredSize(const gfx::Size& fitIn);
    void setPreferredSize(const gfx::Size& fixedSize);
    void setPreferredSize(int fixedWidth, int fixedHeight);

    // ===============================================================
    // MOUSE, FOCUS & KEYBOARD
    // ===============================================================

    void requestFocus();
    void releaseFocus();
    void captureMouse();
    void releaseMouse();
    bool hasFocus();
    bool hasMouse();
    bool hasMouseOver();
    bool hasCapture();

    // Returns lower-case letter that represet the mnemonic of the widget
    // (the underscored character, i.e. the letter after & symbol).
    int getMnemonicChar() const;

    bool isScancodeMnemonic(int scancode) const;

  protected:

    // ===============================================================
    // MESSAGE PROCESSING
    // ===============================================================

    virtual bool onProcessMessage(Message* msg);

    // ===============================================================
    // EVENTS
    // ===============================================================

    virtual void onInvalidateRegion(const JRegion region);
    virtual void onPreferredSize(PreferredSizeEvent& ev);
    virtual void onPaint(PaintEvent& ev);
    virtual void onBroadcastMouseMessage(WidgetsList& targets);
    virtual void onInitTheme(InitThemeEvent& ev);
    virtual void onEnable();
    virtual void onDisable();
    virtual void onSelect();
    virtual void onDeselect();
    virtual void onSetText();

  private:
    gfx::Size* m_preferredSize;
    bool m_doubleBuffered : 1;
  };

} // namespace ui

#endif
