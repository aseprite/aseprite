/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "tinyxml.h"

/* determine is the characer is a blank space */
#define IS_BLANK(c) (((c) ==  ' ') ||  \
		     ((c) == '\t') ||	\
		     ((c) == '\n') ||	\
		     ((c) == '\r'))

/* TODO */
/* #define TRANSLATE_ATTR(a)				\ */
/*   (((a) != NULL && (a)[0] == '_') ?			\ */
/*    ji_translate_string((a)+1): (a)+1) */

#define TRANSLATE_ATTR(a) a

static bool bool_attr_is_true(TiXmlElement* elem, const char* attribute_name);
static JWidget convert_xmlelement_to_widget(TiXmlElement* elem);
static int convert_align_value_to_flags(const char *value);

JWidget ji_load_widget(const char *filename, const char *name)
{
  JWidget widget = NULL;

  TiXmlDocument doc;
  if (!doc.LoadFile(filename))
    throw jexception(&doc);

  // search the requested widget
  TiXmlHandle handle(&doc);
  TiXmlElement* xmlElement = handle
    .FirstChild("jinete")
    .FirstChildElement().ToElement();

  while (xmlElement) {
    const char* nodename = xmlElement->Attribute("name");

    if (nodename && ustrcmp(nodename, name) == 0) {
      widget = convert_xmlelement_to_widget(xmlElement);
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

static JWidget convert_xmlelement_to_widget(TiXmlElement* elem)
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

    widget = jbutton_new(text ? TRANSLATE_ATTR(text): NULL);
    if (widget) {
      bool left   = bool_attr_is_true(elem, "left");
      bool right  = bool_attr_is_true(elem, "right");
      bool top    = bool_attr_is_true(elem, "top");
      bool bottom = bool_attr_is_true(elem, "bottom");
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

	jbutton_set_bevel(widget, b[0], b[1], b[2], b[3]);
      }
    }
  }
  /* check */
  else if (ustrcmp(elem_name, "check") == 0) {
    const char *text = elem->Attribute("text");
    const char *looklike = elem->Attribute("looklike");

    text = (text ? TRANSLATE_ATTR(text): NULL);

    if (looklike != NULL && strcmp(looklike, "button") == 0) {
      widget = ji_generic_button_new(text, JI_CHECK, JI_BUTTON);
    }
    else {
      widget = jcheck_new(text);
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

      widget = jentry_new(ustrtol(maxsize, NULL, 10),
			  text ? TRANSLATE_ATTR(text): NULL);

      if (readonly)
	jentry_readonly(widget, true);
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
      widget = ji_generic_button_new(text, JI_RADIO, JI_BUTTON);
      if (widget)
	jradio_set_group(widget, radio_group);
    }
    else {
      widget = jradio_new(text, radio_group);
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

    widget = jslider_new(min_value, max_value, min_value);
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

  /* the widget was created? */
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

    /* children */
    TiXmlElement* child_elem = elem->FirstChildElement();
    while (child_elem) {
      child = convert_xmlelement_to_widget(child_elem);
      if (child) {
	/* attach the child in the view */
	if (widget->type == JI_VIEW) {
	  jview_attach(widget, child);
	  break;
	}
	/* add the child in the grid */
	else if (widget->type == JI_GRID) {
	  const char* cell_hspan = child_elem->Attribute("cell_hspan");
	  const char* cell_vspan = child_elem->Attribute("cell_vspan");
	  const char* cell_align = child_elem->Attribute("cell_align");
	  int hspan = cell_hspan ? ustrtol(cell_hspan, NULL, 10): 1;
	  int vspan = cell_vspan ? ustrtol(cell_vspan, NULL, 10): 1;
	  int align = cell_align ? convert_align_value_to_flags(cell_align): 0;

	  jgrid_add_child(widget, child, hspan, vspan, align);
	}
	/* just add the child in any other kind of widget */
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
