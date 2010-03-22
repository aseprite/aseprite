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
#include "ui_context.h"
#include "commands/commands.h"
#include "commands/command.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "tools/toolbox.h"
#include "widgets/groupbut.h"
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

public:
  ToolBar();
  ~ToolBar();

  bool isToolVisible(Tool* tool);
  void selectTool(Tool* tool);

protected:
  bool msg_proc(JMessage msg);

private:
  void onToolSelected(Tool* tool);
  void onClosePopup();

  void openPopupWindow(int group_index, ToolGroup* group);
  Rect getToolGroupBounds(int group_index);
};

// Class to show a group of tools (horizontally)
// This widget is inside the ToolBar::m_popup_window
class ToolStrip : public Widget
{
  ToolGroup* m_group;
  Tool* m_hot_tool;

public:
  ToolStrip(ToolGroup* group);
  ~ToolStrip();

  Vaca::Signal1<void, Tool*> ToolSelected;

protected:
  bool msg_proc(JMessage msg);

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

  ToolBox* toolbox = App::instance()->get_toolbox();
  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (m_selected_in_group.find(tool->getGroup()) == m_selected_in_group.end())
      m_selected_in_group[tool->getGroup()] = tool;
  }
}

ToolBar::~ToolBar()
{
  delete m_popup_window;
}

bool ToolBar::isToolVisible(Tool* tool)
{
  return (m_selected_in_group[tool->getGroup()] == tool);
}

void ToolBar::selectTool(Tool* tool)
{
  onToolSelected(tool);
}

bool ToolBar::msg_proc(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      Size iconsize = getToolIconSize(this);
      msg->reqsize.w = iconsize.w + this->border_width.l + this->border_width.r;
      msg->reqsize.h = iconsize.h + this->border_width.t + this->border_width.b;
      return true;
    }

    case JM_DRAW: {
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      ToolBox* toolbox = App::instance()->get_toolbox();
      ToolGroupList::iterator it = toolbox->begin_group();
      int groups = toolbox->getGroupsCount();
      Rect toolrc;

      jdraw_rectfill(this->rc, theme->get_tab_selected_face_color());

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
	theme->draw_bounds(toolrc, nw, face);

	// Draw the tool icon
	BITMAP* icon = theme->get_toolicon(tool->getId().c_str());
	if (icon)
	  draw_sprite(ji_screen, icon,
		      toolrc.x+toolrc.w/2-icon->w/2,
		      toolrc.y+toolrc.h/2-icon->h/2);
      }

      toolrc = getToolGroupBounds(-1);
      theme->draw_bounds(toolrc,
			 m_hot_conf ? PART_TOOLBUTTON_HOT_NW:
				      PART_TOOLBUTTON_LAST_NW,
			 m_hot_conf ? theme->get_button_hot_face_color():
				      theme->get_button_normal_face_color());

      // Draw the tool icon
      BITMAP* icon = theme->get_toolicon("configuration");
      if (icon)
	draw_sprite(ji_screen, icon,
		    toolrc.x+toolrc.w/2-icon->w/2,
		    toolrc.y+toolrc.h/2-icon->h/2);

      return true;
    }
      
    case JM_BUTTONPRESSED: {
      ToolBox* toolbox = App::instance()->get_toolbox();
      int groups = toolbox->getGroupsCount();
      int y = rc->y1;
      Rect toolrc;

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

      ToolGroupList::iterator it = toolbox->begin_group();

      for (int c=0; c<groups; ++c, ++it) {
	ToolGroup* tool_group = *it;
	Tool* tool = m_selected_in_group[tool_group];

	toolrc = getToolGroupBounds(c);
	if (msg->mouse.y >= toolrc.y && msg->mouse.y < toolrc.y+toolrc.h) {
	  hot_tool = tool;

	  if (m_open_on_hot)
	    openPopupWindow(c, tool_group);

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
      }
      break;
    }

    case JM_MOUSELEAVE:
      m_hot_tool = NULL;
      m_hot_conf = false;
      dirty();
      break;

  }

  return Widget::msg_proc(msg);
}

void ToolBar::openPopupWindow(int group_index, ToolGroup* tool_group)
{
  if (m_popup_window) {
    m_popup_window->closeWindow(NULL);
    delete m_popup_window;
  }

  m_open_on_hot = true;
  m_popup_window = new PopupWindow(NULL, false);
  m_popup_window->Close.connect(Vaca::Bind<void>(&ToolBar::onClosePopup, this));

  ToolStrip* groupbox = new ToolStrip(tool_group);
  groupbox->ToolSelected.connect(&ToolBar::onToolSelected, this);
  jwidget_add_child(m_popup_window, groupbox);

  Rect rc = getToolGroupBounds(group_index);
  int w = 0;

  ToolBox* toolbox = App::instance()->get_toolbox();
  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == tool_group)
      w += jrect_w(this->rc)-this->border_width.l-this->border_width.r-1;
  }

  rc.x -= w;
  rc.w = w;

  // Set hotregion of popup window
  {
    jrect rc2 = { rc.x, rc.y, this->rc->x2, rc.y+rc.h };
    JRegion hotregion = jregion_new(&rc2, 1);
    m_popup_window->setHotRegion(hotregion);
  }

  m_popup_window->set_autoremap(false);
  m_popup_window->setBounds(rc);
  groupbox->setBounds(rc);
  m_popup_window->open_window();

  groupbox->setBounds(rc);
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

void ToolBar::onToolSelected(Tool* tool)
{
  assert(tool != NULL);

  m_selected_in_group[tool->getGroup()] = tool;

  UIContext::instance()->getSettings()->setCurrentTool(tool);
  dirty();
}

void ToolBar::onClosePopup()
{
  m_open_on_hot = false;
  m_hot_tool = NULL;
  dirty();
}

//////////////////////////////////////////////////////////////////////
// ToolStrip

ToolStrip::ToolStrip(ToolGroup* group)
  : Widget(JI_WIDGET)
{
  m_group = group;
  m_hot_tool = NULL;
}

ToolStrip::~ToolStrip()
{
}

bool ToolStrip::msg_proc(JMessage msg)
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
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      ToolBox* toolbox = App::instance()->get_toolbox();
      Rect toolrc;
      int index = 0;

      jdraw_rectfill(this->rc, theme->get_tab_selected_face_color());

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
	  theme->draw_bounds(toolrc, nw, face);

	  // Draw the tool icon
	  BITMAP* icon = theme->get_toolicon(tool->getId().c_str());
	  if (icon)
	    draw_sprite(ji_screen, icon,
			toolrc.x+toolrc.w/2-icon->w/2,
			toolrc.y+toolrc.h/2-icon->h/2);
	}
      }
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

      // hot button changed
      if (m_hot_tool != hot_tool) {
	m_hot_tool = hot_tool;
	dirty();
      }
      break;
    }

    case JM_BUTTONPRESSED:
      if (m_hot_tool) {
	ToolSelected(m_hot_tool);
	closeWindow();
      }
      break;

  }
  return Widget::msg_proc(msg);
}

Rect ToolStrip::getToolBounds(int index)
{
  ToolBox* toolbox = App::instance()->get_toolbox();
  Size iconsize = getToolIconSize(this);

  return Rect(rc->x1+index*(iconsize.w-1), rc->y1,
	      iconsize.w, jrect_h(rc));
}
