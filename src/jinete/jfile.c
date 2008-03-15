/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
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
 *   * Neither the name of the Jinete nor the names of its contributors may
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

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

static JWidget convert_tag_to_widget(JXmlElem elem);

JWidget ji_load_widget(const char *filename, const char *name)
{
  JWidget widget = NULL;
  JXml xml;
  JList children;
  JLink link;

  xml = jxml_new_from_file(filename);
  if (!xml)
    return NULL;

  /* search the requested widget */
  children = jxml_get_root(xml)->head.children;
  JI_LIST_FOR_EACH(children, link) {
    JXmlNode node = link->data;

    /* if this node is an XML Element and has a "name" property... */
    if (node->type == JI_XML_ELEM &&
	jxmlelem_has_attr((JXmlElem)node, "name")) {
      /* ...then we can compare both names (the requested one with the
	 element's name) */
      const char *nodename = jxmlelem_get_attr((JXmlElem)node, "name");
      if (nodename != NULL && ustrcmp(nodename, name) == 0) {
	/* convert tags to Jinete widgets */
	widget = convert_tag_to_widget((JXmlElem)node);
	break;
      }
    }
  }

  jxml_free(xml);
  return widget;
}

static JWidget convert_tag_to_widget(JXmlElem elem)
{
  const char *elem_name = jxmlelem_get_name(elem);
  JWidget widget = NULL;
  JWidget child;

  /* box */
  if (ustrcmp(elem_name, "box") == 0) {
    bool horizontal  = jxmlelem_has_attr(elem, "horizontal");
    bool vertical    = jxmlelem_has_attr(elem, "vertical");
    bool homogeneous = jxmlelem_has_attr(elem, "homogeneous");

    widget = jbox_new((horizontal ? JI_HORIZONTAL:
		       vertical ? JI_VERTICAL: 0) |
		      (homogeneous ? JI_HOMOGENEOUS: 0));
  }
  /* button */
  else if (ustrcmp(elem_name, "button") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");

    if (text) {
      widget = jbutton_new(TRANSLATE_ATTR(text));
      if (widget) {
	bool left   = jxmlelem_has_attr(elem, "left");
	bool right  = jxmlelem_has_attr(elem, "right");
	bool top    = jxmlelem_has_attr(elem, "top");
	bool bottom = jxmlelem_has_attr(elem, "bottom");
	char *bevel = jxmlelem_get_attr(elem, "bevel");

	jwidget_set_align(widget,
			  (left ? JI_LEFT: (right ? JI_RIGHT: JI_CENTER)) |
			  (top ? JI_TOP: (bottom ? JI_BOTTOM: JI_MIDDLE)));

	if (bevel != NULL) {
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
	  jbutton_set_bevel(widget, b[0], b[1], b[2], b[3]);
	}
      }
    }
  }
  /* check */
  else if (ustrcmp(elem_name, "check") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");

    if (text) {
      widget = jcheck_new(TRANSLATE_ATTR(text));
      if (widget) {
	bool center = jxmlelem_has_attr(elem, "center");
	bool right  = jxmlelem_has_attr(elem, "right");
	bool top    = jxmlelem_has_attr(elem, "top");
	bool bottom = jxmlelem_has_attr(elem, "bottom");

	jwidget_set_align(widget,
			  (center ? JI_CENTER:
			   (right ? JI_RIGHT: JI_LEFT)) |
			  (top    ? JI_TOP:
			   (bottom ? JI_BOTTOM: JI_MIDDLE)));
      }
    }
  }
  /* combobox */
  else if (ustrcmp(elem_name, "combobox") == 0) {
    widget = jcombobox_new();
  }
  /* entry */
  else if (ustrcmp(elem_name, "entry") == 0) {
    const char *maxsize = jxmlelem_get_attr(elem, "maxsize");
    const char *text = jxmlelem_get_attr(elem, "text");

    if (maxsize != NULL) {
      bool readonly = jxmlelem_has_attr(elem, "readonly");

      widget = jentry_new(ustrtol(maxsize, NULL, 10),
			  text != NULL ? TRANSLATE_ATTR(text): "");

      if (readonly)
	jentry_readonly(widget, TRUE);
    }
  }
  /* label */
  else if (ustrcmp(elem_name, "label") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");

    if (text) {
      widget = jlabel_new(TRANSLATE_ATTR(text));
      if (widget) {
	bool center = jxmlelem_has_attr(elem, "center");
	bool right  = jxmlelem_has_attr(elem, "right");
	bool top    = jxmlelem_has_attr(elem, "top");
	bool bottom = jxmlelem_has_attr(elem, "bottom");

	jwidget_set_align(widget,
			  (center ? JI_CENTER:
			   (right ? JI_RIGHT: JI_LEFT)) |
			  (top    ? JI_TOP:
			   (bottom ? JI_BOTTOM: JI_MIDDLE)));
      }
    }
  }
  /* listbox */
  else if (ustrcmp(elem_name, "listbox") == 0) {
    widget = jlistbox_new();
  }
  /* listitem */
  else if (ustrcmp(elem_name, "listitem") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");

    if (text != NULL)
      widget = jlistitem_new(TRANSLATE_ATTR(text));
  }
  /* panel */
  else if (ustrcmp(elem_name, "panel") == 0) {
    bool horizontal = jxmlelem_has_attr(elem, "horizontal");
    bool vertical = jxmlelem_has_attr(elem, "vertical");

    widget = jpanel_new(horizontal ? JI_HORIZONTAL:
			vertical ? JI_VERTICAL: 0);
  }
  /* radio */
  else if (ustrcmp(elem_name, "radio") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");
    const char *group = jxmlelem_get_attr(elem, "group");

    if (text != NULL && group != NULL) {
      widget = jradio_new(TRANSLATE_ATTR(text),
			  ustrtol(group, NULL, 10));
      if (widget) {
	bool center = jxmlelem_has_attr(elem, "center");
	bool right  = jxmlelem_has_attr(elem, "right");
	bool top    = jxmlelem_has_attr(elem, "top");
	bool bottom = jxmlelem_has_attr(elem, "bottom");

	jwidget_set_align(widget,
			  (center ? JI_CENTER:
			   (right ? JI_RIGHT: JI_LEFT)) |
			  (top    ? JI_TOP:
			   (bottom ? JI_BOTTOM: JI_MIDDLE)));
      }
    }
  }
  /* separator */
  else if (ustrcmp(elem_name, "separator") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");
    bool center      = jxmlelem_has_attr(elem, "center");
    bool right       = jxmlelem_has_attr(elem, "right");
    bool middle      = jxmlelem_has_attr(elem, "middle");
    bool bottom      = jxmlelem_has_attr(elem, "bottom");
    bool horizontal  = jxmlelem_has_attr(elem, "horizontal");
    bool vertical    = jxmlelem_has_attr(elem, "vertical");

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
    const char *min = jxmlelem_get_attr(elem, "min");
    const char *max = jxmlelem_get_attr(elem, "max");
    int min_value = min != NULL ? ustrtol(min, NULL, 10): 0;
    int max_value = max != NULL ? ustrtol(max, NULL, 10): 0;

    widget = jslider_new(min_value, max_value, min_value);
  }
  /* textbox */
  else if (ustrcmp(elem_name, "textbox") == 0) {
    bool wordwrap = jxmlelem_has_attr(elem, "wordwrap");

    /* TODO add translatable support */
    /* TODO here we need jxmlelem_get_text(elem) */
/* widget = jtextbox_new(tag->text, wordwrap ? JI_WORDWRAP: 0); */
  }
  /* view */
  else if (ustrcmp(elem_name, "view") == 0) {
    widget = jview_new();
  }
  /* window */
  else if (ustrcmp(elem_name, "window") == 0) {
    const char *text = jxmlelem_get_attr(elem, "text");

    if (text) {
      bool desktop = jxmlelem_has_attr(elem, "desktop");

      if (desktop)
	widget = jwindow_new_desktop();
      else
	widget = jwindow_new(TRANSLATE_ATTR(text));
    }
  }

  /* the widget was created? */
  if (widget) {
    const char *name      = jxmlelem_get_attr(elem, "name");
    const char *tooltip   = jxmlelem_get_attr(elem, "tooltip");
    bool expansive        = jxmlelem_has_attr(elem, "expansive");
    bool magnetic         = jxmlelem_has_attr(elem, "magnetic");
    bool noborders        = jxmlelem_has_attr(elem, "noborders");
    const char *width     = jxmlelem_get_attr(elem, "width");
    const char *height    = jxmlelem_get_attr(elem, "height");
    const char *minwidth  = jxmlelem_get_attr(elem, "minwidth");
    const char *minheight = jxmlelem_get_attr(elem, "minheight");
    const char *maxwidth  = jxmlelem_get_attr(elem, "maxwidth");
    const char *maxheight = jxmlelem_get_attr(elem, "maxheight");
    JLink link;

    if (name != NULL)
      jwidget_set_name(widget, name);

    if (tooltip != NULL)
      jwidget_add_tooltip_text(widget, tooltip);

    if (expansive)
      jwidget_expansive(widget, TRUE);

    if (magnetic)
      jwidget_magnetic(widget, TRUE);

    if (noborders)
      jwidget_noborders(widget);

    if (width || minwidth || maxwidth ||
	height || minheight || maxheight) {
      int w = (width || minwidth) ? ustrtol(width ? width: minwidth, NULL, 10): 0;
      int h = (height || minheight) ? ustrtol(height ? height: minheight, NULL, 10): 0;
      jwidget_set_min_size(widget, w, h);

      w = (width || maxwidth) ? strtol(width ? width: maxwidth, NULL, 10): INT_MAX;
      h = (height || maxheight) ? strtol(height ? height: maxheight, NULL, 10): INT_MAX;
      jwidget_set_max_size(widget, w, h);
    }

    /* children */
    JI_LIST_FOR_EACH(elem->head.children, link) {
      if (((JXmlNode)link->data)->type == JI_XML_ELEM) {
	child = convert_tag_to_widget(link->data);
	if (child != NULL) {
	  if (widget->type == JI_VIEW) {
	    jview_attach(widget, child);
	    break;
	  }
	  else
	    jwidget_add_child(widget, child);
	}
      }
    }

    if (widget->type == JI_VIEW) {
      bool maxsize = jxmlelem_has_attr(elem, "maxsize");
      if (maxsize)
	jview_maxsize(widget);
    }
  }

  return widget;
}
