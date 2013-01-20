/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "app.h"
#include "base/bind.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "gfx/size.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "skin/skin_theme.h"
#include "tools/tool.h"
#include "ui/gui.h"
#include "ui_context.h"
#include "undo/undo_history.h"
#include "util/misc.h"
#include "widgets/color_button.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

#include <algorithm>
#include <allegro.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

using namespace gfx;
using namespace ui;

enum AniAction {
  ACTION_FIRST,
  ACTION_PREV,
  ACTION_PLAY,
  ACTION_NEXT,
  ACTION_LAST,
};

class StatusBar::CustomizedTipWindow : public ui::TipWindow
{
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
  UniquePtr<ui::Timer> m_timer;
};

static void slider_change_hook(Slider* slider);
static void ani_button_command(Button* widget, AniAction action);

static int statusbar_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

// This widget is used to show the current frame.
class GotoFrameEntry : public Entry {
public:
  GotoFrameEntry() : Entry(4, "") {
  }

  bool onProcessMessage(Message* msg) OVERRIDE {
    switch (msg->type) {
      // When the mouse enter in this entry, it got the focus and the
      // text is automatically selected.
      case JM_MOUSEENTER:
        requestFocus();
        selectText(0, -1);
        break;

      case JM_KEYPRESSED:
        if (msg->key.scancode == KEY_ENTER ||
            msg->key.scancode == KEY_ENTER_PAD) {
          Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoFrame);
          Params params;
          int frame = strtol(this->getText(), NULL, 10);
          if (frame > 0) {
            params.set("frame", this->getText());
            UIContext::instance()->executeCommand(cmd, &params);
          }
          // Select the text again
          selectText(0, -1);
          return true;          // Key used.
        }
        break;
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

#define BUTTON_NEW(name, text, action)                                  \
  {                                                                     \
    (name) = new Button(text);                                          \
    (name)->user_data[0] = this;                                        \
    setup_mini_look(name);                                              \
    (name)->Click.connect(Bind<void>(&ani_button_command, (name), action)); \
  }

#define ICON_NEW(name, icon, action)                                    \
  {                                                                     \
    BUTTON_NEW((name), NULL, (action));                                 \
    set_gfxicon_to_button((name), icon, icon##_SELECTED, icon##_DISABLED, JI_CENTER | JI_MIDDLE); \
  }

  this->setFocusStop(true);

  m_timeout = 0;
  m_state = SHOW_TEXT;
  m_tipwindow = NULL;
  m_hot_layer = LayerIndex(-1);

  // The extra pixel in left and right borders are necessary so
  // m_commandsBox and m_movePixelsBox do not overlap the upper-left
  // and upper-right pixels drawn in JM_DRAW message (see putpixels)
  jwidget_set_border(this, 1*jguiscale(), 0, 1*jguiscale(), 0);

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
    jwidget_set_min_size(m_slider, JI_SCREEN_W/5, 0);

    jwidget_set_border(box1, 2*jguiscale(), 1*jguiscale(), 2*jguiscale(), 2*jguiscale());
    jwidget_noborders(box2);
    jwidget_noborders(box3);
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

    jwidget_set_border(box1, 2*jguiscale(), 1*jguiscale(), 2*jguiscale(), 2*jguiscale());
    jwidget_noborders(box2);
    box2->setExpansive(true);

    m_linkLabel = new LinkLabel((std::string(WEBSITE) + "donate/").c_str(), "Support This Project");

    box1->addChild(box2);
    box1->addChild(m_linkLabel);
    m_notificationsBox = box1;
  }

  // Construct move-pixels box
  {
    Box* filler = new Box(JI_HORIZONTAL);
    filler->setExpansive(true);

    m_movePixelsBox = new Box(JI_HORIZONTAL);
    m_transparentLabel = new Label("Transparent Color:");
    m_transparentColor = new ColorButton(app::Color::fromMask(), IMAGE_RGB);

    m_movePixelsBox->addChild(filler);
    m_movePixelsBox->addChild(m_transparentLabel);
    m_movePixelsBox->addChild(m_transparentColor);

    m_transparentColor->Change.connect(Bind<void>(&StatusBar::onTransparentColorChange, this));
  }

  addChild(m_notificationsBox);

  App::instance()->CurrentToolChange.connect(&StatusBar::onCurrentToolChange, this);
}

StatusBar::~StatusBar()
{
  for (ProgressList::iterator it = m_progress.begin(); it != m_progress.end(); ++it)
    delete *it;

  delete m_tipwindow;           // widget
  delete m_movePixelsBox;
  delete m_commandsBox;
  delete m_notificationsBox;
}

void StatusBar::addObserver(StatusBarObserver* observer)
{
  m_observers.addObserver(observer);
}

void StatusBar::removeObserver(StatusBarObserver* observer)
{
  m_observers.removeObserver(observer);
}

void StatusBar::onCurrentToolChange()
{
  if (isVisible()) {
    tools::Tool* currentTool = UIContext::instance()->getSettings()->getCurrentTool();
    if (currentTool) {
      showTool(500, currentTool);
      setTextf("%s Selected", currentTool->getText().c_str());
    }
  }
}

void StatusBar::onTransparentColorChange()
{
  m_observers.notifyObservers<const app::Color&>(&StatusBarObserver::onChangeTransparentColor,
                                                 getTransparentColor());
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

  x = this->rc->x2 - jrect_w(m_tipwindow->rc);
  y = this->rc->y1 - jrect_h(m_tipwindow->rc);
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

void StatusBar::showMovePixelsOptions()
{
  if (!hasChild(m_movePixelsBox)) {
    addChild(m_movePixelsBox);
    invalidate();
  }
}

void StatusBar::hideMovePixelsOptions()
{
  if (hasChild(m_movePixelsBox)) {
    removeChild(m_movePixelsBox);
    invalidate();
  }
}

app::Color StatusBar::getTransparentColor()
{
  return m_transparentColor->getColor();
}

void StatusBar::showNotification(const char* text, const char* link)
{
  m_linkLabel->setText(text);
  m_linkLabel->setUrl(link);
  layout();
  invalidate();
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
  switch (msg->type) {

    case JM_SETPOS:
      jrect_copy(this->rc, &msg->setpos.rect);
      {
        JRect rc = jrect_new_copy(this->rc);
        rc->x1 = rc->x2 - m_notificationsBox->getPreferredSize().w;
        jwidget_set_rect(m_notificationsBox, rc);
        jrect_free(rc);
      }
      {
        JRect rc = jrect_new_copy(this->rc);
        rc->x2 -= jrect_w(rc)/4 + 4*jguiscale();
        jwidget_set_rect(m_commandsBox, rc);
        jrect_free(rc);
      }
      {
        JRect rc = jrect_new_copy(this->rc);
        Size reqSize = m_movePixelsBox->getPreferredSize();
        rc->x1 = rc->x2 - reqSize.w;
        rc->x2 -= this->border_width.r;
        jwidget_set_rect(m_movePixelsBox, rc);
        jrect_free(rc);
      }
      return true;

    case JM_DRAW: {
      SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
      ui::Color text_color = theme->getColor(ThemeColor::Text);
      ui::Color face_color = theme->getColor(ThemeColor::Face);
      JRect rc = jwidget_get_rect(this);
      BITMAP* doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
                                           jrect_h(&msg->draw.rect));
      jrect_displace(rc,
                     -msg->draw.rect.x1,
                     -msg->draw.rect.y1);

      clear_to_color(doublebuffer, to_system(face_color));

      rc->x1 += 2*jguiscale();
      rc->y1 += 1*jguiscale();
      rc->x2 -= 2*jguiscale();
      rc->y2 -= 2*jguiscale();

      int x = rc->x1+4*jguiscale();

      // Color
      if (m_state == SHOW_COLOR) {
        // Draw eyedropper icon
        BITMAP* icon = theme->get_toolicon("eyedropper");
        if (icon) {
          set_alpha_blender();
          draw_trans_sprite(doublebuffer, icon,
                            x, (rc->y1+rc->y2)/2-icon->h/2);

          x += icon->w + 4*jguiscale();
        }

        // Draw color
        draw_color_button(doublebuffer, Rect(x, rc->y1, 32*jguiscale(), rc->y2-rc->y1),
                          true, true, true, true,
                          true, true, true, true,
                          app_get_current_pixel_format(), m_color,
                          false, false);

        x += (32+4)*jguiscale();

        // Draw color description
        std::string str = m_color.toHumanReadableString(app_get_current_pixel_format(),
                                                        app::Color::LongHumanReadableString);
        if (m_alpha < 255) {
          char buf[512];
          usprintf(buf, ", Alpha %d", m_alpha);
          str += buf;
        }

        textout_ex(doublebuffer, this->getFont(), str.c_str(),
                   x, (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
                   to_system(text_color), -1);

        x += ji_font_text_len(this->getFont(), str.c_str()) + 4*jguiscale();
      }

      // Show tool
      if (m_state == SHOW_TOOL) {
        // Draw eyedropper icon
        BITMAP* icon = theme->get_toolicon(m_tool->getId().c_str());
        if (icon) {
          set_alpha_blender();
          draw_trans_sprite(doublebuffer, icon,
                            x, (rc->y1+rc->y2)/2-icon->h/2);

          x += icon->w + 4*jguiscale();
        }
      }

      // Status bar text
      if (this->getTextSize() > 0) {
        textout_ex(doublebuffer, this->getFont(), this->getText(),
                   x,
                   (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
                   to_system(text_color), -1);

        x += ji_font_text_len(this->getFont(), this->getText()) + 4*jguiscale();
      }

      // Draw progress bar
      if (!m_progress.empty()) {
        int width = 64;
        int y1, y2;
        int x = rc->x2 - (width+4);

        y1 = rc->y1;
        y2 = rc->y2-1;

        for (ProgressList::iterator it = m_progress.begin(); it != m_progress.end(); ++it) {
          Progress* progress = *it;

          theme->drawProgressBar(doublebuffer,
                                 x, y1, x+width-1, y2,
                                 progress->getPos());

          x -= width+4;
        }
      }
      // Show layers only when we are not moving pixels
      else if (!hasChild(m_movePixelsBox)) {
        // Available width for layers buttons
        int width = jrect_w(rc)/4;

        // Draw layers
        try {
          --rc->y2;

          const ActiveDocumentReader document(UIContext::instance());
          const Sprite* sprite(document ? document->getSprite(): NULL);
          if (sprite) {
            if (hasChild(m_notificationsBox)) {
              removeChild(m_notificationsBox);
              invalidate();
            }

            const LayerFolder* folder = sprite->getFolder();
            LayerConstIterator it = folder->getLayerBegin();
            LayerConstIterator end = folder->getLayerEnd();
            int count = folder->getLayersCount();
            char buf[256];

            for (int c=0; it != end; ++it, ++c) {
              int x1 = rc->x2-width + c*width/count;
              int x2 = rc->x2-width + (c+1)*width/count;
              bool hot = (*it == sprite->getCurrentLayer())
                || (LayerIndex(c) == m_hot_layer);

              theme->draw_bounds_nw(doublebuffer,
                                    x1, rc->y1, x2, rc->y2,
                                    hot ? PART_TOOLBUTTON_HOT_NW:
                                          PART_TOOLBUTTON_NORMAL_NW,
                                    hot ? theme->getColor(ThemeColor::ButtonHotFace):
                                          theme->getColor(ThemeColor::ButtonNormalFace));

              if (count == 1)
                uszprintf(buf, sizeof(buf), "%s", (*it)->getName().c_str());
              else
                {
                  if (c+'A' <= 'Z')
                    usprintf(buf, "%c", c+'A');
                  else
                    usprintf(buf, "%d", c-('Z'-'A'));
                }

              textout_centre_ex(doublebuffer, this->getFont(), buf,
                                (x1+x2)/2,
                                (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
                                to_system(hot ? theme->getColor(ThemeColor::ButtonHotText):
                                                theme->getColor(ThemeColor::ButtonNormalText)),
                                -1);
            }
          }
          else {
            if (!hasChild(m_notificationsBox)) {
              addChild(m_notificationsBox);
              invalidate();
            }
          }
        }
        catch (LockedDocumentException&) {
          // Do nothing...
        }
      }

      jrect_free(rc);

      blit(doublebuffer, ji_screen, 0, 0,
           msg->draw.rect.x1,
           msg->draw.rect.y1,
           doublebuffer->w,
           doublebuffer->h);
      destroy_bitmap(doublebuffer);
      return true;
    }

    case JM_MOUSEENTER: {
      const Document* document = UIContext::instance()->getActiveDocument();
      bool state = (document != NULL);

      if (!hasChild(m_movePixelsBox)) {
        if (state && !hasChild(m_commandsBox)) {
          updateCurrentFrame();

          m_b_first->setEnabled(state);
          m_b_prev->setEnabled(state);
          m_b_play->setEnabled(state);
          m_b_next->setEnabled(state);
          m_b_last->setEnabled(state);

          updateFromLayer();

          if (hasChild(m_notificationsBox))
            removeChild(m_notificationsBox);

          addChild(m_commandsBox);
          invalidate();
        }
        else if (!state && !hasChild(m_notificationsBox)) {
          addChild(m_notificationsBox);
          invalidate();
        }
      }
      break;
    }

    case JM_MOTION: {
      JRect rc = jwidget_get_rect(this);

      rc->x1 += 2*jguiscale();
      rc->y1 += 1*jguiscale();
      rc->x2 -= 2*jguiscale();
      rc->y2 -= 2*jguiscale();

      // Available width for layers buttons
      int width = jrect_w(rc)/4;

      // Check layers bounds
      try {
        --rc->y2;

        LayerIndex hot_layer = LayerIndex(-1);

        const ActiveDocumentReader document(UIContext::instance());
        const Sprite* sprite(document ? document->getSprite(): NULL);
        // Check which sprite's layer has the mouse over
        if (sprite) {
          const LayerFolder* folder = sprite->getFolder();
          LayerConstIterator it = folder->getLayerBegin();
          LayerConstIterator end = folder->getLayerEnd();
          int count = folder->getLayersCount();

          for (int c=0; it != end; ++it, ++c) {
            int x1 = rc->x2-width + c*width/count;
            int x2 = rc->x2-width + (c+1)*width/count;

            if (Rect(Point(x1, rc->y1),
                     Point(x2, rc->y2)).contains(Point(msg->mouse.x,
                                                       msg->mouse.y))) {
              hot_layer = LayerIndex(c);
              break;
            }
          }
        }
        // Check if the "Donate" button has the mouse over
        else {
          int x1 = rc->x2-width;
          int x2 = rc->x2;

          if (Rect(Point(x1, rc->y1),
                   Point(x2, rc->y2)).contains(Point(msg->mouse.x,
                                                     msg->mouse.y))) {
            hot_layer = LayerIndex(0);
          }
        }

        if (m_hot_layer != hot_layer) {
          m_hot_layer = hot_layer;
          invalidate();
        }
      }
      catch (LockedDocumentException&) {
        // Do nothing...
      }

      jrect_free(rc);
      break;
    }

    case JM_BUTTONPRESSED:
      // When the user press the mouse-button over a hot-layer-button...
      if (m_hot_layer >= 0) {
        try {
          ActiveDocumentWriter document(UIContext::instance());
          Sprite* sprite(document ? document->getSprite(): NULL);
          if (sprite) {
            Layer* layer = sprite->indexToLayer(m_hot_layer);
            if (layer) {
              // Set the current layer
              if (layer != sprite->getCurrentLayer())
                sprite->setCurrentLayer(layer);

              // Flash the current layer
              if (current_editor != NULL)
                current_editor->flashCurrentLayer();

              // Redraw the status-bar
              invalidate();
            }
          }
        }
        catch (LockedDocumentException&) {
          // Do nothing...
        }
      }
      break;

    case JM_MOUSELEAVE:
      if (hasChild(m_commandsBox)) {
        // If we want restore the state-bar and the slider doesn't have
        // the capture...
        if (getManager()->getCapture() != m_slider) {
          // ...exit from command mode
          getManager()->freeFocus();     // TODO Review this code

          removeChild(m_commandsBox);
          invalidate();
        }

        if (m_hot_layer >= 0) {
          m_hot_layer = LayerIndex(-1);
          invalidate();
        }
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void StatusBar::onPreferredSize(PreferredSizeEvent& ev)
{
  int s = 4*jguiscale() + jwidget_get_text_height(this) + 4*jguiscale();
  ev.setPreferredSize(Size(s, s));
}

bool StatusBar::CustomizedTipWindow::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_TIMER:
      closeWindow(NULL);
      break;
  }

  return ui::TipWindow::onProcessMessage(msg);
}

static void slider_change_hook(Slider* slider)
{
  try {
    ActiveDocumentWriter document(UIContext::instance());
    Sprite* sprite(document ? document->getSprite(): NULL);
    if (sprite) {
      if ((sprite->getCurrentLayer()) &&
          (sprite->getCurrentLayer()->isImage())) {
        Cel* cel = ((LayerImage*)sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame());
        if (cel) {
          // Update the opacity
          cel->setOpacity(slider->getValue());

          // Update the editors
          update_screen_for_document(document);
        }
      }
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
    const ActiveDocumentReader document(UIContext::instance());
    const Sprite* sprite(document ? document->getSprite(): NULL);
    Cel *cel;

    // Opacity layer
    if (sprite &&
        sprite->getCurrentLayer() &&
        sprite->getCurrentLayer()->isImage() &&
        !sprite->getCurrentLayer()->isBackground() &&
        (cel = ((LayerImage*)sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame()))) {
      m_slider->setValue(MID(0, cel->getOpacity(), 255));
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
  const Document* document = UIContext::instance()->getActiveDocument();
  if (document)
    m_currentFrame->setTextf("%d", document->getSprite()->getCurrentFrame()+1);
}

void StatusBar::newFrame()
{
  Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::NewFrame);
  UIContext::instance()->executeCommand(cmd);
  updateCurrentFrame();
}
