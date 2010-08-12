/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <allegro.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "jinete/jinete.h"
#include "Vaca/Size.h"

#include "app.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "core/core.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/skinneable_theme.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "sprite_wrappers.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "util/misc.h"
#include "widgets/colbut.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"

enum {
  ACTION_FIRST,
  ACTION_PREV,
  ACTION_PLAY,
  ACTION_NEXT,
  ACTION_LAST,
};

static bool tipwindow_msg_proc(JWidget widget, JMessage msg);

static bool transparent_color_change_hook(JWidget widget, void *data);
static bool slider_change_hook(JWidget widget, void *data);
static void button_command(JWidget widget, void *data);

static int statusbar_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

StatusBar::StatusBar()
  : Widget(statusbar_type())
{
#define BUTTON_NEW(name, text, data)					\
  {									\
    (name) = jbutton_new(text);						\
    (name)->user_data[0] = this;					\
    setup_mini_look(name);						\
    jbutton_add_command_data((name), button_command, (void *)(data));	\
  }

#define ICON_NEW(name, icon, action)					\
  {									\
    BUTTON_NEW((name), NULL, (action));					\
    add_gfxicon_to_button((name), (icon), JI_CENTER | JI_MIDDLE);	\
  }

  jwidget_focusrest(this, true);

  m_timeout = 0;
  m_state = SHOW_TEXT;
  m_progress = jlist_new();
  m_tipwindow = NULL;
  m_hot_layer = -1;

  // The extra pixel in left and right borders are necessary so
  // m_commandsBox and m_movePixelsBox do not overlap the upper-left
  // and upper-right pixels drawn in JM_DRAW message (see putpixels)
  jwidget_set_border(this, 1*jguiscale(), 0, 1*jguiscale(), 0);

  // Construct the commands box
  {
    Widget* box1 = jbox_new(JI_HORIZONTAL);
    Widget* box2 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
    Widget* box3 = jbox_new(JI_HORIZONTAL);
    m_slider = jslider_new(0, 255, 255);

    setup_mini_look(m_slider);

    ICON_NEW(m_b_first, GFX_ANI_FIRST, ACTION_FIRST);
    ICON_NEW(m_b_prev, GFX_ANI_PREV, ACTION_PREV);
    ICON_NEW(m_b_play, GFX_ANI_PLAY, ACTION_PLAY);
    ICON_NEW(m_b_next, GFX_ANI_NEXT, ACTION_NEXT);
    ICON_NEW(m_b_last, GFX_ANI_LAST, ACTION_LAST);

    HOOK(m_slider, JI_SIGNAL_SLIDER_CHANGE, slider_change_hook, 0);
    jwidget_set_min_size(m_slider, JI_SCREEN_W/5, 0);

    jwidget_set_border(box1, 2*jguiscale(), 1*jguiscale(), 2*jguiscale(), 2*jguiscale());
    jwidget_noborders(box2);
    jwidget_noborders(box3);
    jwidget_expansive(box3, true);

    jwidget_add_child(box2, m_b_first);
    jwidget_add_child(box2, m_b_prev);
    jwidget_add_child(box2, m_b_play);
    jwidget_add_child(box2, m_b_next);
    jwidget_add_child(box2, m_b_last);

    jwidget_add_child(box1, box3);
    jwidget_add_child(box1, box2);
    jwidget_add_child(box1, m_slider);

    m_commandsBox = box1;
  }

  // Construct move-pixels box
  {
    Widget* filler = jbox_new(JI_HORIZONTAL);
    jwidget_expansive(filler, true);

    m_movePixelsBox = jbox_new(JI_HORIZONTAL);
    m_transparentLabel = new Label("Transparent Color:");
    m_transparentColor = colorbutton_new(color_mask(), IMAGE_RGB);

    jwidget_add_child(m_movePixelsBox, filler);
    jwidget_add_child(m_movePixelsBox, m_transparentLabel);
    jwidget_add_child(m_movePixelsBox, m_transparentColor);

    HOOK(m_transparentColor, SIGNAL_COLORBUTTON_CHANGE, transparent_color_change_hook, 1);
  }

  App::instance()->CurrentToolChange.connect(&StatusBar::onCurrentToolChange, this);
}

StatusBar::~StatusBar()
{
  JLink link;

  JI_LIST_FOR_EACH(m_progress, link) {
    jfree(link->data);
  }
  jlist_free(m_progress);

  if (m_tipwindow != NULL)
    jwidget_free(m_tipwindow);
}

void StatusBar::onCurrentToolChange()
{
  if (isVisible()) {
    Tool* currentTool = UIContext::instance()->getSettings()->getCurrentTool();
    if (currentTool) {
      this->showTool(500, currentTool);
      this->setTextf("%s Selected", currentTool->getText().c_str());
    }
  }
}

void StatusBar::clearText()
{
  setStatusText(1, "");
}

bool StatusBar::setStatusText(int msecs, const char *format, ...)
{
  if ((ji_clock > m_timeout) || (msecs > 0)) {
    char buf[256];		// TODO warning buffer overflow
    va_list ap;

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    m_timeout = ji_clock + msecs;
    m_state = SHOW_TEXT;

    this->setText(buf);
    this->dirty();

    return true;
  }
  else
    return false;
}

void StatusBar::showTip(int msecs, const char *format, ...)
{
  char buf[256];		// TODO warning buffer overflow
  va_list ap;
  int x, y;

  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  if (m_tipwindow == NULL) {
    m_tipwindow = new TipWindow(buf);
    m_tipwindow->user_data[0] = (void *)jmanager_add_timer(m_tipwindow, msecs);
    m_tipwindow->user_data[1] = this;
    jwidget_add_hook(m_tipwindow, -1, tipwindow_msg_proc, NULL);
  }
  else {
    m_tipwindow->setText(buf);

    jmanager_set_timer_interval((size_t)m_tipwindow->user_data[0], msecs);
  }

  if (m_tipwindow->isVisible())
    m_tipwindow->closeWindow(NULL);

  m_tipwindow->open_window();
  m_tipwindow->remap_window();

  x = this->rc->x2 - jrect_w(m_tipwindow->rc);
  y = this->rc->y1 - jrect_h(m_tipwindow->rc);
  m_tipwindow->position_window(x, y);

  jmanager_start_timer((size_t)m_tipwindow->user_data[0]);

  // Set the text in status-bar (with inmediate timeout)
  m_timeout = ji_clock;
  this->setText(buf);
  this->dirty();
}

void StatusBar::showColor(int msecs, const char* text, color_t color, int alpha)
{
  if (setStatusText(msecs, text)) {
    m_state = SHOW_COLOR;
    m_color = color;
    m_alpha = alpha;
  }
}

void StatusBar::showTool(int msecs, Tool* tool)
{
  ASSERT(tool != NULL);

  // Tool name
  std::string text = tool->getText();

  // Tool shortcut
  JAccel accel = get_accel_to_change_tool(tool);
  if (accel) {
    char buf[512];		// TODO possible buffer overflow
    jaccel_to_string(accel, buf);
    text += ", Shortcut: ";
    text += buf;
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
  if (!jwidget_has_child(this, m_movePixelsBox)) {
    jwidget_add_child(this, m_movePixelsBox);
    jwidget_dirty(this);
  }
}

void StatusBar::hideMovePixelsOptions()
{
  if (jwidget_has_child(this, m_movePixelsBox)) {
    jwidget_remove_child(this, m_movePixelsBox);
    jwidget_dirty(this);
  }
}

color_t StatusBar::getTransparentColor()
{
  return colorbutton_get_color(m_transparentColor);
}

//////////////////////////////////////////////////////////////////////
// Progress bars stuff

Progress* StatusBar::addProgress()
{
  Progress* progress = new Progress(this);
  jlist_append(m_progress, progress);
  jwidget_dirty(this);
  return progress;
}

void StatusBar::removeProgress(Progress* progress)
{
  ASSERT(progress->m_statusbar == this);

  jlist_remove(m_progress, progress);
  jwidget_dirty(this);
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

void Progress::setPos(float pos)
{
  if (m_pos != pos) {
    m_pos = pos;
    m_statusbar->dirty();
  }
}

float Progress::getPos() const
{
  return m_pos;
}

//////////////////////////////////////////////////////////////////////
// StatusBar message handler

bool StatusBar::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      msg->reqsize.w = msg->reqsize.h =
	4*jguiscale()
	+ jwidget_get_text_height(this)
	+ 4*jguiscale();
      return true;

    case JM_SETPOS:
      jrect_copy(this->rc, &msg->setpos.rect);
      {
	JRect rc = jrect_new_copy(this->rc);
	rc->x2 -= jrect_w(rc)/4 + 4*jguiscale();
	jwidget_set_rect(m_commandsBox, rc);
	jrect_free(rc);
      }
      {
	JRect rc = jrect_new_copy(this->rc);
	Vaca::Size reqSize = m_movePixelsBox->getPreferredSize();
	rc->x1 = rc->x2 - reqSize.w;
	rc->x2 -= this->border_width.r;
	jwidget_set_rect(m_movePixelsBox, rc);
	jrect_free(rc);
      }
      return true;

    case JM_CLOSE:
      if (!jwidget_has_child(this, m_commandsBox)) {
	// Append the "m_commandsBox" so it is destroyed in StatusBar dtor.
	jwidget_add_child(this, m_commandsBox);
      }
      if (!jwidget_has_child(this, m_movePixelsBox)) {
	// Append the "m_movePixelsBox" so it is destroyed in StatusBar dtor.
	jwidget_add_child(this, m_movePixelsBox);
      }
      break;

    case JM_DRAW: {
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      int text_color = ji_color_foreground();
      int face_color = ji_color_face();
      JRect rc = jwidget_get_rect(this);
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      jrect_displace(rc,
		     -msg->draw.rect.x1,
		     -msg->draw.rect.y1);

      clear_to_color(doublebuffer, face_color);

      putpixel(doublebuffer, rc->x1, rc->y1, theme->get_tab_selected_face_color());
      putpixel(doublebuffer, rc->x2-1, rc->y1, theme->get_tab_selected_face_color());

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
			  app_get_current_image_type(), m_color,
			  false, false);

	x += (32+4)*jguiscale();

	// Draw color description
	char buf[512];		// TODO warning buffer overflow
	color_to_formalstring(app_get_current_image_type(),
			      m_color, buf, sizeof(buf), true);
	if (m_alpha < 255)
	  usprintf(buf+ustrlen(buf), ", Alpha %d", m_alpha);

	textout_ex(doublebuffer, this->getFont(), buf,
		   x, (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
		   text_color, -1);

	x += ji_font_text_len(this->getFont(), buf) + 4*jguiscale();
      }

      // Show tool
      if (m_state == SHOW_TOOL) {
	// Draw eyedropper icon
	BITMAP* icon = theme->get_toolicon(m_tool->getId().c_str());
	if (icon) {
	  set_alpha_blender();
	  draw_trans_sprite(doublebuffer, icon,
			    x, (rc->y1+rc->y2)/2-icon->h/2);
	}

	x += icon->w + 4*jguiscale();
      }

      // Status bar text
      if (this->getTextSize() > 0) {
	textout_ex(doublebuffer, this->getFont(), this->getText(),
		   x,
		   (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
		   text_color, -1);

	x += ji_font_text_len(this->getFont(), this->getText()) + 4*jguiscale();
      }

      // Draw progress bar
      if (!jlist_empty(m_progress)) {
	int width = 64;
	int y1, y2;
	int x = rc->x2 - (width+4);
	JLink link;

	y1 = rc->y1;
	y2 = rc->y2-1;

	JI_LIST_FOR_EACH(m_progress, link) {
	  Progress* progress = reinterpret_cast<Progress*>(link->data);

	  draw_progress_bar(doublebuffer,
			    x, y1, x+width-1, y2,
			    progress->getPos());

	  x -= width+4;
	}
      }
      else {
	// Available width for layers buttons
	int width = jrect_w(rc)/4;

	// Draw layers
	try {
	  --rc->y2;

	  const CurrentSpriteReader sprite(UIContext::instance());
	  if (sprite) {
	    const LayerFolder* folder = sprite->getFolder();
	    LayerConstIterator it = folder->get_layer_begin();
	    LayerConstIterator end = folder->get_layer_end();
	    int count = folder->get_layers_count();
	    char buf[256];

	    for (int c=0; it != end; ++it, ++c) {
	      int x1 = rc->x2-width + c*width/count;
	      int x2 = rc->x2-width + (c+1)*width/count;
	      bool hot = (*it == sprite->getCurrentLayer())
		|| (c == m_hot_layer);

	      theme->draw_bounds_nw(doublebuffer,
				    x1, rc->y1, x2, rc->y2,
				    hot ? PART_TOOLBUTTON_HOT_NW:
					  PART_TOOLBUTTON_NORMAL_NW,
				    hot ? theme->get_button_hot_face_color():
					  theme->get_button_normal_face_color());

	      if (count == 1)
		uszprintf(buf, sizeof(buf), "%s", (*it)->get_name().c_str());
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
				hot ? theme->get_button_hot_text_color():
				      theme->get_button_normal_text_color(), -1);
	    }
	  }
	  else {
	    int x1 = rc->x2-width;
	    int x2 = rc->x2;
	    bool hot = (0 == m_hot_layer);

	    theme->draw_bounds_nw(doublebuffer,
				  x1, rc->y1, x2, rc->y2,
				  hot ? PART_TOOLBUTTON_HOT_NW:
					PART_TOOLBUTTON_NORMAL_NW,
				  hot ? theme->get_button_hot_face_color():
					theme->get_button_normal_face_color());

	    textout_centre_ex(doublebuffer, this->getFont(), "Donate",
			      (x1+x2)/2,
			      (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
			      hot ? theme->get_button_hot_text_color():
				    theme->get_button_normal_text_color(), -1);
	  }
	}
	catch (LockedSpriteException&) {
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
      bool state = (UIContext::instance()->get_current_sprite() != NULL);

      if (!jwidget_has_child(this, m_movePixelsBox)) {
	if (!jwidget_has_child(this, m_commandsBox) && state) {
	  m_b_first->setEnabled(state);
	  m_b_prev->setEnabled(state);
	  m_b_play->setEnabled(state);
	  m_b_next->setEnabled(state);
	  m_b_last->setEnabled(state);

	  updateFromLayer();

	  jwidget_add_child(this, m_commandsBox);
	  jwidget_dirty(this);
	}
	else {
	  // Status text for donations
	  setStatusText(0, "Click the \"Donate\" button to support ASE development");
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

	int hot_layer = -1;

	const CurrentSpriteReader sprite(UIContext::instance());
	// Check which sprite's layer has the mouse over
	if (sprite) {
	  const LayerFolder* folder = sprite->getFolder();
	  LayerConstIterator it = folder->get_layer_begin();
	  LayerConstIterator end = folder->get_layer_end();
	  int count = folder->get_layers_count();

	  for (int c=0; it != end; ++it, ++c) {
	    int x1 = rc->x2-width + c*width/count;
	    int x2 = rc->x2-width + (c+1)*width/count;

	    if (Rect(Point(x1, rc->y1),
		     Point(x2, rc->y2)).contains(Point(msg->mouse.x,
						       msg->mouse.y))) {
	      hot_layer = c;
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
	    hot_layer = 0;
	  }
	}

	if (m_hot_layer != hot_layer) {
	  m_hot_layer = hot_layer;
	  dirty();
	}
      }
      catch (LockedSpriteException&) {
	// Do nothing...
      }
      break;
    }
      
    case JM_BUTTONPRESSED:
      // When the user press the mouse-button over a hot-layer-button...
      if (m_hot_layer >= 0) {
	try {
	  CurrentSpriteWriter sprite(UIContext::instance());
	  if (sprite) {
	    Layer* layer = sprite->indexToLayer(m_hot_layer);
	    if (layer) {
	      // Set the current layer
	      if (layer != sprite->getCurrentLayer())
		sprite->setCurrentLayer(layer);

	      // Flash the current layer
	      ASSERT(current_editor != NULL); // Cannot be null when we have a current sprite
	      current_editor->flashCurrentLayer();

	      // Redraw the status-bar
	      dirty();
	    }
	  }
	  else {
	    // Call "Donate" command
	    Command* donate = CommandsModule::instance()
	      ->get_command_by_name(CommandId::donate);

	    Params params;
	    UIContext::instance()->execute_command(donate, &params);
	  }
	}
	catch (LockedSpriteException&) {
	  // Do nothing...
	}
      }
      break;

    case JM_MOUSELEAVE:
      if (jwidget_has_child(this, m_commandsBox)) {
	// If we want restore the state-bar and the slider doesn't have
	// the capture...
	if (jmanager_get_capture() != m_slider) {
	  // ...exit from command mode
	  jmanager_free_focus();		// TODO Review this code

	  jwidget_remove_child(this, m_commandsBox);
	  jwidget_dirty(this);
	}

	if (m_hot_layer >= 0) {
	  m_hot_layer = -1;
	  dirty();
	}
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

static bool tipwindow_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DESTROY:
      jmanager_remove_timer((size_t)widget->user_data[0]);
      break;

    case JM_TIMER:
      static_cast<Frame*>(widget)->closeWindow(NULL);
      break;
  }

  return false;
}

static bool transparent_color_change_hook(JWidget widget, void *data)
{
  if (current_editor)
    current_editor->setMaskColorForPixelsMovement(app_get_statusbar()->getTransparentColor());
  return true;
}

static bool slider_change_hook(JWidget widget, void *data)
{
  try {
    CurrentSpriteWriter sprite(UIContext::instance());
    if (sprite) {
      if ((sprite->getCurrentLayer()) &&
	  (sprite->getCurrentLayer()->is_image())) {
	Cel* cel = ((LayerImage*)sprite->getCurrentLayer())->get_cel(sprite->getCurrentFrame());
	if (cel) {
	  // Update the opacity
	  cel->opacity = jslider_get_value(widget);

	  // Update the editors
	  update_screen_for_sprite(sprite);
	}
      }
    }
  }
  catch (LockedSpriteException&) {
    // do nothing
  }
  return false;
}

static void button_command(JWidget widget, void *data)
{
  Command* cmd = NULL;

  switch ((size_t)data) {
    //case ACTION_LAYER: cmd = CommandsModule::instance()->get_command_by_name(CommandId::layer_properties); break;
    case ACTION_FIRST: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_first_frame); break;
    case ACTION_PREV: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_previous_frame); break;
    case ACTION_PLAY: cmd = CommandsModule::instance()->get_command_by_name(CommandId::play_animation); break;
    case ACTION_NEXT: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_next_frame); break;
    case ACTION_LAST: cmd = CommandsModule::instance()->get_command_by_name(CommandId::goto_last_frame); break;
  }

  if (cmd)
    UIContext::instance()->execute_command(cmd);
}

void StatusBar::updateFromLayer()
{
  try {
    const CurrentSpriteReader sprite(UIContext::instance());
    Cel *cel;

    // Opacity layer
    if (sprite &&
	sprite->getCurrentLayer() &&
	sprite->getCurrentLayer()->is_image() &&
	!sprite->getCurrentLayer()->is_background() &&
	(cel = ((LayerImage*)sprite->getCurrentLayer())->get_cel(sprite->getCurrentFrame()))) {
      jslider_set_value(m_slider, MID(0, cel->opacity, 255));
      m_slider->setEnabled(true);
    }
    else {
      jslider_set_value(m_slider, 255);
      m_slider->setEnabled(false);
    }
  }
  catch (LockedSpriteException&) {
    // Disable all
    m_slider->setEnabled(false);
  }
}
