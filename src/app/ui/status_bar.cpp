// Aseprite
// Copyright (C) 2001-2016  David Capello
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
#include "app/pref/preferences.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui/zoom_entry.h"
#include "app/ui_context.h"
#include "app/util/range_utils.h"
#include "base/bind.h"
#include "doc/document_event.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/size.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace doc;

class StatusBar::Indicators : public HBox {

  class Indicator : public Widget {
  public:
    enum IndicatorType {
      kText,
      kIcon,
      kColor
    };
    Indicator(IndicatorType type) : m_type(type) { }
    IndicatorType indicatorType() const { return m_type; }
  private:
    IndicatorType m_type;
  };

  class TextIndicator : public Indicator {
  public:
    TextIndicator(const char* text) : Indicator(kText) {
      updateIndicator(text);
    }

    void updateIndicator(const char* text) {
      if (this->text() == text)
        return;

      setText(text);

      if (minSize().w > textSize().w*2)
        setMinSize(textSize());
      else
        setMinSize(minSize().createUnion(textSize()));
    }

  private:
    void onPaint(ui::PaintEvent& ev) override {
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
      gfx::Color textColor = theme->colors.statusBarText();
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();

      g->fillRect(bgColor(), rc);
      if (textLength() > 0) {
        g->drawString(text(), textColor, ColorNone,
                      Point(rc.x, rc.y + rc.h/2 - font()->height()/2));
      }
    }
  };

  class IconIndicator : public Indicator {
  public:
    IconIndicator(she::Surface* icon, bool colored)
      : Indicator(kIcon)
      , m_icon(nullptr)
      , m_colored(colored) {
      updateIndicator(icon, colored);
    }

    void updateIndicator(she::Surface* icon, bool colored) {
      if (m_icon == icon && m_colored == colored)
        return;

      ASSERT(icon);

      m_icon = icon;
      m_colored = colored;
      setMinSize(minSize().createUnion(Size(m_icon->width(),
                                            m_icon->height())));
    }

  private:
    void onPaint(ui::PaintEvent& ev) override {
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
      gfx::Color textColor = theme->colors.statusBarText();
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();

      g->fillRect(bgColor(), rc);
      if (m_colored)
        g->drawColoredRgbaSurface(
          m_icon, textColor,
          rc.x, rc.y + rc.h/2 - m_icon->height()/2);
      else
        g->drawRgbaSurface(
          m_icon,
          rc.x, rc.y + rc.h/2 - m_icon->height()/2);
    }

    she::Surface* m_icon;
    bool m_colored;
  };

  class ColorIndicator : public Indicator {
  public:
    ColorIndicator(const app::Color& color)
      : Indicator(kColor)
      , m_color(Color::fromMask()) {
      updateIndicator(color, true);
    }

    void updateIndicator(const app::Color& color, bool first = false) {
      if (m_color == color && !first)
        return;

      m_color = color;
      setMinSize(minSize().createUnion(Size(32*guiscale(), 1)));
    }

  private:
    void onPaint(ui::PaintEvent& ev) override {
      Rect rc = clientBounds();
      Graphics* g = ev.graphics();

      g->fillRect(bgColor(), rc);
      draw_color_button(
        g, Rect(rc.x, rc.y, 32*guiscale(), rc.h),
        m_color,
        (doc::ColorMode)app_get_current_pixel_format(), false, false);
    }

    app::Color m_color;
  };

public:

  Indicators() {
  }

  void startIndicators() {
    m_iterator = m_indicators.begin();
  }

  void endIndicators() {
    removeAllNextIndicators();
    layout();
  }

  void addTextIndicator(const char* text) {
    // Re-use indicator
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kText) {
        static_cast<TextIndicator*>(*m_iterator)
          ->updateIndicator(text);
        ++m_iterator;
        return;
      }
      else
        removeAllNextIndicators();
    }

    auto indicator = new TextIndicator(text);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    addChild(indicator);
  }

  void addIconIndicator(she::Surface* icon, bool colored) {
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kIcon) {
        static_cast<IconIndicator*>(*m_iterator)
          ->updateIndicator(icon, colored);
        ++m_iterator;
        return;
      }
      else
        removeAllNextIndicators();
    }

    auto indicator = new IconIndicator(icon, colored);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    addChild(indicator);
  }

  void addColorIndicator(const app::Color& color) {
    if (m_iterator != m_indicators.end()) {
      if ((*m_iterator)->indicatorType() == Indicator::kColor) {
        static_cast<ColorIndicator*>(*m_iterator)
          ->updateIndicator(color);
        ++m_iterator;
        return;
      }
      else
        removeAllNextIndicators();
    }

    auto indicator = new ColorIndicator(color);
    m_indicators.push_back(indicator);
    m_iterator = m_indicators.end();
    addChild(indicator);
  }

private:
  void removeAllNextIndicators() {
    auto it = m_iterator;
    auto end = m_indicators.end();
    for (; it != end; ++it) {
      auto indicator = *it;
      removeChild(indicator);
      delete indicator;
    }
    m_indicators.erase(m_iterator, end);
  }

  std::vector<Indicator*> m_indicators;
  std::vector<Indicator*>::iterator m_iterator;
};

class StatusBar::IndicatorsGeneration {
public:
  IndicatorsGeneration(StatusBar::Indicators* indicators)
    : m_indicators(indicators) {
    m_indicators->startIndicators();
  }

  ~IndicatorsGeneration() {
    m_indicators->endIndicators();
  }

  IndicatorsGeneration& add(const char* text) {
    auto theme = SkinTheme::instance();

    for (auto i = text; *i; ) {
      // Icon
      if (*i == ':' && (i == text || *(i-1) == ' ')) {
        const char* j = i+1;
        for (; *j; ++j) {
          if (*j == ':')
            break;
        }

        if (*(j+1) == 0 || *(j+1) == ' ') {
          if (i != text) {
            // Here i is ':' and i-1 is a whitespace ' '
            m_indicators->addTextIndicator(std::string(text, i-1).c_str());
          }

          auto part = theme->getPartById("icon_" + std::string(i+1, j));
          if (part)
            add(part.get(), true);

          text = i = (*(j+1) == ' ' ? j+2: j+1);
        }
      }
      else
        ++i;
    }

    if (*text != 0)
      m_indicators->addTextIndicator(text);

    return *this;
  }

  IndicatorsGeneration& add(she::Surface* icon, bool colored) {
    if (icon)
      m_indicators->addIconIndicator(icon, colored);
    return *this;
  }

  IndicatorsGeneration& add(const skin::SkinPart* part, bool colored) {
    return add(part->bitmap(0), colored);
  }

  IndicatorsGeneration& add(const app::Color& color) {
    auto theme = SkinTheme::instance();

    // Eyedropper icon
    add(theme->getToolIcon("eyedropper"), false);

    // Color
    m_indicators->addColorIndicator(color);

    // Color description
    std::string str = color.toHumanReadableString(
      app_get_current_pixel_format(),
      app::Color::LongHumanReadableString);
    if (color.getAlpha() < 255) {
      char buf[256];
      sprintf(buf, " \xCE\xB1%d", color.getAlpha());
      str += buf;
    }
    m_indicators->addTextIndicator(str.c_str());

    return *this;
  }

  IndicatorsGeneration& add(tools::Tool* tool) {
    auto theme = SkinTheme::instance();

    // Tool icon + text
    add(theme->getToolIcon(tool->getId().c_str()), false);
    m_indicators->addTextIndicator(tool->getText().c_str());

    // Tool shortcut
    Key* key = KeyboardShortcuts::instance()->tool(tool);
    if (key && !key->accels().empty()) {
      add(theme->parts.iconKey()->bitmap(0), true);
      m_indicators->addTextIndicator(key->accels().front().toString().c_str());
    }
    return *this;
  }

private:
  StatusBar::Indicators* m_indicators;
};

class StatusBar::CustomizedTipWindow : public ui::TipWindow {
public:
  CustomizedTipWindow(const std::string& text)
    : ui::TipWindow(text)
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
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kTimerMessage:
        closeWindow(NULL);
        break;
    }
    return ui::TipWindow::onProcessMessage(msg);
  }

private:
  base::UniquePtr<ui::Timer> m_timer;
};

// TODO Use a ui::TipWindow with rounded borders, when we add support
//      to invalidate transparent windows.
class StatusBar::SnapToGridWindow : public ui::PopupWindow {
public:
  SnapToGridWindow()
    : ui::PopupWindow("", ClickBehavior::DoNothingOnClick)
    , m_button("Disable Snap to Grid") {
    setBorder(gfx::Border(2 * guiscale()));
    setBgColor(gfx::rgba(255, 255, 200));
    makeFloating();

    addChild(&m_button);
    m_button.Click.connect(base::Bind<void>(&SnapToGridWindow::onDisableSnapToGrid, this));
  }

  void setDocument(app::Document* doc) {
    m_doc = doc;
  }

private:
  void onDisableSnapToGrid() {
    Preferences::instance().document(m_doc).grid.snap(false);
    closeWindow(nullptr);
  }

  app::Document* m_doc;
  ui::Button m_button;
};

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
        if (Preferences::instance().statusBar.focusFrameFieldOnMouseover()) {
          requestFocus();
          selectText(0, -1);
        }
        break;

      case kKeyDownMessage: {
        KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keymsg->scancode();

        if (hasFocus() &&
            (scancode == kKeyEnter || // TODO customizable keys
             scancode == kKeyEnterPad)) {
          Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoFrame);
          Params params;
          int frame = textInt();
          if (frame > 0) {
            params.set("frame", text().c_str());
            UIContext::instance()->executeCommand(cmd, params);
          }
          // Select the text again
          selectText(0, -1);
          releaseFocus();
          return true;          // Key used.
        }
        break;
      }
    }

    bool result = Entry::onProcessMessage(msg);

    if (msg->type() == kMouseDownMessage)
      selectText(0, -1);

    return result;
  }

};

StatusBar* StatusBar::m_instance = NULL;

StatusBar::StatusBar()
  : m_timeout(0)
  , m_indicators(new Indicators)
  , m_docControls(new HBox)
  , m_doc(nullptr)
  , m_tipwindow(nullptr)
  , m_snapToGridWindow(nullptr)
{
  m_instance = this;

  setDoubleBuffered(true);

  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  setBgColor(theme->colors.statusBarFace());

  setFocusStop(true);
  setBorder(gfx::Border(6*guiscale(), 0, 6*guiscale(), 0));

  setMinSize(Size(0, textHeight()+8*guiscale()));
  setMaxSize(Size(INT_MAX, textHeight()+8*guiscale()));

  m_indicators->setExpansive(true);
  m_docControls->setVisible(false);
  addChild(m_indicators);
  addChild(m_docControls);

  // Construct the commands box
  {
    Box* box1 = new Box(HORIZONTAL);
    Box* box4 = new Box(HORIZONTAL);

    m_frameLabel = new Label("Frame:");
    m_currentFrame = new GotoFrameEntry();
    m_newFrame = new Button("+");
    m_newFrame->Click.connect(base::Bind<void>(&StatusBar::newFrame, this));
    m_zoomEntry = new ZoomEntry;
    m_zoomEntry->ZoomChange.connect(&StatusBar::onChangeZoom, this);

    setup_mini_look(m_currentFrame);
    setup_mini_look(m_newFrame);

    box1->setBorder(gfx::Border(2, 1, 2, 2)*guiscale());

    box4->addChild(m_currentFrame);
    box4->addChild(m_newFrame);

    box1->addChild(m_frameLabel);
    box1->addChild(box4);
    box1->addChild(m_zoomEntry);

    m_docControls->addChild(box1);
  }

  // Tooltips manager
  TooltipManager* tooltipManager = new TooltipManager();
  addChild(tooltipManager);
  tooltipManager->addTooltipFor(m_currentFrame, "Current Frame", BOTTOM);
  tooltipManager->addTooltipFor(m_zoomEntry, "Zoom Level", BOTTOM);

  UIContext::instance()->addObserver(this);
  UIContext::instance()->documents().addObserver(this);
  App::instance()->activeToolManager()->addObserver(this);
}

StatusBar::~StatusBar()
{
  App::instance()->activeToolManager()->removeObserver(this);
  UIContext::instance()->documents().removeObserver(this);
  UIContext::instance()->removeObserver(this);

  delete m_tipwindow;           // widget
  delete m_snapToGridWindow;
}

void StatusBar::onSelectedToolChange(tools::Tool* tool)
{
  if (isVisible() && tool)
    showTool(500, tool);
}

void StatusBar::clearText()
{
  setStatusText(1, "");
}

void StatusBar::updateFromEditor(Editor* editor)
{
  if (editor)
    m_zoomEntry->setZoom(editor->zoom());
}

bool StatusBar::setStatusText(int msecs, const char *format, ...)
{
  if ((base::current_tick() > m_timeout) || (msecs > 0)) {
    char buf[256];              // TODO warning buffer overflow
    va_list ap;

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    IndicatorsGeneration(m_indicators).add(buf);
    m_timeout = base::current_tick() + msecs;
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

  x = bounds().x2() - m_tipwindow->bounds().w;
  y = bounds().y - m_tipwindow->bounds().h;
  m_tipwindow->positionWindow(x, y);

  m_tipwindow->startTimer();

  // Set the text in status-bar (with inmediate timeout)
  IndicatorsGeneration(m_indicators).add(buf);
  m_timeout = base::current_tick();
}

void StatusBar::showColor(int msecs, const char* text, const app::Color& color)
{
  if ((base::current_tick() > m_timeout) || (msecs > 0)) {
    IndicatorsGeneration gen(m_indicators);
    gen.add(color);
    if (text)
      gen.add(text);

    m_timeout = base::current_tick() + msecs;
  }
}

void StatusBar::showTool(int msecs, tools::Tool* tool)
{
  ASSERT(tool != NULL);
  IndicatorsGeneration(m_indicators).add(tool);

  m_timeout = base::current_tick() + msecs;
}

void StatusBar::showSnapToGridWarning(bool state)
{
  if (state) {
    ASSERT(m_doc);
    if (!m_doc)
      return;

    if (!m_snapToGridWindow) {
      m_snapToGridWindow = new SnapToGridWindow;
    }

    if (!m_snapToGridWindow->isVisible()) {
      m_snapToGridWindow->openWindow();
      m_snapToGridWindow->remapWindow();

      Rect rc = bounds();
      int toolBarWidth = ToolBar::instance()->sizeHint().w;

      m_snapToGridWindow->positionWindow(
        rc.x+rc.w-toolBarWidth-m_snapToGridWindow->bounds().w,
        rc.y-m_snapToGridWindow->bounds().h);
    }

    m_snapToGridWindow->setDocument(
      static_cast<app::Document*>(m_doc));
  }
  else {
    if (m_snapToGridWindow)
      m_snapToGridWindow->closeWindow(nullptr);
  }
}

//////////////////////////////////////////////////////////////////////
// StatusBar message handler

void StatusBar::onResize(ResizeEvent& ev)
{
  Rect rc = ev.bounds();
  m_docControls->setVisible(m_doc && rc.w > 300*ui::guiscale());

  HBox::onResize(ev);
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

    m_docControls->setVisible(true);
    showSnapToGridWarning(
      Preferences::instance().document(
        static_cast<app::Document*>(m_doc)).grid.snap());

    // Current frame
    m_currentFrame->setTextf("%d", site.frame()+1);
  }
  else {
    ASSERT(m_doc == nullptr);
    m_docControls->setVisible(false);
    showSnapToGridWarning(false);
  }
  layout();
}

void StatusBar::onRemoveDocument(doc::Document* doc)
{
  if (m_doc &&
      m_doc == doc) {
    m_doc->removeObserver(this);
    m_doc = nullptr;
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

void StatusBar::onChangeZoom(const render::Zoom& zoom)
{
  if (current_editor)
    current_editor->setEditorZoom(zoom);
}

} // namespace app
