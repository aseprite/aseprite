// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_STATUS_BAR_H_INCLUDED
#define APP_UI_STATUS_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "base/observers.h"
#include "doc/context_observer.h"
#include "doc/document_observer.h"
#include "doc/documents_observer.h"
#include "doc/layer_index.h"
#include "ui/base.h"
#include "ui/widget.h"

#include <string>
#include <vector>

namespace ui {
  class Box;
  class Button;
  class Entry;
  class Label;
  class Window;
}

namespace render {
  class Zoom;
}

namespace app {
  class ButtonSet;
  class Editor;
  class ZoomEntry;

  namespace tools {
    class Tool;
  }

  class StatusBar : public ui::Widget
                  , public doc::ContextObserver
                  , public doc::DocumentsObserver
                  , public doc::DocumentObserver {
    static StatusBar* m_instance;
  public:
    static StatusBar* instance() { return m_instance; }

    StatusBar();
    ~StatusBar();

    void clearText();

    bool setStatusText(int msecs, const char *format, ...);
    void showTip(int msecs, const char *format, ...);
    void showColor(int msecs, const char* text, const Color& color);
    void showTool(int msecs, tools::Tool* tool);
    void showSnapToGridWarning(bool state);

    // Used by AppEditor to update the zoom level in the status bar.
    void updateFromEditor(Editor* editor);

  protected:
    void onResize(ui::ResizeEvent& ev) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

    // ContextObserver impl
    void onActiveSiteChange(const doc::Site& site) override;

    // DocumentObservers impl
    void onRemoveDocument(doc::Document* doc) override;

    // DocumentObserver impl
    void onPixelFormatChanged(DocumentEvent& ev) override;

  private:
    void onCurrentToolChange();
    void onCelOpacitySliderChange();
    void newFrame();
    void onChangeZoom(const render::Zoom& zoom);

    enum State { SHOW_TEXT, SHOW_COLOR, SHOW_TOOL };

    int m_timeout;
    State m_state;

    // Showing a tool
    tools::Tool* m_tool;

    // Showing a color
    Color m_color;

    // Box of main commands
    ui::Widget* m_docControls;
    ui::Label* m_frameLabel;
    ui::Entry* m_currentFrame;        // Current frame and go to frame entry
    ui::Button* m_newFrame;           // Button to create a new frame
    ZoomEntry* m_zoomEntry;
    doc::Document* m_doc;      // Document used to show the cel slider

    // Tip window
    class CustomizedTipWindow;
    CustomizedTipWindow* m_tipwindow;

    // Snap to grid window
    class SnapToGridWindow;
    SnapToGridWindow* m_snapToGridWindow;
  };

} // namespace app

#endif
