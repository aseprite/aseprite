/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/settings.h"
#include "app/tools/tool.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/misc.h"
#include "base/bind.h"
#include "gfx/size.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/ui.h"
#include "undo/undo_history.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace raster;

enum AniAction {
  ACTION_FIRST,
  ACTION_PREV,
  ACTION_PLAY,
  ACTION_NEXT,
  ACTION_LAST,
};

static const char* kStatusBarText = "status_bar_text";
static const char* kStatusBarFace = "status_bar_face";

class StatusBar::CustomizedTipWindow : public ui::TipWindow {
public:
  CustomizedTipWindow(const char* text)
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
  bool onProcessMessage(Message* msg);

private:
  base::UniquePtr<ui::Timer> m_timer;
};

static void slider_change_hook(Slider* slider);
static void ani_button_command(Button* widget, AniAction action);

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

  bool onProcessMessage(Message* msg) OVERRIDE {
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
        
        if (scancode == kKeyEnter || // TODO customizable keys
            scancode == kKeyEnterPad) {
          Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoFrame);
          Params params;
          int frame = getTextInt();
          if (frame > 0) {
            params.set("frame", getText().c_str());
            UIContext::instance()->executeCommand(cmd, &params);
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
{
  m_instance = this;

  setDoubleBuffered(true);

  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  setBgColor(theme->getColorById(kStatusBarFace));

#define BUTTON_NEW(name, text, action)                                  \
  {                                                                     \
    (name) = new Button(text);                                          \
    (name)->user_data[0] = this;                                        \
    setup_mini_look(name);                                              \
    (name)->Click.connect(Bind<void>(&ani_button_command, (name), action)); \
  }

#define ICON_NEW(name, icon, action)                                    \
  {                                                                     \
    BUTTON_NEW((name), "", (action));                                   \
    set_gfxicon_to_button((name), icon, icon##_SELECTED, icon##_DISABLED, JI_CENTER | JI_MIDDLE); \
  }

  this->setFocusStop(true);

  m_timeout = 0;
  m_state = SHOW_TEXT;
  m_tipwindow = NULL;

  // The extra pixel in left and right borders are necessary so
  // m_commandsBox and m_movePixelsBox do not overlap the upper-left
  // and upper-right pixels drawn in onPaint() event (see putpixels)
  setBorder(gfx::Border(1*jguiscale(), 0, 1*jguiscale(), 0));

  // Construct the commands box
  {
    Box* box1 = new Box(JI_HORIZONTAL);
    Box* box2 = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);
    Box* box3 = new Box(JI_HORIZONTAL);
    Box* box4 = new Box(JI_HORIZONTAL);
    m_slider = new Slider(0, 255, 255);
    m_currentFrame = new GotoFrameEntry();
    m_newFrame = new Button("+");
    m_newFrame->Click.connect(Bind<void>(&StatusBar::newFrame, this));

    setup_mini_look(m_slider);
    setup_mini_look(m_currentFrame);
    setup_mini_look(m_newFrame);

    ICON_NEW(m_b_first, PART_ANI_FIRST, ACTION_FIRST);
    ICON_NEW(m_b_prev, PART_ANI_PREVIOUS, ACTION_PREV);
    ICON_NEW(m_b_play, PART_ANI_PLAY, ACTION_PLAY);
    ICON_NEW(m_b_next, PART_ANI_NEXT, ACTION_NEXT);
    ICON_NEW(m_b_last, PART_ANI_LAST, ACTION_LAST);

    m_slider->Change.connect(Bind<void>(&slider_change_hook, m_slider));
    m_slider->setMinSize(gfx::Size(JI_SCREEN_W/5, 0));

    box1->setBorder(gfx::Border(2, 1, 2, 2)*jguiscale());
    box2->noBorderNoChildSpacing();
    box3->noBorderNoChildSpacing();
    box3->setExpansive(true);

    box4->addChild(m_currentFrame);
    box4->addChild(m_newFrame);

    box2->addChild(m_b_first);
    box2->addChild(m_b_prev);
    box2->addChild(m_b_play);
    box2->addChild(m_b_next);
    box2->addChild(m_b_last);

    box1->addChild(box3);
    box1->addChild(box4);
    box1->addChild(box2);
    box1->addChild(m_slider);

    m_commandsBox = box1;
  }

  // Create the box to show notifications.
  {
    Box* box1 = new Box(JI_HORIZONTAL);
    Box* box2 = new Box(JI_VERTICAL);

    box1->setBorder(gfx::Border(2, 1, 2, 2)*jguiscale());
    box2->noBorderNoChildSpacing();
    box2->setExpansive(true);

    m_linkLabel = new LinkLabel((std::string(WEBSITE) + "donate/").c_str(), "Support This Project");

    box1->addChild(box2);
    box1->addChild(m_linkLabel);
    m_notificationsBox = box1;
  }

  addChild(m_notificationsBox);

  App::instance()->CurrentToolChange.connect(&StatusBar::onCurrentToolChange, this);
}

StatusBar::~StatusBar()
{
  for (ProgressList::iterator it = m_progress.begin(); it != m_progress.end(); ++it)
    delete *it;

  delete m_tipwindow;           // widget
  delete m_commandsBox;
  delete m_notificationsBox;
}

void StatusBar::onCurrentToolChange()
{
  if (isVisible()) {
    tools::Tool* currentTool = UIContext::instance()->settings()->getCurrentTool();
    if (currentTool) {
      showTool(500, currentTool);
      setTextf("%s Selected", currentTool->getText().c_str());
    }
  }
}

void StatusBar::clearText()
{
  setStatusText(1, "");
}

bool StatusBar::setStatusText(int msecs, const char *format, ...)
{
  // TODO this call should be in an observer of the "current frame" property changes.
  updateCurrentFrame();

  if ((ji_clock > m_timeout) || (msecs > 0)) {
    char buf[256];              // TODO warning buffer overflow
    va_list ap;

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    m_timeout = ji_clock + msecs;
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
  m_timeout = ji_clock;
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
  Accelerator* accel = get_accel_to_change_tool(tool);
  if (accel) {
    text += ", Shortcut: ";
    text += accel->toString();
  }

  // Set text
  if (setStatusText(msecs, text.c_str())) {
    // Show tool
    m_state = SHOW_TOOL;
    m_tool = tool;
  }
}

//////////////////////////////////////////////////////////////////////
// Progress bars stuff

Progress* StatusBar::addProgress()
{
  Progress* progress = new Progress(this);
  m_progress.push_back(progress);
  invalidate();
  return progress;
}

void StatusBar::removeProgress(Progress* progress)
{
  ASSERT(progress->m_statusbar == this);

  ProgressList::iterator it = std::find(m_progress.begin(), m_progress.end(), progress);
  ASSERT(it != m_progress.end());

  m_progress.erase(it);
  invalidate();
}

Progress::Progress(StatusBar* statusbar)
  : m_statusbar(statusbar)
  , m_pos(0.0f)
{
}

Progress::~Progress()
{
  if (m_statusbar) {
    m_statusbar->removeProgress(this);
    m_statusbar = NULL;
  }
}

void Progress::setPos(double pos)
{
  if (m_pos != pos) {
    m_pos = pos;
    m_statusbar->invalidate();
  }
}

double Progress::getPos() const
{
  return m_pos;
}

//////////////////////////////////////////////////////////////////////
// StatusBar message handler

bool StatusBar::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseEnterMessage: {
      updateSubwidgetsVisibility();

      const Document* document = UIContext::instance()->activeDocument();
      if (document != NULL)
        updateCurrentFrame();
      break;
    }

    case kMouseLeaveMessage:
      if (hasChild(m_commandsBox)) {
        // If we want restore the state-bar and the slider doesn't have
        // the capture...
        if (getManager()->getCapture() != m_slider) {
          // ...exit from command mode
          getManager()->freeFocus();     // TODO Review this code

          removeChild(m_commandsBox);
          invalidate();
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void StatusBar::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  Rect rc = ev.getBounds();
  rc.x = rc.x2() - m_notificationsBox->getPreferredSize().w;
  rc.w = m_notificationsBox->getPreferredSize().w;
  m_notificationsBox->setBounds(rc);

  rc = ev.getBounds();
  rc.w -= rc.w/4 + 4*jguiscale();
  m_commandsBox->setBounds(rc);
}

void StatusBar::onPreferredSize(PreferredSizeEvent& ev)
{
  int s = 4*jguiscale() + getTextHeight() + 4*jguiscale();
  ev.setPreferredSize(Size(s, s));
}

void StatusBar::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  gfx::Color textColor = theme->getColorById(kStatusBarText);
  Rect rc = getClientBounds();
  Graphics* g = ev.getGraphics();

  g->fillRect(getBgColor(), rc);

  rc.shrink(Border(2, 1, 2, 2)*jguiscale());

  int x = rc.x + 4*jguiscale();

  // Color
  if (m_state == SHOW_COLOR) {
    // Draw eyedropper icon
    she::Surface* icon = theme->get_toolicon("eyedropper");
    if (icon) {
      g->drawRgbaSurface(icon, x, rc.y + rc.h/2 - icon->height()/2);
      x += icon->width() + 4*jguiscale();
    }

    // Draw color
    draw_color_button(g, gfx::Rect(x, rc.y, 32*jguiscale(), rc.h),
      m_color, false, false);

    x += (32+4)*jguiscale();

    // Draw color description
    std::string str = m_color.toHumanReadableString(app_get_current_pixel_format(),
      app::Color::LongHumanReadableString);
    if (m_alpha < 255) {
      char buf[256];
      sprintf(buf, ", Alpha %d", m_alpha);
      str += buf;
    }

    g->drawString(str, textColor, ColorNone,
      gfx::Point(x, rc.y + rc.h/2 - getFont()->height()/2));

    x += getFont()->textLength(str.c_str()) + 4*jguiscale();
  }

  // Show tool
  if (m_state == SHOW_TOOL) {
    // Draw eyedropper icon
    she::Surface* icon = theme->get_toolicon(m_tool->getId().c_str());
    if (icon) {
      g->drawRgbaSurface(icon, x, rc.y + rc.h/2 - icon->height()/2);
      x += icon->width() + 4*jguiscale();
    }
  }

  // Status bar text
  if (getTextLength() > 0) {
    g->drawString(getText(), textColor, ColorNone,
      gfx::Point(x, rc.y + rc.h/2 - getFont()->height()/2));

    x += getFont()->textLength(getText().c_str()) + 4*jguiscale();
  }

  // Draw progress bar
  if (!m_progress.empty()) {
    int width = 64;
    int x = rc.x2() - (width+4);

    for (ProgressList::iterator it = m_progress.begin(); it != m_progress.end(); ++it) {
      Progress* progress = *it;

      theme->paintProgressBar(g,
        gfx::Rect(x, rc.y, width, rc.h),
        progress->getPos());

      x -= width+4;
    }
  }

  updateSubwidgetsVisibility();
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

static void slider_change_hook(Slider* slider)
{
  try {
    ContextWriter writer(UIContext::instance());
    Cel* cel = writer.cel();
    if (cel) {
      // Update the opacity
      cel->setOpacity(slider->getValue());

      // Update the editors
      update_screen_for_document(writer.document());
    }
  }
  catch (LockedDocumentException&) {
    // do nothing
  }
}

static void ani_button_command(Button* widget, AniAction action)
{
  Command* cmd = NULL;

  switch (action) {
    //case ACTION_LAYER: cmd = CommandsModule::instance()->getCommandByName(CommandId::LayerProperties); break;
    case ACTION_FIRST: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoFirstFrame); break;
    case ACTION_PREV: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoPreviousFrame); break;
    case ACTION_PLAY: cmd = CommandsModule::instance()->getCommandByName(CommandId::PlayAnimation); break;
    case ACTION_NEXT: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoNextFrame); break;
    case ACTION_LAST: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoLastFrame); break;
  }

  if (cmd)
    UIContext::instance()->executeCommand(cmd);
}

void StatusBar::updateFromLayer()
{
  try {
    const ContextReader reader(UIContext::instance());
    const Cel* cel;

    // Opacity layer
    if (reader.layer() &&
        reader.layer()->isImage() &&
        !reader.layer()->isBackground() &&
        (cel = reader.cel())) {
      m_slider->setValue(MID(0, cel->opacity(), 255));
      m_slider->setEnabled(true);
    }
    else {
      m_slider->setValue(255);
      m_slider->setEnabled(false);
    }
  }
  catch (LockedDocumentException&) {
    // Disable all
    m_slider->setEnabled(false);
  }
}

void StatusBar::updateCurrentFrame()
{
  DocumentLocation location = UIContext::instance()->activeLocation();
  if (location.sprite())
    m_currentFrame->setTextf("%d", location.frame()+1);
}

void StatusBar::newFrame()
{
  Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::NewFrame);
  UIContext::instance()->executeCommand(cmd);
  updateCurrentFrame();
}

void StatusBar::updateSubwidgetsVisibility()
{
  const Document* document = UIContext::instance()->activeDocument();
  bool commandsVisible = (document != NULL && hasMouse());
  bool notificationsVisible = (document == NULL);

  if (commandsVisible) {
    if (!hasChild(m_commandsBox)) {
      addChild(m_commandsBox);
      invalidate();
    }
  }
  else {
    if (hasChild(m_commandsBox)) {
      removeChild(m_commandsBox);
      invalidate();
    }
  }

  if (notificationsVisible) {
    if (!hasChild(m_notificationsBox)) {
      addChild(m_notificationsBox);
      invalidate();
    }
  }
  else {
    if (hasChild(m_notificationsBox)) {
      removeChild(m_notificationsBox);
      invalidate();
    }
  }
}

} // namespace app
