/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jinete.h"

/* determine is the characer is a blank space */
#define IS_BLANK(chr) (((chr) ==  ' ') ||  \
                       ((chr) == '\t') || \
                       ((chr) == '\n') || \
                       ((chr) == '\r'))

#define TRANSLATE_ATTR(attr)						    \
  ((attr->translatable) ? ji_translate_string (attr->value): (attr->value))

typedef struct Attr
{
  char *name;
  char *value;
  bool translatable : 1;
} Attr;

typedef struct Tag
{
  char *name;
  char *text;
  JList attr;
  JList sub_tags;
} Tag;

static JWidget convert_tag_to_widget(Tag *tag);
static JList read_tags(FILE *f);

static Attr *attr_new(char *name, char *value, bool translatable);
static void attr_free(Attr *attr);

static Tag *tag_new(const char *name);
static Tag *tag_new_from_string(char *tag_string);
static void tag_free(Tag *tag);
static void tag_add_attr(Tag *tag, Attr *attr);
static void tag_add_tag(Tag *tag, Tag *child);
static Attr *tag_get_attr(Tag *tag, const char *name);

JWidget ji_load_widget(const char *filename, const char *name)
{
  JWidget widget = NULL;
  Attr *attr;
  FILE *f;
  Tag *tag;
  JList tags;
  JLink link;

  f = fopen(filename, "rt");	/* open the file */
  if (!f)
    return 0;
  tags = read_tags(f);		/* read the tags */
  fclose(f);			/* and close it */

  /* search the requested widget */
  JI_LIST_FOR_EACH(tags, link) {
    tag = link->data;
    attr = tag_get_attr(tag, "name");

    if (attr && strcmp(attr->value, name) == 0) {
      /* convert tags to Jinete widgets */
      widget = convert_tag_to_widget(tag);
      break;
    }
  }

  JI_LIST_FOR_EACH(tags, link)
    tag_free(link->data);
  jlist_free(tags);

  return widget;
}

static JWidget convert_tag_to_widget(Tag *tag)
{
  JWidget widget = NULL;
  JWidget child;

  /* box */
  if (strcmp(tag->name, "box") == 0) {
    Attr *horizontal = tag_get_attr(tag, "horizontal");
    Attr *vertical = tag_get_attr(tag, "vertical");
    Attr *homogeneous = tag_get_attr(tag, "homogeneous");

    widget = jbox_new((horizontal ? JI_HORIZONTAL:
			 vertical ? JI_VERTICAL: 0) |
			(homogeneous ? JI_HOMOGENEOUS: 0));
  }
  /* button */
  else if (strcmp(tag->name, "button") == 0) {
    Attr *text = tag_get_attr(tag, "text");

    if (text) {
      widget = jbutton_new (TRANSLATE_ATTR (text));
      if (widget) {
	Attr *left = tag_get_attr(tag, "left");
	Attr *right = tag_get_attr(tag, "right");
	Attr *top = tag_get_attr(tag, "top");
	Attr *bottom = tag_get_attr(tag, "bottom");

	jwidget_set_align(widget,
			    (left ? JI_LEFT:
			     right ? JI_RIGHT: JI_CENTER) |
			    (top ? JI_TOP:
			     bottom ? JI_BOTTOM: JI_MIDDLE));
      }
    }
  }
  /* check */
  else if (strcmp(tag->name, "check") == 0) {
    Attr *text = tag_get_attr(tag, "text");

    if (text) {
      widget = jcheck_new(TRANSLATE_ATTR(text));
      if (widget) {
	Attr *center = tag_get_attr(tag, "center");
	Attr *right = tag_get_attr(tag, "right");
	Attr *top = tag_get_attr(tag, "top");
	Attr *bottom = tag_get_attr(tag, "bottom");

	jwidget_set_align(widget,
			    (center ? JI_CENTER:
			     right ? JI_RIGHT: JI_LEFT) |
			    (top ? JI_TOP:
			     bottom ? JI_BOTTOM: JI_MIDDLE));
      }
    }
  }
  /* combobox */
  else if (strcmp(tag->name, "combobox") == 0) {
    widget = jcombobox_new();
  }
  /* entry */
  else if (strcmp(tag->name, "entry") == 0) {
    Attr *maxsize = tag_get_attr(tag, "maxsize");
    Attr *text = tag_get_attr(tag, "text");

    if (maxsize && maxsize->value) {
      widget = jentry_new(strtol(maxsize->value, NULL, 10),
			    text && text->value ?
			    TRANSLATE_ATTR(text): "");
    }
  }
  /* label */
  else if (strcmp (tag->name, "label") == 0) {
    Attr *text = tag_get_attr(tag, "text");

    if (text) {
      widget = jlabel_new(TRANSLATE_ATTR (text));
      if (widget) {
	Attr *center = tag_get_attr(tag, "center");
	Attr *right = tag_get_attr(tag, "right");
	Attr *top = tag_get_attr(tag, "top");
	Attr *bottom = tag_get_attr(tag, "bottom");

	jwidget_set_align(widget,
			    (center ? JI_CENTER:
			     right ? JI_RIGHT: JI_LEFT) |
			    (top ? JI_TOP:
			     bottom ? JI_BOTTOM: JI_MIDDLE));
      }
    }
  }
  /* listbox */
  else if (strcmp(tag->name, "listbox") == 0) {
    widget = jlistbox_new();
  }
  /* listitem */
  else if (strcmp(tag->name, "listitem") == 0) {
    Attr *text = tag_get_attr(tag, "text");

    if (text)
      widget = jlistitem_new(TRANSLATE_ATTR(text));
  }
  /* panel */
  else if (strcmp(tag->name, "panel") == 0) {
    Attr *horizontal = tag_get_attr(tag, "horizontal");
    Attr *vertical = tag_get_attr(tag, "vertical");

    widget = ji_panel_new(horizontal ? JI_HORIZONTAL:
			  vertical ? JI_VERTICAL: 0);
  }
  /* radio */
  else if (strcmp(tag->name, "radio") == 0) {
    Attr *text = tag_get_attr(tag, "text");
    Attr *group = tag_get_attr(tag, "group");

    if (text && group && group->value) {
      widget = jradio_new(TRANSLATE_ATTR(text),
			    strtol (group->value, NULL, 10));
      if (widget) {
	Attr *center = tag_get_attr(tag, "center");
	Attr *right = tag_get_attr(tag, "right");
	Attr *top = tag_get_attr(tag, "top");
	Attr *bottom = tag_get_attr(tag, "bottom");

	jwidget_set_align(widget,
			    (center ? JI_CENTER:
			     right ? JI_RIGHT: JI_LEFT) |
			    (top ? JI_TOP:
			     bottom ? JI_BOTTOM: JI_MIDDLE));
      }
    }
  }
  /* separator */
  else if (strcmp(tag->name, "separator") == 0) {
    Attr *text = tag_get_attr(tag, "text");
    Attr *center = tag_get_attr(tag, "center");
    Attr *right = tag_get_attr(tag, "right");
    Attr *middle = tag_get_attr(tag, "middle");
    Attr *bottom = tag_get_attr(tag, "bottom");
    Attr *horizontal = tag_get_attr(tag, "horizontal");
    Attr *vertical = tag_get_attr(tag, "vertical");

    widget = ji_separator_new(text ? TRANSLATE_ATTR (text): NULL,
			      (horizontal ? JI_HORIZONTAL: 0) |
			      (vertical ? JI_VERTICAL: 0) |
			      (center ? JI_CENTER:
			       right ? JI_RIGHT: JI_LEFT) |
			      (middle ? JI_MIDDLE:
			       bottom ? JI_BOTTOM: JI_TOP));
  }
  /* slider */
  else if (strcmp(tag->name, "slider") == 0) {
    Attr *min = tag_get_attr(tag, "min");
    Attr *max = tag_get_attr(tag, "max");
    int min_value = min && min->value? strtol(min->value, NULL, 10): 0;
    int max_value = max && max->value? strtol(max->value, NULL, 10): 0;

    widget = jslider_new(min_value, max_value, min_value);
  }
  /* textbox */
  else if (strcmp(tag->name, "textbox") == 0) {
    Attr *wordwrap = tag_get_attr(tag, "wordwrap");

    /* XXX add translatable support */
    widget = jtextbox_new(tag->text, wordwrap ? JI_WORDWRAP: 0);
  }
  /* view */
  else if (strcmp(tag->name, "view") == 0) {
    widget = jview_new();
  }
  /* window */
  else if (strcmp(tag->name, "window") == 0) {
    Attr *text = tag_get_attr(tag, "text");

    if (text) {
      Attr *desktop = tag_get_attr(tag, "desktop");

      if (desktop)
	widget = jwindow_new_desktop();
      else
	widget = jwindow_new(TRANSLATE_ATTR (text));
    }
  }

  /* the widget was created? */
  if (widget) {
    Attr *name = tag_get_attr (tag, "name");
    Attr *expansive = tag_get_attr (tag, "expansive");
    Attr *magnetic = tag_get_attr (tag, "magnetic");
    Attr *noborders = tag_get_attr (tag, "noborders");
    JLink link;

    if (name) jwidget_set_name (widget, name->value);
    if (expansive) jwidget_expansive (widget, TRUE);
    if (magnetic) jwidget_magnetic (widget, TRUE);
    if (noborders) jwidget_noborders (widget);

    /* children */
    JI_LIST_FOR_EACH(tag->sub_tags, link) {
      child = convert_tag_to_widget(link->data);
      if (child) {
	if (widget->type == JI_VIEW) {
	  jview_attach(widget, child);
	  break;
	}
	else
	  jwidget_add_child(widget, child);
      }
    }

    if (widget->type == JI_VIEW) {
      Attr *maxsize = tag_get_attr(tag, "maxsize");
      if (maxsize)
	jview_maxsize(widget);
    }
  }

  return widget;
}

static JList read_tags(FILE *f)
{
  JList root_tags = jlist_new();
  JList parent_stack = jlist_new();
  char *s, buf[1024];

  while (fgets (buf, sizeof (buf), f)) {
    for (s=buf; *s; s++) {
      /* tag beginning */
      if (*s == '<') {
	char *tag_start = s + 1;
	int open = (s[1] != '/');
	bool auto_closed = FALSE;
	Tag *tag;

	/* comment? */
	if (s[1] == '!' && s[2] == '-' && s[3] == '-') {
	  s += 2;
	  for (;;) {
	    if (strncmp (s, "-->", 3) == 0) {
	      s += 2;
	      break;
	    }
	    else if (*s == 0) {
	      if (!fgets (buf+strlen (buf), sizeof (buf)-strlen (buf), f))
		break;
	      s = buf;
	    }
	    else {
	      s++;
	    }
	  }
	  continue;
	}

	/* go to end of the tag */
	for (; ; s++) {
	  if (*s == '>') {
	    if (*(s-1) == '/') {
	      *(s-1) = 0;
	      auto_closed = TRUE;
	    }
	    break;
	  }
	  else if (*s == '\"') {
	    for (s++; ; s++) {
	      if (*s == 0) {
		if (!fgets (buf+strlen (buf), sizeof (buf)-strlen (buf), f))
		  break;
		s--;
	      }
	      else if ((*s == '\"') && (*(s-1) != '\\')) {
		break;
	      }
	    }

	    if (feof (f))
	      break;
	  }
	  else if (*s == 0) {
	    if (!fgets (buf+strlen (buf), sizeof (buf)-strlen (buf), f))
	      break;
	    s--;
	  }
	}

	*s = 0;

	/* create the new tag */
	tag = tag_new_from_string (open ? tag_start: tag_start+1);

	if (tag) {
/* 	  fprintf (stderr, "%s tag: %s (parent %s)\n", */
/* 		   open ? "open": "close", */
/* 		   tag->name, */
/* 		   !jlist_empty(parent_stack) ? ((Tag *)parent_stack->data)->name: "ROOT"); */

	  /* open a level */
	  if (open) {
	    /* add this tag in parent list */
	    if (jlist_empty(parent_stack))
	      jlist_append(root_tags, tag);
	    else
	      tag_add_tag(jlist_first(parent_stack)->data, tag);

	    /* if it isn't closed in the same open tag */
	    if (!auto_closed)
	      /* add to the parent stack */
	      jlist_prepend(parent_stack, tag);
	  }
	  /* close a level */
	  else {
	    if ((!jlist_empty(parent_stack)) &&
		(strcmp(tag->name, ((Tag *)jlist_first(parent_stack)->data)->name) == 0)) {
	      /* remove the first tag from stack */
	      jlist_remove(parent_stack, jlist_first(parent_stack)->data);
	    }
	    else {
	      /* XXX error msg */
	      /* printf ("you must open the tag before close it\n"); */
	    }

	    tag_free(tag);
	  }
	}
      }
      /* put characters in the last tag-text */
      else {
	if (!jlist_empty(parent_stack)) {
	  Tag *tag = jlist_first(parent_stack)->data;

	  if (tag->text || IS_BLANK(*s)) {
	    int len = tag->text ? strlen (tag->text): 0;
	    tag->text = jrealloc (tag->text, len+2);
	    tag->text[len] = *s;
	    tag->text[len+1] = 0;
	  }
	}
      }
    }
  }

  jlist_free(parent_stack);
  return root_tags;
}

static Attr *attr_new(char *name, char *value, bool translatable)
{
  Attr *attr;

  attr = jnew(Attr, 1);
  if (!attr)
    return NULL;

  attr->name = name;
  attr->value = value;
  attr->translatable = translatable;

  return attr;
}

static void attr_free(Attr *attr)
{
  if (attr->name)
    jfree(attr->name);

  if (attr->value)
    jfree(attr->value);

  jfree(attr);
}

static Tag *tag_new(const char *name)
{
  Tag *tag;

  tag = jnew(Tag, 1);
  if (!tag)
    return NULL;

  tag->name = jstrdup(name);
  tag->text = NULL;
  tag->attr = jlist_new();
  tag->sub_tags = jlist_new();

  return tag;
}

static Tag *tag_new_from_string (char *tag_string)
{
  char c, *s;
  Tag *tag;

  /* find the end of the tag-name */
  for (s=tag_string; *s && !IS_BLANK (*s); s++);

  /* create the new tag with the found name */
  c = *s;
  *s = 0;
  tag = tag_new (tag_string);
  *s = c;

  /* continue reading attributes */
  while (*s) {
    /* jump white spaces */
    while (*s && IS_BLANK (*s))
      s++;

    /* isn't end of string? */
    if (*s) {
      char *name_beg = s;
      char *name;
      char *value = NULL;
      bool translatable = FALSE;
      Attr *attr;

      /* read the attribute-name */
      while (*s && !IS_BLANK (*s) && *s != '=')
	s++;

      c = *s;
      *s = 0;
      name = jstrdup (name_beg);
      *s = c;

      if (*s == '=') {
	char *value_beg = ++s;
	bool go_next = FALSE;

	/* see for the translation prefix _() */
	if (strncmp (s, "_(\"", 3) == 0) {
	  translatable = TRUE;
	  s += 2;
	}

	if (*s == '\"') {
	  /* jump the double-quote */
	  value_beg = ++s;

	  /* read the attribute-value */
	  while (*s) {
	    if (*s == '\\') {
	      memmove (s, s+1, strlen (s)-1);
	    }
	    else if (*s == '\"') {
	      go_next = TRUE;
	      break;
	    }
	    s++;
	  }
	}
	else {
	  /* read the attribute-value */
	  while (*s && !IS_BLANK (*s))
	    s++;
	}

	c = *s;
	*s = 0;
	value = jstrdup (value_beg);
	*s = c;

	if (go_next)
	  s++;
      }

      /* create the attribute */
      attr = attr_new (name, value, translatable);

      /* add the attribute to the tag */
      if (attr)
	tag_add_attr (tag, attr);
    }
  }

  return tag;
}

static void tag_free(Tag *tag)
{
  JLink link;

  if (tag->name)
    jfree(tag->name);

  if (tag->text)
    jfree(tag->text);

  JI_LIST_FOR_EACH(tag->attr, link)
    attr_free(link->data);

  JI_LIST_FOR_EACH(tag->sub_tags, link)
    tag_free(link->data);

  jlist_free(tag->attr);
  jlist_free(tag->sub_tags);

  jfree(tag);
}

static void tag_add_attr(Tag *tag, Attr *attr)
{
  jlist_append(tag->attr, attr);
}

static void tag_add_tag(Tag *tag, Tag *child)
{
  jlist_append(tag->sub_tags, child);
}

static Attr *tag_get_attr(Tag *tag, const char *name)
{
  JLink link;
  Attr *attr;

  JI_LIST_FOR_EACH(tag->attr, link) {
    attr = link->data;
    if (strcmp(attr->name, name) == 0)
      return attr;
  }

  return NULL;
}
