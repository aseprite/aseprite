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

#include <map>
#include <string>
#include <allegro.h>

#include "jinete/jinete.h"
#include "Vaca/Bind.h"
#include "Vaca/Signal.h"
#include "Vaca/Size.h"

#include "app.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "tools/toolbox.h"
#include "ui_context.h"
#include "widgets/groupbut.h"
#include "widgets/statebar.h"
#include "widgets/toolbar.h"

using Vaca::Size;

// Class to show selected tools for each tool (vertically)
class ToolBar : public Widget
{
  // What tool is selected for each tool-group
  std::map<const ToolGroup*, Tool*> m_selected_in_group;

  // What tool has the mouse above
  Tool* m_hot_tool;

  // Does the configuration button have the mouse above?
  bool m_hot_conf;

  // True if the popup-window must be opened when a tool-button is hot
  bool m_open_on_hot;

  // Window displayed to show a tool-group
  PopupWindow* m_popup_window;

  // Tool-tip window
  TipWindow* m_tipWindow;

  int m_tipTimerId;
  bool m_tipOpened;

public:
  ToolBar();
  ~ToolBar();

  bool isToolVisible(Tool* tool);
  void selectTool(Tool* tool);

  void openTipWindow(ToolGroup* tool_group, Tool* tool);
  void closeTipWindow();

protected:
  bool onProcessMessage(JMessage msg);

private:
  int getToolGroupIndex(ToolGroup* group);
  void openPopupWindow(int group_index, ToolGroup* group);
  Rect getToolGroupBounds(int group_index);
  void openTipWindow(int group_index, Tool* tool);
  void onClosePopup();
};

// Class to show a group of tools (horizontally)
// This widget is inside the ToolBar::m_popup_window
class ToolStrip : public Widget
{
  ToolGroup* m_group;
  Tool* m_hot_tool;
  ToolBar* m_toolbar;
  BITMAP* m_overlapped;

public:
  ToolStrip(ToolGroup* group, ToolBar* toolbar);
  ~ToolStrip();

  void saveOverlappedArea(const Rect& bounds);

  Vaca::Signal1<void, Tool*> ToolSelected;

protected:
  bool onProcessMessage(JMessage msg);

private:
  Rect getToolBounds(int index);
};

static Size getToolIconSize(Widget* widget)
{
  SkinneableTheme* theme = static_cast<SkinneableTheme*>(widget->theme);
  BITMAP* icon = theme->get_toolicon("configuration");
  if (icon)
    return Size(icon->w, icon->h);
  else
    return Size(16, 16) * jguiscale();
}

//////////////////////////////////////////////////////////////////////
// ToolBar

JWidget toolbar_new()
{
  return new ToolBar();
}

bool toolbar_is_tool_visible(JWidget toolbar, Tool* tool)
{
  return ((ToolBar*)toolbar)->isToolVisible(tool);
}

void toolbar_select_tool(JWidget toolbar, Tool* tool)
{
  ((ToolBar*)toolbar)->selectTool(tool);
}

ToolBar::ToolBar()
  : Widget(JI_WIDGET)
{
  this->border_width.l = 1*jguiscale();
  this->border_width.t = 0;
  this->border_width.r = 1*jguiscale();
  this->border_width.b = 0;

  m_hot_tool = NULL;
  m_hot_conf = false;
  m_open_on_hot = false;
  m_popup_window = NULL;
  m_tipWindow = NULL;
  m_tipTimerId = jmanager_add_timer(this, 300);
  m_tipOpened = false;
  
  ToolBox* toolbox = App::instance()->get_toolbox();
  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (m_selected_in_group.find(tool->getGroup()) == m_selected_in_group.end())
      m_selected_in_group[tool->getGroup()] = tool;
  }
}

ToolBar::~ToolBar()
{
  jmanager_remove_timer(m_tipTimerId);
  delete m_popup_window;
  delete m_tipWindow;
}

bool ToolBar::isToolVisible(Tool* tool)
{
  return (m_selected_in_group[tool->getGroup()] == tool);
}

bool ToolBar::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      Size iconsize = getToolIconSize(this);
      msg->reqsize.w = iconsize.w + this->border_width.l + this->border_width.r;
      msg->reqsize.h = iconsize.h + this->border_width.t + this->border_width.b;
      return true;
    }

    case JM_DRAW: {
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      ToolBox* toolbox = App::instance()->get_toolbox();
      ToolGroupList::iterator it = toolbox->begin_group();
      int groups = toolbox->getGroupsCount();
      Rect toolrc;

      clear_to_color(doublebuffer, theme->get_tab_selected_face_color());

      for (int c=0; c<groups; ++c, ++it) {
	ToolGroup* tool_group = *it;
	Tool* tool = m_selected_in_group[tool_group];
	int face, nw;

	if (UIContext::instance()->getSettings()->getCurrentTool() == tool ||
	    m_hot_tool == tool) {
	  nw = PART_TOOLBUTTON_HOT_NW;
	  face = theme->get_button_hot_face_color();
	}
	else {
	  nw = c >= 0 && c < groups-1 ? PART_TOOLBUTTON_NORMAL_NW:
					PART_TOOLBUTTON_LAST_NW;
	  face = theme->get_button_normal_face_color();
	}

	toolrc = getToolGroupBounds(c);
	toolrc.offset(-msg->draw.rect.x1, -msg->draw.rect.y1);
	theme->draw_bounds_nw(doublebuffer, toolrc, nw, face);

	// Draw the tool icon
	BITMAP* icon = theme->get_toolicon(tool->getId().c_str());
	if (icon) {
	  set_alpha_blender();
	  draw_trans_sprite(doublebuffer, icon,
			    toolrc.x+toolrc.w/2-icon->w/2,
			    toolrc.y+toolrc.h/2-icon->h/2);
	}
      }

      toolrc = getToolGroupBounds(-1);
      toolrc.offset(-msg->draw.rect.x1, -msg->draw.rect.y1);
      theme->draw_bounds_nw(doublebuffer,
			    toolrc,
			    m_hot_conf ? PART_TOOLBUTTON_HOT_NW:
					 PART_TOOLBUTTON_LAST_NW,
			    m_hot_conf ? theme->get_button_hot_face_color():
					 theme->get_button_normal_face_color());

      // Draw the tool icon
      BITMAP* icon = theme->get_toolicon("configuration");
      if (icon) {
	set_alpha_blender();
	draw_trans_sprite(doublebuffer, icon,
			  toolrc.x+toolrc.w/2-icon->w/2,
			  toolrc.y+toolrc.h/2-icon->h/2);
      }

      blit(doublebuffer, ji_screen, 0, 0,
	   msg->draw.rect.x1,
	   msg->draw.rect.y1,
	   doublebuffer->w,
	   doublebuffer->h);
      destroy_bitmap(doublebuffer);
      return true;
    }
      
    case JM_BUTTONPRESSED: {
      ToolBox* toolbox = App::instance()->get_toolbox();
      int groups = toolbox->getGroupsCount();
      Rect toolrc;

      closeTipWindow();

      ToolGroupList::iterator it = toolbox->begin_group();

      for (int c=0; c<groups; ++c, ++it) {
	ToolGroup* tool_group = *it;
	Tool* tool = m_selected_in_group[tool_group];

	toolrc = getToolGroupBounds(c);
	if (msg->mouse.y >= toolrc.y && msg->mouse.y < toolrc.y+toolrc.h) {
	  UIContext::instance()->getSettings()->setCurrentTool(tool);
	  dirty();

	  openPopupWindow(c, tool_group);
	}
      }

      toolrc = getToolGroupBounds(-1);
      if (msg->mouse.y >= toolrc.y && msg->mouse.y < toolrc.y+toolrc.h) {
	Command* conf_tools_cmd = 
	  CommandsModule::instance()->get_command_by_name(CommandId::configure_tools);

	UIContext::instance()->execute_command(conf_tools_cmd);
      }
      break;
    }

    case JM_MOTION: {
      ToolBox* toolbox = App::instance()->get_toolbox();
      int groups = toolbox->getGroupsCount();
      Tool* hot_tool = NULL;
      bool hot_conf = false;
      Rect toolrc;
      int tip_index = -1;

      ToolGroupList::iterator it = toolbox->begin_group();

      for (int c=0; c<groups; ++c, ++it) {
	ToolGroup* tool_group = *it;
	Tool* tool = m_selected_in_group[tool_group];

	toolrc = getToolGroupBounds(c);
	if (msg->mouse.y >= toolrc.y && msg->mouse.y < toolrc.y+toolrc.h) {
	  hot_tool = tool;

	  if ((m_open_on_hot) && (m_hot_tool != hot_tool))
	    openPopupWindow(c, tool_group);

	  tip_index = c;
	  break;
	}
      }

      toolrc = getToolGroupBounds(-1);
      if (msg->mouse.y >= toolrc.y && msg->mouse.y < toolrc.y+toolrc.h) {
	hot_conf = true;
      }

      // hot button changed
      if (m_hot_tool != hot_tool ||
	  m_hot_conf != hot_conf) {
	m_hot_tool = hot_tool;
	m_hot_conf = hot_conf;
	dirty();

	if (m_hot_tool || m_hot_conf)
	  openTipWindow(tip_index, m_hot_tool);
	else
	  closeTipWindow();

	if (m_hot_tool)
	  app_get_statusbar()->showTool(0, m_hot_tool);
      }
      break;
    }

    case JM_MOUSELEAVE:
      closeTipWindow();

      if (!m_popup_window)
	m_tipOpened = false;

      m_hot_tool = NULL;
      m_hot_conf = false;
      dirty();

      app_get_statusbar()->clearText();
      break;

    case JM_TIMER:
      if (msg->timer.timer_id == m_tipTimerId) {
	if (m_tipWindow)
	  m_tipWindow->open_window();

	jmanager_stop_timer(m_tipTimerId);
	m_tipOpened = true;
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

int ToolBar::getToolGroupIndex(ToolGroup* group)
{
  ToolBox* toolbox = App::instance()->get_toolbox();
  ToolGroupList::iterator it = toolbox->begin_group();
  int groups = toolbox->getGroupsCount();

  for (int c=0; c<groups; ++c, ++it) {
    if (group == *it)
      return c;
  }

  return -1;
}

void ToolBar::openPopupWindow(int group_index, ToolGroup* tool_group)
{
  // Close the current popup window
  if (m_popup_window) {
    m_popup_window->closeWindow(NULL);
    delete m_popup_window;
    m_popup_window = NULL;
  }

  // Close tip window
  closeTipWindow();

  // If this group contains only one tool, do not show the popup
  ToolBox* toolbox = App::instance()->get_toolbox();
  int count = 0;
  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == tool_group)
      ++count;
  }
  if (count <= 1)
    return;

  // In case this tool contains more than just one tool, show the popup window
  m_open_on_hot = true;
  m_popup_window = new PopupWindow(NULL, false);
  m_popup_window->Close.connect(Vaca::Bind<void, ToolBar, ToolBar>(&ToolBar::onClosePopup, this));

  ToolStrip* toolstrip = new ToolStrip(tool_group, this);
  jwidget_add_child(m_popup_window, toolstrip);

  Rect rc = getToolGroupBounds(group_index);
  int w = 0;

  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == tool_group)
      w += jrect_w(this->rc)-this->border_width.l-this->border_width.r-1;
  }

  rc.x -= w;
  rc.w = w;

  // Redraw the overlapped area and save it to use it in the ToolStrip::onProcessMessage(JM_DRAW)
  {
    JRect rcTemp = jrect_new(rc.x, rc.y, rc.x+rc.w, rc.y+rc.h);
    jwidget_invalidate_rect(ji_get_default_manager(),  rcTemp);
    jrect_free(rcTemp);

    // Flush JM_DRAW messages and send them
    jwidget_flush_redraw(ji_get_default_manager());
    jmanager_dispatch_messages(ji_get_default_manager());

    // Save the area
    toolstrip->saveOverlappedArea(rc);
  }

  // Set hotregion of popup window
  {
    jrect rc2 = { rc.x, rc.y, this->rc->x2, rc.y+rc.h };
    JRegion hotregion = jregion_new(&rc2, 1);
    m_popup_window->setHotRegion(hotregion);
  }

  m_popup_window->set_autoremap(false);
  m_popup_window->setBounds(rc);
  toolstrip->setBounds(rc);
  m_popup_window->open_window();

  toolstrip->setBounds(rc);
}

Rect ToolBar::getToolGroupBounds(int group_index)
{
  ToolBox* toolbox = App::instance()->get_toolbox();
  int groups = toolbox->getGroupsCount();
  Size iconsize = getToolIconSize(this);

  if (group_index >= 0)
    return Rect(rc->x1+border_width.l,
		rc->y1+border_width.t+group_index*(iconsize.h-1*jguiscale()),
		jrect_w(rc)-border_width.l-border_width.r,
		group_index < groups-1 ? iconsize.h+1*jguiscale():
					 iconsize.h+2*jguiscale());
  else
    return Rect(rc->x1+border_width.l,
		rc->y1+border_width.t+groups*(iconsize.h-1*jguiscale())+ 8*jguiscale(),
		jrect_w(rc)-border_width.l-border_width.r,
		iconsize.h+2*jguiscale());
}

void ToolBar::openTipWindow(ToolGroup* tool_group, Tool* tool)
{
  openTipWindow(getToolGroupIndex(tool_group), tool);
}

void ToolBar::openTipWindow(int group_index, Tool* tool)
{
  if (m_tipWindow)
    closeTipWindow();

  std::string tooltip;
  if (tool) {
    tooltip = tool->getText();
    if (tool->getTips().size() > 0) {
      tooltip += ":\n";
      tooltip += tool->getTips();
    }
  }
  else {
    tooltip = "Configure Tool";
  }

  m_tipWindow = new TipWindow(tooltip.c_str(), true);
  m_tipWindow->remap_window();

  Rect toolrc = getToolGroupBounds(group_index);
  int w = jrect_w(m_tipWindow->rc);
  int h = jrect_h(m_tipWindow->rc);
  int x = toolrc.x - w;
  int y = toolrc.y + toolrc.h;

  m_tipWindow->position_window(MID(0, x, JI_SCREEN_W-w),
			       MID(0, y, JI_SCREEN_H-h));

  if (m_tipOpened)
    m_tipWindow->open_window();
  else
    jmanager_start_timer(m_tipTimerId);
}

void ToolBar::closeTipWindow()
{
  jmanager_stop_timer(m_tipTimerId);

  if (m_tipWindow) {
    m_tipWindow->closeWindow(NULL);
    delete m_tipWindow;
    m_tipWindow = NULL;
  }
}

void ToolBar::selectTool(Tool* tool)
{
  ASSERT(tool != NULL);

  m_selected_in_group[tool->getGroup()] = tool;

  UIContext::instance()->getSettings()->setCurrentTool(tool);
  dirty();
}

void ToolBar::onClosePopup()
{
  closeTipWindow();

  if (!hasMouse())
    m_tipOpened = false;

  m_open_on_hot = false;
  m_hot_tool = NULL;
  dirty();
}

//////////////////////////////////////////////////////////////////////
// ToolStrip
//////////////////////////////////////////////////////////////////////

ToolStrip::ToolStrip(ToolGroup* group, ToolBar* toolbar)
  : Widget(JI_WIDGET)
{
  m_group = group;
  m_hot_tool = NULL;
  m_toolbar = toolbar;
  m_overlapped = NULL;
}

ToolStrip::~ToolStrip()
{
  if (m_overlapped)
    destroy_bitmap(m_overlapped);
}

void ToolStrip::saveOverlappedArea(const Rect& bounds)
{
  if (m_overlapped)
    destroy_bitmap(m_overlapped);

  m_overlapped = create_bitmap(bounds.w, bounds.h);

  blit(ji_screen, m_overlapped,
       bounds.x, bounds.y, 0, 0,
       bounds.w, bounds.h);
}

bool ToolStrip::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      ToolBox* toolbox = App::instance()->get_toolbox();
      int c = 0;

      for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
	Tool* tool = *it;
	if (tool->getGroup() == m_group) {
	  ++c;
	}
      }

      Size iconsize = getToolIconSize(this);
      msg->reqsize.w = iconsize.w * c;
      msg->reqsize.h = iconsize.h;
      return true;
    }

    case JM_DRAW: {
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      ToolBox* toolbox = App::instance()->get_toolbox();
      Rect toolrc;
      int index = 0;

      // Get the chunk of screen where we will draw
      blit(m_overlapped, doublebuffer,
	   this->rc->x1 - msg->draw.rect.x1,
	   this->rc->y1 - msg->draw.rect.y1, 0, 0,
	   doublebuffer->w,
	   doublebuffer->h);

      for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
	Tool* tool = *it;
	if (tool->getGroup() == m_group) {
	  int face, nw;

	  if (UIContext::instance()->getSettings()->getCurrentTool() == tool ||
	      m_hot_tool == tool) {
	    nw = PART_TOOLBUTTON_HOT_NW;
	    face = theme->get_button_hot_face_color();
	  }
	  else {
	    nw = PART_TOOLBUTTON_LAST_NW;
	    face = theme->get_button_normal_face_color();
	  }

	  toolrc = getToolBounds(index++);
	  toolrc.offset(-msg->draw.rect.x1, -msg->draw.rect.y1);
	  theme->draw_bounds_nw(doublebuffer, toolrc, nw, face);

	  // Draw the tool icon
	  BITMAP* icon = theme->get_toolicon(tool->getId().c_str());
	  if (icon) {
	    set_alpha_blender();
	    draw_trans_sprite(doublebuffer, icon,
			      toolrc.x+toolrc.w/2-icon->w/2,
			      toolrc.y+toolrc.h/2-icon->h/2);
	  }
	}
      }

      blit(doublebuffer, ji_screen, 0, 0,
	   msg->draw.rect.x1,
	   msg->draw.rect.y1,
	   doublebuffer->w,
	   doublebuffer->h);
      destroy_bitmap(doublebuffer);
      return true;
    }

    case JM_MOTION: {
      ToolBox* toolbox = App::instance()->get_toolbox();
      Tool* hot_tool = NULL;
      Rect toolrc;
      int index = 0;

      for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
	Tool* tool = *it;
	if (tool->getGroup() == m_group) {
	  toolrc = getToolBounds(index++);
	  if (toolrc.contains(Point(msg->mouse.x, msg->mouse.y))) {
	    hot_tool = tool;
	    break;
	  }
	}
      }

      // Hot button changed
      if (m_hot_tool != hot_tool) {
	m_hot_tool = hot_tool;
	dirty();

	// Show the tooltip for the hot tool
	if (m_hot_tool)
	  m_toolbar->openTipWindow(m_group, m_hot_tool);
	else
	  m_toolbar->closeTipWindow();

	if (m_hot_tool)
	  app_get_statusbar()->showTool(0, m_hot_tool);
      }
      break;
    }

    case JM_BUTTONPRESSED:
      if (m_hot_tool) {
	m_toolbar->selectTool(m_hot_tool);
	closeWindow();
      }
      break;

  }
  return Widget::onProcessMessage(msg);
}

Rect ToolStrip::getToolBounds(int index)
{
  Size iconsize = getToolIconSize(this);

  return Rect(rc->x1+index*(iconsize.w-1), rc->y1,
	      iconsize.w, jrect_h(rc));
}
