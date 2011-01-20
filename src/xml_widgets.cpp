/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include <allegro/unicode.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ase_exception.h"
#include "base/bind.h"
#include "gui/jinete.h"
#include "modules/gui.h"

#include "tinyxml.h"

/* determine is the characer is a blank space */
#define IS_BLANK(c) (((c) ==  ' ') ||  \
		     ((c) == '\t') ||	\
		     ((c) == '\n') ||	\
		     ((c) == '\r'))

#define TRANSLATE_ATTR(a) a

static bool bool_attr_is_true(TiXmlElement* elem, const char* attribute_name);
static Widget* convert_xmlelement_to_widget(TiXmlElement* elem, Widget* root);
static int convert_align_value_to_flags(const char *value);

Widget* load_widget_from_xmlfile(const char* xmlFilename, const char* widgetName)
{
  Widget* widget = NULL;

  TiXmlDocument doc;
  if (!doc.LoadFile(xmlFilename))
    throw AseException(&doc);

  // search the requested widget
  TiXmlHandle handle(&doc);
  TiXmlElement* xmlElement = handle
    .FirstChild("jinete")
    .FirstChildElement().ToElement();

  while (xmlElement) {
    const char* nodename = xmlElement->Attribute("name");

    if (nodename && ustrcmp(nodename, widgetName) == 0) {
      widget = convert_xmlelement_to_widget(xmlElement, NULL);
      break;
    }

    xmlElement = xmlElement->NextSiblingElement();
  }

  return widget;
}

static bool bool_attr_is_true(TiXmlElement* elem, const char* attribute_name)
{
  const char* value = elem->Attribute(attribute_name);

  return (value != NULL) && (strcmp(value, "true") == 0);
}

static Widget* convert_xmlelement_to_widget(TiXmlElement* elem, Widget* root)
{
  const char *elem_name = elem->Value();
  JWidget widget = NULL;
  JWidget child;

  /* TODO error handling: add a message if the widget is bad specified */

  /* box */
  if (ustrcmp(elem_name, "box") == 0) {
    bool horizontal  = bool_attr_is_true(elem, "horizontal");
    bool vertical    = bool_attr_is_true(elem, "vertical");
    bool homogeneous = bool_attr_is_true(elem, "homogeneous");

    widget = jbox_new((horizontal ? JI_HORIZONTAL:
		       vertical ? JI_VERTICAL: 0) |
		      (homogeneous ? JI_HOMOGENEOUS: 0));
  }
  /* button */
  else if (ustrcmp(elem_name, "button") == 0) {
    const char *text = elem->Attribute("text");

    widget = new Button(text ? TRANSLATE_ATTR(text): NULL);
    if (widget) {
      bool left   = bool_attr_is_true(elem, "left");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");
      bool closewindow = bool_attr_is_true(elem, "closewindow");
      const char *_bevel = elem->Attribute("bevel");

      widget->setAlign((left ? JI_LEFT: (right ? JI_RIGHT: JI_CENTER)) |
		       (top ? JI_TOP: (bottom ? JI_BOTTOM: JI_MIDDLE)));

      if (_bevel != NULL) {
	char *bevel = jstrdup(_bevel);
	int c, b[4];
	char *tok;

	for (c=0; c<4; ++c)
	  b[c] = 0;

	for (tok=ustrtok(bevel, " "), c=0;
	     tok;
	     tok=ustrtok(NULL, " "), ++c) {
	  if (c < 4)
	    b[c] = ustrtol(tok, NULL, 10);
	}
	jfree(bevel);

	setup_bevels(widget, b[0], b[1], b[2], b[3]);
      }

      if (closewindow && root) {
	if (Frame* frame = dynamic_cast<Frame*>(root)) {
	  static_cast<Button*>(widget)->
	    Click.connect(Bind<void>(&Frame::closeWindow, frame, widget));
	}
      }
    }
  }
  /* check */
  else if (ustrcmp(elem_name, "check") == 0) {
    const char *text = elem->Attribute("text");
    const char *looklike = elem->Attribute("looklike");

    text = (text ? TRANSLATE_ATTR(text): NULL);

    if (looklike != NULL && strcmp(looklike, "button") == 0) {
      widget = new CheckBox(text, JI_BUTTON);
    }
    else {
      widget = new CheckBox(text);
    }

    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign((center ? JI_CENTER:
			(right ? JI_RIGHT: JI_LEFT)) |
		       (top    ? JI_TOP:
			(bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* combobox */
  else if (ustrcmp(elem_name, "combobox") == 0) {
    widget = new ComboBox();
  }
  /* entry */
  else if (ustrcmp(elem_name, "entry") == 0) {
    const char *maxsize = elem->Attribute("maxsize");
    const char *text = elem->Attribute("text");

    if (maxsize != NULL) {
      bool readonly = bool_attr_is_true(elem, "readonly");

      widget = new Entry(ustrtol(maxsize, NULL, 10),
			 text ? TRANSLATE_ATTR(text): NULL);

      if (readonly)
	((Entry*)widget)->setReadOnly(true);
    }
  }
  /* grid */
  else if (ustrcmp(elem_name, "grid") == 0) {
    const char *columns = elem->Attribute("columns");
    bool same_width_columns = bool_attr_is_true(elem, "same_width_columns");

    if (columns != NULL) {
      widget = jgrid_new(ustrtol(columns, NULL, 10),
			 same_width_columns);
    }
  }
  /* label */
  else if (ustrcmp(elem_name, "label") == 0) {
    const char *text = elem->Attribute("text");

    widget = new Label(text ? TRANSLATE_ATTR(text): NULL);
    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign((center ? JI_CENTER:
			(right ? JI_RIGHT: JI_LEFT)) |
		       (top    ? JI_TOP:
			(bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* listbox */
  else if (ustrcmp(elem_name, "listbox") == 0) {
    widget = jlistbox_new();
  }
  /* listitem */
  else if (ustrcmp(elem_name, "listitem") == 0) {
    const char *text = elem->Attribute("text");

    widget = jlistitem_new(text ? TRANSLATE_ATTR(text): NULL);
  }
  /* panel */
  else if (ustrcmp(elem_name, "panel") == 0) {
    bool horizontal = bool_attr_is_true(elem, "horizontal");
    bool vertical = bool_attr_is_true(elem, "vertical");

    widget = jpanel_new(horizontal ? JI_HORIZONTAL:
			vertical ? JI_VERTICAL: 0);
  }
  /* radio */
  else if (ustrcmp(elem_name, "radio") == 0) {
    const char* text = elem->Attribute("text");
    const char* group = elem->Attribute("group");
    const char *looklike = elem->Attribute("looklike");

    text = (text ? TRANSLATE_ATTR(text): NULL);
    int radio_group = (group ? ustrtol(group, NULL, 10): 1);
    
    if (looklike != NULL && strcmp(looklike, "button") == 0) {
      widget = new RadioButton(text, radio_group, JI_BUTTON);
    }
    else {
      widget = new RadioButton(text, radio_group);
    }

    if (widget) {
      bool center = bool_attr_is_true(elem, "center");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");

      widget->setAlign((center ? JI_CENTER:
			(right ? JI_RIGHT: JI_LEFT)) |
		       (top    ? JI_TOP:
			(bottom ? JI_BOTTOM: JI_MIDDLE)));
    }
  }
  /* separator */
  else if (ustrcmp(elem_name, "separator") == 0) {
    const char *text = elem->Attribute("text");
    bool center      = bool_attr_is_true(elem, "center");
    bool right       = bool_attr_is_true(elem, "right");
    bool middle      = bool_attr_is_true(elem, "middle");
    bool bottom      = bool_attr_is_true(elem, "bottom");
    bool horizontal  = bool_attr_is_true(elem, "horizontal");
    bool vertical    = bool_attr_is_true(elem, "vertical");

    widget = ji_separator_new(text ? TRANSLATE_ATTR(text): NULL,
			      (horizontal ? JI_HORIZONTAL: 0) |
			      (vertical ? JI_VERTICAL: 0) |
			      (center ? JI_CENTER:
			       (right ? JI_RIGHT: JI_LEFT)) |
			      (middle ? JI_MIDDLE:
			       (bottom ? JI_BOTTOM: JI_TOP)));
  }
  /* slider */
  else if (ustrcmp(elem_name, "slider") == 0) {
    const char *min = elem->Attribute("min");
    const char *max = elem->Attribute("max");
    int min_value = min != NULL ? ustrtol(min, NULL, 10): 0;
    int max_value = max != NULL ? ustrtol(max, NULL, 10): 0;

    widget = new Slider(min_value, max_value, min_value);
  }
  /* textbox */
  else if (ustrcmp(elem_name, "textbox") == 0) {
    //bool wordwrap = bool_attr_is_true(elem, "wordwrap");

    /* TODO add translatable support */
    /* TODO here we need jxmlelem_get_text(elem) */
    /* widget = jtextbox_new(tag->text, wordwrap ? JI_WORDWRAP: 0); */
    ASSERT(false);
  }
  /* view */
  else if (ustrcmp(elem_name, "view") == 0) {
    widget = jview_new();
  }
  /* window */
  else if (ustrcmp(elem_name, "window") == 0) {
    const char *text = elem->Attribute("text");

    if (text) {
      bool desktop = bool_attr_is_true(elem, "desktop");

      if (desktop)
	widget = new Frame(true, NULL);
      else
	widget = new Frame(false, TRANSLATE_ATTR(text));
    }
  }

  // Was the widget created?
  if (widget) {
    const char *name      = elem->Attribute("name");
    const char *tooltip   = elem->Attribute("tooltip");
    bool selected         = bool_attr_is_true(elem, "selected");
    bool disabled         = bool_attr_is_true(elem, "disabled");
    bool expansive        = bool_attr_is_true(elem, "expansive");
    bool magnetic         = bool_attr_is_true(elem, "magnetic");
    bool noborders        = bool_attr_is_true(elem, "noborders");
    const char *width     = elem->Attribute("width");
    const char *height    = elem->Attribute("height");
    const char *minwidth  = elem->Attribute("minwidth");
    const char *minheight = elem->Attribute("minheight");
    const char *maxwidth  = elem->Attribute("maxwidth");
    const char *maxheight = elem->Attribute("maxheight");
    const char *childspacing = elem->Attribute("childspacing");

    if (name != NULL)
      widget->setName(name);

    if (tooltip != NULL)
      jwidget_add_tooltip_text(widget, tooltip);

    if (selected)
      widget->setSelected(selected);

    if (disabled)
      widget->setEnabled(false);

    if (expansive)
      jwidget_expansive(widget, true);

    if (magnetic)
      jwidget_magnetic(widget, true);

    if (noborders)
      jwidget_noborders(widget);

    if (childspacing)
      widget->child_spacing = ustrtol(childspacing, NULL, 10);

    if (width || minwidth ||
	height || minheight) {
      int w = (width || minwidth) ? ustrtol(width ? width: minwidth, NULL, 10): 0;
      int h = (height || minheight) ? ustrtol(height ? height: minheight, NULL, 10): 0;
      jwidget_set_min_size(widget, w*jguiscale(), h*jguiscale());
    }

    if (width || maxwidth ||
	height || maxheight) {
      int w = (width || maxwidth) ? strtol(width ? width: maxwidth, NULL, 10): INT_MAX;
      int h = (height || maxheight) ? strtol(height ? height: maxheight, NULL, 10): INT_MAX;
      jwidget_set_max_size(widget, w*jguiscale(), h*jguiscale());
    }

    if (!root)
      root = widget;

    // Children
    TiXmlElement* child_elem = elem->FirstChildElement();
    while (child_elem) {
      child = convert_xmlelement_to_widget(child_elem, root);
      if (child) {
	// Attach the child in the view
	if (widget->type == JI_VIEW) {
	  jview_attach(widget, child);
	  break;
	}
	// Add the child in the grid
	else if (widget->type == JI_GRID) {
	  const char* cell_hspan = child_elem->Attribute("cell_hspan");
	  const char* cell_vspan = child_elem->Attribute("cell_vspan");
	  const char* cell_align = child_elem->Attribute("cell_align");
	  int hspan = cell_hspan ? ustrtol(cell_hspan, NULL, 10): 1;
	  int vspan = cell_vspan ? ustrtol(cell_vspan, NULL, 10): 1;
	  int align = cell_align ? convert_align_value_to_flags(cell_align): 0;

	  jgrid_add_child(widget, child, hspan, vspan, align);
	}
	// Just add the child in any other kind of widget
	else
	  jwidget_add_child(widget, child);
      }
      child_elem = child_elem->NextSiblingElement();
    }

    if (widget->type == JI_VIEW) {
      bool maxsize = bool_attr_is_true(elem, "maxsize");
      if (maxsize)
	jview_maxsize(widget);
    }
  }

  return widget;
}

static int convert_align_value_to_flags(const char *value)
{
  char *tok, *ptr = jstrdup(value);
  int flags = 0;

  for (tok=ustrtok(ptr, " ");
       tok != NULL;
       tok=ustrtok(NULL, " ")) {
    if (ustrcmp(tok, "horizontal") == 0) {
      flags |= JI_HORIZONTAL;
    }
    else if (ustrcmp(tok, "vertical") == 0) {
      flags |= JI_VERTICAL;
    }
    else if (ustrcmp(tok, "left") == 0) {
      flags |= JI_LEFT;
    }
    else if (ustrcmp(tok, "center") == 0) {
      flags |= JI_CENTER;
    }
    else if (ustrcmp(tok, "right") == 0) {
      flags |= JI_RIGHT;
    }
    else if (ustrcmp(tok, "top") == 0) {
      flags |= JI_TOP;
    }
    else if (ustrcmp(tok, "middle") == 0) {
      flags |= JI_MIDDLE;
    }
    else if (ustrcmp(tok, "bottom") == 0) {
      flags |= JI_BOTTOM;
    }
    else if (ustrcmp(tok, "homogeneous") == 0) {
      flags |= JI_HOMOGENEOUS;
    }
  }

  jfree(ptr);
  return flags;
}
