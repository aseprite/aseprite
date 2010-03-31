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
#include <cassert>

#include "Vaca/Bind.h"
#include "jinete/jinete.h"

#include "app.h"
#include "commands/commands.h"
#include "core/core.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/skinneable_theme.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "sprite_wrappers.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "util/misc.h"
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
  m_progress = jlist_new();
  m_tipwindow = NULL;
  m_hot_layer = -1;

  // Construct the commands box
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

  m_commands_box = box1;

  App::instance()->CurrentToolChange.connect(Vaca::Bind<void>(&StatusBar::onCurrentToolChange, this));
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
  if (jwidget_is_visible(this)) {
    Tool* currentTool = UIContext::instance()->getSettings()->getCurrentTool();
    if (currentTool)
      this->setStatusText(500, "%s selected",
			  currentTool->getText().c_str());
  }
}

void StatusBar::setStatusText(int msecs, const char *format, ...)
{
  if ((ji_clock > m_timeout) || (msecs > 0)) {
    char buf[256];		// TODO warning buffer overflow
    va_list ap;

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    m_timeout = ji_clock + msecs;
    this->setText(buf);
    this->dirty();
  }
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

  if (jwidget_is_visible(m_tipwindow))
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

void StatusBar::showColor(int msecs, int imgtype, color_t color)
{
  char buf[128];		// TODO warning buffer overflow
  color_to_formalstring(imgtype, color, buf, sizeof(buf), true);
  setStatusText(msecs, "%s %s", _("Color"), buf);
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
  assert(progress->m_statusbar == this);

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

bool StatusBar::msg_proc(JMessage msg)
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
	jwidget_set_rect(m_commands_box, rc);
	jrect_free(rc);
      }
      return true;

    case JM_CLOSE:
      if (!jwidget_has_child(this, m_commands_box)) {
	/* append the "commands_box" to destroy it in the jwidget_free */
	jwidget_add_child(this, m_commands_box);
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

      rc->x1 += 2*jguiscale();
      rc->y1 += 1*jguiscale();
      rc->x2 -= 2*jguiscale();
      rc->y2 -= 2*jguiscale();

      // Status bar text
      if (this->getText()) {
	textout_ex(doublebuffer, this->getFont(), this->getText(),
		   rc->x1+4*jguiscale(),
		   (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
		   text_color, -1);
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

	      {
		BITMAP* old_ji_screen = ji_screen; // TODO fix this ugly hack
		ji_screen = doublebuffer;
		theme->draw_bounds(x1, rc->y1, x2, rc->y2,
				   hot ? PART_TOOLBUTTON_HOT_NW:
					 PART_TOOLBUTTON_NORMAL_NW,
				   hot ? theme->get_button_hot_face_color():
					 theme->get_button_normal_face_color());
		ji_screen = old_ji_screen;
	      }

	      if (count == 1)
		uszprintf(buf, sizeof(buf), "%s", (*it)->get_name().c_str());
	      else
		usprintf(buf, "%d", c);

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

	    {
	      BITMAP* old_ji_screen = ji_screen; // TODO fix this ugly hack
	      ji_screen = doublebuffer;
	      theme->draw_bounds(x1, rc->y1, x2, rc->y2,
				 PART_TOOLBUTTON_NORMAL_NW,
				 theme->get_button_normal_face_color());
	      ji_screen = old_ji_screen;
	    }

	    textout_centre_ex(doublebuffer, this->getFont(), "No Sprite",
			      (x1+x2)/2,
			      (rc->y1+rc->y2)/2-text_height(this->getFont())/2,
			      theme->get_button_normal_text_color(), -1);
	  }
	}
	catch (locked_sprite_exception&) {
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

    case JM_MOUSEENTER:
      if (!jwidget_has_child(this, m_commands_box)) {
	bool state = (UIContext::instance()->get_current_sprite() != NULL);

	m_b_first->setEnabled(state);
	m_b_prev->setEnabled(state);
	m_b_play->setEnabled(state);
	m_b_next->setEnabled(state);
	m_b_last->setEnabled(state);

	updateFromLayer();

	jwidget_add_child(this, m_commands_box);
	jwidget_dirty(this);
      }
      break;

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

	const CurrentSpriteReader sprite(UIContext::instance());
	if (sprite) {
	  const LayerFolder* folder = sprite->getFolder();
	  LayerConstIterator it = folder->get_layer_begin();
	  LayerConstIterator end = folder->get_layer_end();
	  int count = folder->get_layers_count();
	  int hot_layer = -1;

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

	  if (m_hot_layer != hot_layer) {
	    m_hot_layer = hot_layer;
	    dirty();
	  }
	}
      }
      catch (locked_sprite_exception&) {
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
	      assert(current_editor != NULL); // Cannot be null when we have a current sprite
	      current_editor->flashCurrentLayer();

	      // Redraw the status-bar
	      dirty();
	    }
	  }
	}
	catch (locked_sprite_exception&) {
	  // Do nothing...
	}
      }
      break;

    case JM_MOUSELEAVE:
      if (jwidget_has_child(this, m_commands_box)) {
	/* if we want restore the state-bar and the slider doesn't have
	   the capture... */
	if (jmanager_get_capture() != m_slider) {
	  /* exit from command mode */
	  jmanager_free_focus();

	  jwidget_remove_child(this, m_commands_box);
	  jwidget_dirty(this);
	}

	if (m_hot_layer >= 0) {
	  m_hot_layer = -1;
	  dirty();
	}
      }
      break;

  }

  return Widget::msg_proc(msg);
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

static bool slider_change_hook(JWidget widget, void *data)
{
  try {
    CurrentSpriteWriter sprite(UIContext::instance());
    if (sprite) {
      if ((sprite->getCurrentLayer()) &&
	  (sprite->getCurrentLayer()->is_image())) {
	Cel* cel = ((LayerImage*)sprite->getCurrentLayer())->get_cel(sprite->getCurrentFrame());
	if (cel) {
	  // update the opacity
	  cel->opacity = jslider_get_value(widget);

	  // update the editors
	  update_screen_for_sprite(sprite);
	}
      }
    }
  }
  catch (locked_sprite_exception&) {
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

    /* opacity layer */
    if (sprite &&
	sprite->getCurrentLayer() &&
	sprite->getCurrentLayer()->is_image() &&
	!sprite->getCurrentLayer()->is_background() &&
	(cel = ((LayerImage*)sprite->getCurrentLayer())->get_cel(sprite->getCurrentFrame()))) {
      jslider_set_value(m_slider, MID(0, cel->opacity, 255));
      jwidget_enable(m_slider);
    }
    else {
      jslider_set_value(m_slider, 255);
      jwidget_disable(m_slider);
    }
  }
  catch (locked_sprite_exception&) {
    // disable all
    jwidget_disable(m_slider);
  }
}
