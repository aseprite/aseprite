// Aseprite UI Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_WIDGET_H_INCLUDED
#define UI_WIDGET_H_INCLUDED
#pragma once

#include "gfx/border.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "gfx/size.h"
#include "obs/signal.h"
#include "os/font.h"
#include "ui/base.h"
#include "ui/component.h"
#include "ui/graphics.h"
#include "ui/widget_type.h"
#include "ui/widgets_list.h"

#include <string>

#define ASSERT_VALID_WIDGET(widget) ASSERT((widget) != nullptr)

namespace ui {

  class Display;
  class InitThemeEvent;
  class KeyMessage;
  class LoadLayoutEvent;
  class Manager;
  class Message;
  class MouseMessage;
  class PaintEvent;
  class ResizeEvent;
  class SaveLayoutEvent;
  class SizeHintEvent;
  class Style;
  class Theme;
  class Window;

  class Widget : public Component {
  public:

    // ===============================================================
    // CTOR & DTOR
    // ===============================================================

    Widget(WidgetType type = kGenericWidget);
    virtual ~Widget();

    // Safe way to delete a widget when it is not in the manager message
    // queue anymore.
    void deferDelete();

    // Main properties.

    WidgetType type() const { return m_type; }
    void setType(WidgetType type) { m_type = type; } // TODO remove this function

    const std::string& id() const { return m_id; }
    void setId(const char* id) { m_id = id; }

    int flags() const { return m_flags; }
    bool hasFlags(int flags) const { return ((m_flags & flags) == flags); }
    void enableFlags(int flags) { m_flags |= flags; }
    void disableFlags(int flags) { m_flags &= ~flags; }

    int align() const { return (m_flags & ALIGN_MASK); }
    void setAlign(int align) {
      m_flags = ((m_flags & PROPERTIES_MASK) |
                 (align & ALIGN_MASK));
    }

    // Text property.

    bool hasText() const { return hasFlags(HAS_TEXT); }

    const std::string& text() const { return m_text; }
    int textInt() const;
    double textDouble() const;
    void setText(const std::string& text);
    void setTextf(const char* text, ...);
    void setTextQuiet(const std::string& text);

    int textWidth() const;
    int textHeight() const;

    gfx::Size textSize() const {
      return gfx::Size(textWidth(), textHeight());
    }

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

    os::Font* font() const;

    // Gets the background color of the widget.
    gfx::Color bgColor() const {
      if (gfx::geta(m_bgColor) == 0 && m_parent)
        return m_parent->bgColor();
      else
        return m_bgColor;
    }

    // Sets the background color of the widget
    void setBgColor(gfx::Color color);

    Theme* theme() const { return m_theme; }
    Style* style() const { return m_style; }
    void setTheme(Theme* theme);
    virtual void setStyle(Style* style);
    void initTheme();

    // ===============================================================
    // PARENTS & CHILDREN
    // ===============================================================

    Window* window() const;
    Widget* parent() const { return m_parent; }
    int parentIndex() const { return m_parentIndex; }
    Manager* manager() const;
    Display* display() const;

    // Returns a list of children.
    const WidgetsList& children() const { return m_children; }
    bool hasChildren() const { return !m_children.empty(); }

    Widget* at(int index) { return m_children[index]; }
    int getChildIndex(Widget* child);

    // Returns the first/last child or nullptr if it doesn't exist.
    Widget* firstChild() {
      return (hasChildren() ? m_children.front(): nullptr);
    }
    Widget* lastChild() {
      return (hasChildren() ? m_children.back(): nullptr);
    }

    // Returns the next or previous siblings.
    Widget* nextSibling();
    Widget* previousSibling();

    Widget* pick(const gfx::Point& pt,
                 const bool checkParentsVisibility = true) const;
    virtual Widget* pickFromScreenPos(const gfx::Point& screenPos) const;

    bool hasChild(Widget* child);
    bool hasAncestor(Widget* ancestor);
    Widget* findChild(const char* id) const;

    // Returns a widget in the same window that is located "sibling".
    Widget* findSibling(const char* id) const;

    // Finds a child with the specified ID and dynamic-casts it to type
    // T.
    template<class T>
    T* findChildT(const char* id) const {
      return dynamic_cast<T*>(findChild(id));
    }

    template<class T>
    T* findFirstChildByType() const {
      for (auto child : m_children) {
        if (T* specificChild = dynamic_cast<T*>(child))
          return specificChild;
      }
      return nullptr;
    }

    void addChild(Widget* child);
    void removeChild(Widget* child);
    void removeAllChildren();
    void replaceChild(Widget* oldChild, Widget* newChild);
    void insertChild(int index, Widget* child);
    void moveChildTo(Widget* thisChild, Widget* toThisPosition);

    // ===============================================================
    // LAYOUT & CONSTRAINT
    // ===============================================================

    void layout();
    void loadLayout();
    void saveLayout();

    void setDecorativeWidgetBounds();

    // ===============================================================
    // POSITION & GEOMETRY
    // ===============================================================

    gfx::Rect bounds() const { return m_bounds; }
    gfx::Point origin() const { return m_bounds.origin(); }
    gfx::Size size() const { return m_bounds.size(); }

    gfx::Rect clientBounds() const {
      return gfx::Rect(0, 0, m_bounds.w, m_bounds.h);
    }

    gfx::Rect childrenBounds() const;
    gfx::Rect clientChildrenBounds() const;

    // Bounds of this widget or window on native screen/desktop coordinates.
    gfx::Rect boundsOnScreen() const;

    // Sets the bounds of the widget generating a onResize() event.
    void setBounds(const gfx::Rect& rc);

    // Sets the bounds of the widget without generating any kind of
    // event. This member function must be used if you override
    // onResize() and want to change the size of the widget without
    // generating recursive onResize() events.
    void setBoundsQuietly(const gfx::Rect& rc);
    void offsetWidgets(int dx, int dy);

    const gfx::Size& minSize() const { return m_minSize; }
    const gfx::Size& maxSize() const { return m_maxSize; }
    void setMinSize(const gfx::Size& sz);
    void setMaxSize(const gfx::Size& sz);
    void setMinMaxSize(const gfx::Size& minSz, const gfx::Size& maxSz);
    void resetMinSize();
    void resetMaxSize();

    const gfx::Border& border() const { return m_border; }
    void setBorder(const gfx::Border& border);

    int childSpacing() const { return m_childSpacing; }
    void setChildSpacing(int childSpacing);

    void noBorderNoChildSpacing();

    // Flags for getDrawableRegion()
    enum DrawableRegionFlags {
      kCutTopWindows = 1, // Cut areas where are windows on top.
      kUseChildArea = 2,  // Use areas where are children.
      kCutTopWindowsAndUseChildArea = kCutTopWindows | kUseChildArea,
    };

    void getRegion(gfx::Region& region);
    void getDrawableRegion(gfx::Region& region, DrawableRegionFlags flags);

    gfx::Point toClient(const gfx::Point& pt) const {
      return pt - m_bounds.origin();
    }
    gfx::Rect toClient(const gfx::Rect& rc) const {
      return gfx::Rect(rc).offset(-m_bounds.x, -m_bounds.y);
    }

    void getTextIconInfo(
      gfx::Rect* box,
      gfx::Rect* text = nullptr,
      gfx::Rect* icon = nullptr,
      int icon_align = 0, int icon_w = 0, int icon_h = 0);

    // ===============================================================
    // REFRESH ISSUES
    // ===============================================================

    bool isDoubleBuffered() const;
    void setDoubleBuffered(bool doubleBuffered);

    bool isTransparent() const;
    void setTransparent(bool transparent);

    void invalidate();
    void invalidateRect(const gfx::Rect& rect);
    void invalidateRegion(const gfx::Region& region);

    // Returns the region to generate PaintMessages. It's cleared
    // after flushRedraw() is called.
    const gfx::Region& getUpdateRegion() const {
      return m_updateRegion;
    }

    // Generates paint messages for the current update region.
    void flushRedraw();

    GraphicsPtr getGraphics(const gfx::Rect& clip);

    // ===============================================================
    // GUI MANAGER
    // ===============================================================

    bool sendMessage(Message* msg);
    void closeWindow();

    void broadcastMouseMessage(const gfx::Point& screenPos,
                               WidgetsList& targets);

    // ===============================================================
    // SIZE & POSITION
    // ===============================================================

    gfx::Size sizeHint();
    gfx::Size sizeHint(const gfx::Size& fitIn);
    void setSizeHint(const gfx::Size& fixedSize);
    void setSizeHint(int fixedWidth, int fixedHeight);
    void resetSizeHint();

    // ===============================================================
    // MOUSE, FOCUS & KEYBOARD
    // ===============================================================

    void requestFocus();
    void releaseFocus();
    void captureMouse();
    void releaseMouse();

    // True when the widget has the keyboard focus (only widgets with
    // FOCUS_STOP flag will receive the HAS_FOCUS flag/receive the
    // focus when the user press the tab key to navigate widgets).
    bool hasFocus() const { return hasFlags(HAS_FOCUS); }

    // True when the widget has the mouse above. If the mouse leaves
    // the widget, the widget will lose the HAS_MOUSE flag. If some
    // widget captures the mouse, no other widget will have this flag,
    // so in this case there are just two options:
    //
    // 1) The widget with the capture (hasCapture()) will has the
    //    mouse flag too.
    // 2) Or no other widget will have the mouse flag until the widget
    //    releases the capture (releaseCapture())
    bool hasMouse() const { return hasFlags(HAS_MOUSE); }

    // True when the widget has captured the mouse, e.g. generally
    // when the user press a mouse button above a clickeable widget
    // (e.g. ui::Button), the widget will capture the mouse
    // temporarily until the mouse button is released. If a widget
    // captures the mouse, it will receive all mouse events until it
    // release (even if the mouse moves outside the widget).
    bool hasCapture() const { return hasFlags(HAS_CAPTURE); }

    // Returns the mouse position relative to the top-left corner of
    // the ui::Display's client area/content rect.
    gfx::Point mousePosInDisplay() const;

    // Returns the mouse position relative to the top-left cornder of
    // the widget bounds.
    gfx::Point mousePosInClientBounds() const {
      return toClient(mousePosInDisplay());
    }

    // Offer the capture to widgets of the given type. Returns true if
    // the capture was passed to other widget.
    bool offerCapture(ui::MouseMessage* mouseMsg, int widget_type);

    // Returns lower-case letter that represet the mnemonic of the widget
    // (the underscored character, i.e. the letter after & symbol).
    int mnemonic() const {
      return (m_mnemonic & kMnemonicCharMask);
    }
    bool mnemonicRequiresModifiers() const {
      return (m_mnemonic & kMnemonicModifiersMask ? true: false);
    }
    void setMnemonic(const int mnemonic,
                     const bool requireModifiers);

    // Assigns mnemonic from the character preceded by the given
    // escapeChar ('&' by default).
    void processMnemonicFromText(const int escapeChar = '&',
                                 const bool requireModifiers = true);

    // Returns true if the mnemonic character is pressed (without modifiers).
    // TODO maybe we can add check for modifiers now that this
    //      information is included in the Widget
    bool isMnemonicPressed(const ui::KeyMessage* keyMsg) const;

    // Signals
    obs::signal<void()> InitTheme;

  protected:
    // ===============================================================
    // MESSAGE PROCESSING
    // ===============================================================

    virtual bool onProcessMessage(Message* msg);

    // ===============================================================
    // EVENTS
    // ===============================================================

    virtual void onInvalidateRegion(const gfx::Region& region);
    virtual void onSizeHint(SizeHintEvent& ev);
    virtual void onLoadLayout(LoadLayoutEvent& ev);
    virtual void onSaveLayout(SaveLayoutEvent& ev);
    virtual void onResize(ResizeEvent& ev);
    virtual void onPaint(PaintEvent& ev);
    virtual void onBroadcastMouseMessage(const gfx::Point& screenPos,
                                         WidgetsList& targets);
    virtual void onInitTheme(InitThemeEvent& ev);
    virtual void onSetDecorativeWidgetBounds();
    virtual void onVisible(bool visible);
    virtual void onEnable(bool enabled);
    virtual void onSelect(bool selected);
    virtual void onSetText();
    virtual void onSetBgColor();
    virtual int onGetTextInt() const;
    virtual double onGetTextDouble() const;

  private:
    void removeChild(const WidgetsList::iterator& it);
    void paint(Graphics* graphics,
               const gfx::Region& drawRegion,
               const bool isBg);
    bool paintEvent(Graphics* graphics,
                    const bool isBg);
    void setDirtyFlag();

    WidgetType m_type;           // Widget's type
    std::string m_id;            // Widget's id
    int m_flags;                 // Special boolean properties (see flags in ui/base.h)
    Theme* m_theme;              // Widget's theme
    Style* m_style;
    std::string m_text;          // Widget text
    mutable os::FontRef m_font;  // Cached font returned by the theme
    gfx::Color m_bgColor;        // Background color
    gfx::Rect m_bounds;
    gfx::Region m_updateRegion;   // Region to be redrawed.
    WidgetsList m_children;       // Sub-widgets
    Widget* m_parent;             // Who is the parent?
    int m_parentIndex;            // Location/index of this widget in the parent's Widget::m_children vector
    gfx::Size* m_sizeHint;

    // Keyboard shortcut to access this widget like Alt+mnemonic.  If
    // kMnemonicModifiersMask bit is zero, it means that the mnemonic
    // can be used without Alt or Command key modifiers (useful for
    // buttons in ui::Alert).
    static constexpr int kMnemonicCharMask = 0x7f;
    static constexpr int kMnemonicModifiersMask = 0x80;
    int m_mnemonic;

    // Widget size limits
    gfx::Size m_minSize, m_maxSize;

    gfx::Border m_border;       // Border separation with the parent
    int m_childSpacing;         // Separation between children
  };

  WidgetType register_widget_type();

} // namespace ui

#endif
