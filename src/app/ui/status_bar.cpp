// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/document_range.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/tools/tool.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "app/util/range_utils.h"
#include "base/bind.h"
#include "doc/cel.h"
#include "doc/document_event.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/size.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace doc;

static const char* kStatusBarText = "status_bar_text";
static const char* kStatusBarFace = "status_bar_face";

class StatusBar::CustomizedTipWindow : public ui::TipWindow {
public:
  CustomizedTipWindow(const char* text)
    : ui::TipWindow(text, gfx::Rect())
  {
  }

  void setInterval(int msecs)
  {
    if (!m_timer)
      m_timer.reset(new ui::Timer(msecs, this));
    else
      m_timer->setInterval(msecs);
  }

  void startTimer()
  {
    m_timer->start();
  }

protected:
  bool onProcessMessage(Message* msg);

private:
  base::UniquePtr<ui::Timer> m_timer;
};

static WidgetType statusbar_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

// This widget is used to show the current frame.
class GotoFrameEntry : public Entry {
public:
  GotoFrameEntry() : Entry(4, "") {
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      // When the mouse enter in this entry, it got the focus and the
      // text is automatically selected.
      case kMouseEnterMessage:
        requestFocus();
        selectText(0, -1);
        break;

      case kKeyDownMessage: {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        if (hasFocus() &&
            (scancode == kKeyEnter || // TODO customizable keys
             scancode == kKeyEnterPad)) {
          Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoFrame);
          Params params;
          int frame = getTextInt();
          if (frame > 0) {
            params.set("frame", getText().c_str());
            UIContext::instance()->executeCommand(cmd, params);
          }
          // Select the text again
          selectText(0, -1);
          return true;          // Key used.
        }
        break;
      }
    }
    return Entry::onProcessMessage(msg);
  }

};

StatusBar* StatusBar::m_instance = NULL;

StatusBar::StatusBar()
  : Widget(statusbar_type())
  , m_color(app::Color::fromMask())
  , m_doc(nullptr)
{
  m_instance = this;

  setDoubleBuffered(true);

  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  setBgColor(theme->getColorById(kStatusBarFace));

  this->setFocusStop(true);

  m_timeout = 0;
  m_state = SHOW_TEXT;
  m_tipwindow = NULL;

  // The extra pixel in left and right borders are necessary so
  // m_commandsBox and m_movePixelsBox do not overlap the upper-left
  // and upper-right pixels drawn in onPaint() event (see putpixels)
  setBorder(gfx::Border(1*guiscale(), 0, 1*guiscale(), 0));

  // Construct the commands box
  {
    Box* box1 = new Box(HORIZONTAL);
    Box* box4 = new Box(HORIZONTAL);

    m_frameLabel = new Label("Frame:");
    m_currentFrame = new GotoFrameEntry();
    m_newFrame = new Button("+");
    m_newFrame->Click.connect(Bind<void>(&StatusBar::newFrame, this));
    m_slider = new Slider(0, 255, 255);

    setup_mini_look(m_currentFrame);
    setup_mini_look(m_newFrame);
    setup_mini_look(m_slider);

    m_slider->Change.connect(Bind<void>(&StatusBar::onCelOpacitySliderChange, this));

    box1->setBorder(gfx::Border(2, 1, 2, 2)*guiscale());

    box4->addChild(m_currentFrame);
    box4->addChild(m_newFrame);

    box1->addChild(m_frameLabel);
    box1->addChild(box4);
    box1->addChild(m_slider);

    m_commandsBox = box1;
    addChild(m_commandsBox);
    m_commandsBox->setVisible(false);
  }

  // Tooltips manager
  TooltipManager* tooltipManager = new TooltipManager();
  addChild(tooltipManager);
  tooltipManager->addTooltipFor(m_currentFrame, "Current Frame", BOTTOM);
  tooltipManager->addTooltipFor(m_slider, "Cel Opacity", BOTTOM);

  Preferences::instance().toolBox.activeTool.AfterChange.connect(
    Bind<void>(&StatusBar::onCurrentToolChange, this));

  UIContext::instance()->addObserver(this);
  UIContext::instance()->documents().addObserver(this);
}

StatusBar::~StatusBar()
{
  UIContext::instance()->documents().removeObserver(this);
  UIContext::instance()->removeObserver(this);

  delete m_tipwindow;           // widget
  delete m_commandsBox;
}

void StatusBar::onCurrentToolChange()
{
  if (isVisible()) {
    tools::Tool* tool = App::instance()->activeTool();
    if (tool) {
      showTool(500, tool);
      setTextf("%s Selected", tool->getText().c_str());
    }
  }
}

void StatusBar::clearText()
{
  setStatusText(1, "");
}

bool StatusBar::setStatusText(int msecs, const char *format, ...)
{
  if ((ui::clock() > m_timeout) || (msecs > 0)) {
    char buf[256];              // TODO warning buffer overflow
    va_list ap;

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    m_timeout = ui::clock() + msecs;
    m_state = SHOW_TEXT;

    setText(buf);
    invalidate();

    return true;
  }
  else
    return false;
}

void StatusBar::showTip(int msecs, const char *format, ...)
{
  char buf[256];                // TODO warning buffer overflow
  va_list ap;
  int x, y;

  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  if (m_tipwindow == NULL) {
    m_tipwindow = new CustomizedTipWindow(buf);
  }
  else {
    m_tipwindow->setText(buf);
  }

  m_tipwindow->setInterval(msecs);

  if (m_tipwindow->isVisible())
    m_tipwindow->closeWindow(NULL);

  m_tipwindow->openWindow();
  m_tipwindow->remapWindow();

  x = getBounds().x2() - m_tipwindow->getBounds().w;
  y = getBounds().y - m_tipwindow->getBounds().h;
  m_tipwindow->positionWindow(x, y);

  m_tipwindow->startTimer();

  // Set the text in status-bar (with inmediate timeout)
  m_timeout = ui::clock();
  setText(buf);
  invalidate();
}

void StatusBar::showColor(int msecs, const char* text, const app::Color& color, int alpha)
{
  if (setStatusText(msecs, text)) {
    m_state = SHOW_COLOR;
    m_color = color;
    m_alpha = alpha;
  }
}

void StatusBar::showTool(int msecs, tools::Tool* tool)
{
  ASSERT(tool != NULL);

  // Tool name
  std::string text = tool->getText();

  // Tool shortcut
  Key* key = KeyboardShortcuts::instance()->tool(tool);
  if (key && !key->accels().empty()) {
    text += ", Shortcut: ";
    text += key->accels().front().toString();
  }

  // Set text
  if (setStatusText(msecs, text.c_str())) {
    // Show tool
    m_state = SHOW_TOOL;
    m_tool = tool;
  }
}

//////////////////////////////////////////////////////////////////////
// StatusBar message handler

void StatusBar::onResize(ResizeEvent& ev)
{
  m_slider->setMinSize(gfx::Size(ui::display_w()/5, 0));

  setBoundsQuietly(ev.getBounds());

  Border border = this->border();
  Rect rc = ev.getBounds();
  bool frameControls = (rc.w > 300*ui::guiscale());

  if (frameControls) {
    m_slider->setVisible(rc.w > 400*ui::guiscale());
    int prefWidth = m_commandsBox->getPreferredSize().w;
    int toolBarWidth = ToolBar::instance()->getPreferredSize().w;

    rc.x += rc.w - prefWidth - border.right() - toolBarWidth;
    rc.w = prefWidth;

    m_commandsBox->setVisible(m_doc != nullptr);
    m_commandsBox->setBounds(rc);
  }
  else
    m_commandsBox->setVisible(false);
}

void StatusBar::onPreferredSize(PreferredSizeEvent& ev)
{
  int s = 4*guiscale() + getTextHeight() + 4*guiscale();
  ev.setPreferredSize(Size(s, s));
}

void StatusBar::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  gfx::Color textColor = theme->getColorById(kStatusBarText);
  Rect rc = getClientBounds();
  Graphics* g = ev.getGraphics();

  g->fillRect(getBgColor(), rc);

  rc.shrink(Border(2, 1, 2, 2)*guiscale());

  int x = rc.x + 4*guiscale();

  // Color
  if (m_state == SHOW_COLOR) {
    // Draw eyedropper icon
    she::Surface* icon = theme->get_toolicon("eyedropper");
    if (icon) {
      g->drawRgbaSurface(icon, x, rc.y + rc.h/2 - icon->height()/2);
      x += icon->width() + 4*guiscale();
    }

    // Draw color
    draw_color_button(g, gfx::Rect(x, rc.y, 32*guiscale(), rc.h),
      m_color, false, false);

    x += (32+4)*guiscale();

    // Draw color description
    std::string str = m_color.toHumanReadableString(app_get_current_pixel_format(),
      app::Color::LongHumanReadableString);
    if (m_alpha < 255) {
      char buf[256];
      sprintf(buf, " \xCE\xB1%d", m_alpha);
      str += buf;
    }

    g->drawString(str, textColor, ColorNone,
      gfx::Point(x, rc.y + rc.h/2 - getFont()->height()/2));

    x += getFont()->textLength(str.c_str()) + 4*guiscale();
  }

  // Show tool
  if (m_state == SHOW_TOOL) {
    // Draw eyedropper icon
    she::Surface* icon = theme->get_toolicon(m_tool->getId().c_str());
    if (icon) {
      g->drawRgbaSurface(icon, x, rc.y + rc.h/2 - icon->height()/2);
      x += icon->width() + 4*guiscale();
    }
  }

  // Status bar text
  if (getTextLength() > 0) {
    g->drawString(getText(), textColor, ColorNone,
      gfx::Point(x, rc.y + rc.h/2 - getFont()->height()/2));

    x += getFont()->textLength(getText().c_str()) + 4*guiscale();
  }
}

bool StatusBar::CustomizedTipWindow::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kTimerMessage:
      closeWindow(NULL);
      break;
  }

  return ui::TipWindow::onProcessMessage(msg);
}

void StatusBar::onCelOpacitySliderChange()
{
  try {
    ContextWriter writer(UIContext::instance(), 500);

    DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
    if (range.enabled()) {
      for (Cel* cel : get_unique_cels(writer.sprite(), range))
        cel->setOpacity(m_slider->getValue());
    }
    else {
      Cel* cel = writer.cel();
      if (cel) {
        // Update the opacity
        cel->setOpacity(m_slider->getValue());
      }
    }

    // Update the editors
    update_screen_for_document(writer.document());
  }
  catch (LockedDocumentException&) {
    // do nothing
  }
}

void StatusBar::onActiveSiteChange(const doc::Site& site)
{
  if (m_doc && site.document() != m_doc) {
    m_doc->removeObserver(this);
    m_doc = nullptr;
  }

  if (site.document() && site.sprite()) {
    if (!m_doc) {
      m_doc = const_cast<doc::Document*>(site.document());
      m_doc->addObserver(this);
    }
    else {
      ASSERT(m_doc == site.document());
    }

    m_commandsBox->setVisible(true);

    // Current frame
    m_currentFrame->setTextf("%d", site.frame()+1);

    // Cel opacity
    const Cel* cel;
    if (site.sprite()->supportAlpha() &&
        site.layer() &&
        site.layer()->isImage() &&
        !site.layer()->isBackground() &&
        (cel = site.layer()->cel(site.frame()))) {
      m_slider->setValue(MID(0, cel->opacity(), 255));
      m_slider->setEnabled(true);
    }
    else {
      m_slider->setValue(255);
      m_slider->setEnabled(false);
    }
  }
  else {
    ASSERT(m_doc == nullptr);
    m_commandsBox->setVisible(false);
  }
}

void StatusBar::onRemoveDocument(doc::Document* doc)
{
  if (m_doc &&
      m_doc == doc) {
    m_doc->removeObserver(this);
    m_doc = nullptr;
  }
}

void StatusBar::onCelOpacityChanged(doc::DocumentEvent& ev)
{
  ASSERT(m_doc);
  ASSERT(m_doc == const_cast<doc::Document*>(ev.document()));
  ASSERT(ev.cel());

  if (m_doc &&
      m_doc == const_cast<doc::Document*>(ev.document()) &&
      ev.cel()) {
    m_slider->setValue(MID(0, ev.cel()->opacity(), 255));
  }
}

void StatusBar::onPixelFormatChanged(DocumentEvent& ev)
{
  onActiveSiteChange(UIContext::instance()->activeSite());
}

void StatusBar::newFrame()
{
  Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::NewFrame);
  UIContext::instance()->executeCommand(cmd);
}

} // namespace app
