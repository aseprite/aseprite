/* Jinete - a GUI library
 * Copyright (c) 2007 David A. Capello
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

#include "jinete.h"

/* determine is the characer is a blank space */
#define IS_BLANK(chr) (((chr) ==  ' ') ||  \
                       ((chr) == '\t') || \
                       ((chr) == '\n') || \
                       ((chr) == '\r'))

static JList read_nodes(JStream stream);
static JXmlElem read_elem(char *elem_string);

/**********************************************************************/
/* JXml                                                               */
/**********************************************************************/

JXml jxml_new(void)
{
  JXml xml= jnew(struct jxml, 1);
  if (!xml)
    return NULL;

  xml->root = NULL;

  return xml;
}

JXml jxml_new_from_file(const char *filename)
{
  JStream stream;
  JXml xml;
  FILE *f;

  /* open the file */
  f = fopen(filename, "rt");
  if (!f)
    return NULL;

  /* read the xml */
  stream = jstream_new_for_file(f);
  xml = jxml_new_from_stream(stream);
  jstream_free(stream);

  /* and close it */
  fclose(f);

  return xml;
}

JXml jxml_new_from_string(const char *buffer)
{
  JStream stream;
  JXml xml;

  stream = jstream_new_for_string(buffer);
  xml = jxml_new_from_stream(stream);
  jstream_free(stream);

  return xml;
}

JXml jxml_new_from_stream(JStream stream)
{
  JList nodes = read_nodes(stream);
  JXml xml = NULL;

  if (!nodes)
    return NULL;

  if (jlist_length(nodes) != 1) {
    JLink link;

    JI_LIST_FOR_EACH(nodes, link) {
      JXmlNode node = (JXmlNode)link->data;
      jxmlnode_free(node);
    }

    /* xml_error("More than one tag in root"); */
  }
  else {
    xml = jxml_new();
    xml->root = jlist_first_data(nodes);
  }

  jlist_free(nodes);
  return xml;
}

void jxml_free(JXml xml)
{
  if (xml->root)
    jxmlelem_free(xml->root);

  jfree(xml);
}

JXmlElem jxml_get_root(JXml xml)
{
  return xml->root;
}

JXmlElem jxml_get_elem_by_id(JXml xml, const char *id)
{
  if (xml->root)
    return jxmlelem_get_elem_by_id(xml->root, id);
  else
    return NULL;
}

/**********************************************************************/
/* JXmlAttr                                                           */
/**********************************************************************/

JXmlAttr jxmlattr_new(const char *name, const char *value)
{
  JXmlAttr attr = jnew(struct jxmlattr, 1);
  attr->name = name ? jstrdup(name): NULL;
  attr->value = value ? jstrdup(value): NULL;
  return attr;
}

void jxmlattr_free(JXmlAttr attr)
{
  if (attr->name)
    jfree(attr->name);

  if (attr->value)
    jfree(attr->value);

  jfree(attr);
}

const char *jxmlattr_get_name(JXmlAttr attr)
{
  return attr->name;
}

const char *jxmlattr_get_value(JXmlAttr attr)
{
  return attr->value;
}

void jxmlattr_set_name(JXmlAttr attr, const char *name)
{
  if (attr->name)
    jfree(attr->name);

  attr->name = name ? jstrdup(name): NULL;
}

void jxmlattr_set_value(JXmlAttr attr, const char *value)
{
  if (attr->value)
    jfree(attr->value);

  attr->value = value ? jstrdup(value): NULL;
}

/**********************************************************************/
/* JXmlNode                                                           */
/**********************************************************************/

JXmlNode jxmlnode_new(int type, const char *value)
{
  JXmlNode node = NULL;

  switch (type) {
    case JI_XML_ELEM:
      node = (JXmlNode)jnew(struct jxmlelem, 1);
      if (node) {
	((JXmlElem)node)->attributes = jlist_new();
      }
      break;
    case JI_XML_TEXT:
      node = (JXmlNode)jnew(struct jxmltext, 1);
      break;
  }

  if (node) {
    node->type = type;
    node->value = value ? jstrdup(value): NULL;
    node->parent = NULL;
    node->children = jlist_new();
  }

  return node;
}

void jxmlnode_free(JXmlNode node)
{
  JLink link;

  switch (node->type) {
    case JI_XML_ELEM:
      JI_LIST_FOR_EACH(((JXmlElem)node)->attributes, link) {
	JXmlAttr child = (JXmlAttr)link->data;
	jxmlattr_free(child);
      }
      jlist_free(((JXmlElem)node)->attributes);
      break;
    case JI_XML_TEXT:
      /* do nothing */
      break;
  }

  if (node->value)
    jfree(node->value);

  JI_LIST_FOR_EACH(node->children, link) {
    JXmlNode child = (JXmlNode)link->data;
    jxmlnode_free(child);
  }
  jlist_free(node->children);

  jfree(node);
}

const char *jxmlnode_get_value(JXmlNode node)
{
  return node->value;
}

void jxmlnode_set_value(JXmlNode node, const char *value)
{
  if (node->value)
    jfree(node->value);

  node->value = value ? jstrdup(value): NULL;
}

void jxmlnode_add_child(JXmlNode node, JXmlNode child)
{
  jlist_append(node->children, child);
  child->parent = node;
}

void jxmlnode_remove_child(JXmlNode node, JXmlNode child)
{
  jlist_remove(node->children, child);
  child->parent = NULL;
}

/**********************************************************************/
/* JXmlElem                                                           */
/**********************************************************************/

JXmlElem jxmlelem_new(const char *name)
{
  return (JXmlElem)jxmlnode_new(JI_XML_ELEM, name);
}

void jxmlelem_free(JXmlElem elem)
{
  jxmlnode_free((JXmlNode)elem);
}

const char *jxmlelem_get_name(JXmlElem elem)
{
  return jxmlnode_get_value((JXmlNode)elem);
}

void jxmlelem_set_name(JXmlElem elem, const char *name)
{
  jxmlnode_set_value((JXmlNode)elem, name);
}

bool jxmlelem_has_attr(JXmlElem elem, const char *name)
{
  JLink link;

  JI_LIST_FOR_EACH(elem->attributes, link) {
    JXmlAttr attr = (JXmlAttr)link->data;
    if (strcmp(name, jxmlattr_get_name(attr)) == 0)
      return TRUE;
  }

  return FALSE;
}

const char *jxmlelem_get_attr(JXmlElem elem, const char *name)
{
  JLink link;

  JI_LIST_FOR_EACH(elem->attributes, link) {
    JXmlAttr attr = (JXmlAttr)link->data;
    if (strcmp(name, jxmlattr_get_name(attr)) == 0) {
      return jxmlattr_get_value(attr);
    }
  }

  return NULL;
}

void jxmlelem_set_attr(JXmlElem elem, const char *name, const char *value)
{
  JXmlAttr attr;
  JLink link;

  /* does the attribute exist? */
  JI_LIST_FOR_EACH(elem->attributes, link) {
    attr = (JXmlAttr)link->data;
    if (strcmp(name, jxmlattr_get_name(attr)) == 0) {
      jxmlattr_set_value(attr, value);
      return;
    }
  }

  attr = jxmlattr_new(name, value);
  jlist_append(elem->attributes, attr);
}

JXmlElem jxmlelem_get_elem_by_id(JXmlElem elem, const char *id)
{
  const char *elem_id = jxmlelem_get_attr(elem, "id");
  JLink link;

  /* fprintf(stderr, "- %s id=%s\n", jxmlelem_get_name(elem), elem_id ? elem_id: "(null)"); */

  /* this is the element with the specified ID */
  if (elem_id && strcmp(elem_id, id) == 0)
    return elem;

  /* go through the children */
  JI_LIST_FOR_EACH(((JXmlNode)elem)->children, link) {
    JXmlNode child = (JXmlNode)link->data;
    if (child->type == JI_XML_ELEM) {
      JXmlElem found = jxmlelem_get_elem_by_id((JXmlElem)child, id);
      if (found)
	return found;
    }
  }

  return NULL;
}

/**********************************************************************/
/* JXmlText                                                           */
/**********************************************************************/

JXmlText jxmltext_new(const char *value)
{
  return (JXmlText)jxmlnode_new(JI_XML_TEXT, value);
}

void jxmltext_free(JXmlText text)
{
  jxmlnode_free((JXmlNode)text);
}

const char *jxmltext_get_text(JXmlText text)
{
  return ((JXmlNode)text)->value;
}

void jxmltext_set_text(JXmlText text, const char *value)
{
  if (((JXmlNode)text)->value)
    jfree(((JXmlNode)text)->value);

  ((JXmlNode)text)->value = value ? jstrdup(value): NULL;
}


/**********************************************************************/
/* READ XML TAGS                                                      */
/**********************************************************************/

static JList read_nodes(JStream stream)
{
  JList root_tags = jlist_new();
  JList parent_stack = jlist_new();
  char *s, buf[1024];

  while (jstream_gets(stream, buf, sizeof(buf))) {
    for (s=buf; *s; s++) {
      /* beginning of a new element */
      if (*s == '<') {
	char *tag_start = s + 1;
	int open = (s[1] != '/');
	bool auto_closed = FALSE;
	JXmlElem tag;

	/* comment? */
	if (s[1] == '!' && s[2] == '-' && s[3] == '-') {
	  s += 2;
	  for (;;) {
	    if (strncmp (s, "-->", 3) == 0) {
	      s += 2;
	      break;
	    }
	    else if (*s == 0) {
	      if (!jstream_gets(stream, buf+strlen(buf), sizeof(buf)-strlen(buf)))
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
		if (!jstream_gets(stream, buf+strlen(buf), sizeof(buf)-strlen(buf)))
		  break;
		s--;
	      }
	      else if ((*s == '\"') && (*(s-1) != '\\')) {
		break;
	      }
	    }

	    if (jstream_eof(stream))
	      break;
	  }
	  else if (*s == 0) {
	    if (!jstream_gets(stream, buf+strlen(buf), sizeof(buf)-strlen(buf)))
	      break;
	    s--;
	  }
	}

	*s = 0;

	/* create the new tag */
	tag = read_elem(open ? tag_start: tag_start+1);
	if (tag) {
/* 	  fprintf (stderr, "%s tag: %s (parent %s)\n", */
/* 		   open ? "open": "close", */
/* 		   jxmlelem_get_name(tag), */
/* 		   !jlist_empty(parent_stack) ? ((JXmlNode)jlist_first_data(parent_stack))->value: "ROOT"); */

	  /* open a level */
	  if (open) {
	    /* add this tag in parent list */
	    if (jlist_empty(parent_stack))
	      jlist_append(root_tags, tag);
	    else
	      jxmlnode_add_child(jlist_first(parent_stack)->data, (JXmlNode)tag);

	    /* if it isn't closed in the same open tag */
	    if (!auto_closed)
	      /* add to the parent stack */
	      jlist_prepend(parent_stack, tag);
	  }
	  /* close a level */
	  else {
	    if ((!jlist_empty(parent_stack)) &&
		(strcmp(((JXmlNode)tag)->value, ((JXmlNode)jlist_first_data(parent_stack))->value) == 0)) {
	      /* remove the first tag from stack */
	      jlist_remove(parent_stack, jlist_first_data(parent_stack));
	    }
	    else {
	      /* TODO error msg */
	      /* printf ("you must open the tag before close it\n"); */
	    }

	    jxmlelem_free(tag);
	  }
	}
      }
      /* put characters in the last JXmlText */
      else {
	if (!jlist_empty(parent_stack)) {
	  /* TODO */
/* 	  JXmlNode tag = jlist_first(parent_stack)->data; */

/* 	  if (tag->value || IS_BLANK(*s)) { */
/* 	    int len = tag->value ? strlen(tag->value): 0; */
/* 	    tag->value = jrealloc(tag->value, len+2); */
/* 	    tag->value[len] = *s; */
/* 	    tag->value[len+1] = 0; */
/* 	  } */
	}
      }
    }
  }

  jlist_free(parent_stack);
  return root_tags;
}

static JXmlElem read_elem(char *elem_string)
{
  char c, *s;
  JXmlElem elem;

  /* find the end of the tag-name */
  for (s=elem_string; *s && !IS_BLANK(*s); s++)
    ;

  /* create the new tag with the found name */
  c = *s;
  *s = 0;
  elem = jxmlelem_new(elem_string);
  *s = c;

  /* continue reading attributes */
  while (*s) {
    /* jump white spaces */
    while (*s && IS_BLANK(*s))
      s++;

    /* isn't end of string? */
    if (*s) {
      char *name_beg = s;
      char *name;
      char *value = NULL;
      bool translatable = FALSE;

      /* read the attribute-name */
      while (*s && !IS_BLANK(*s) && *s != '=')
	s++;

      c = *s;
      *s = 0;
      name = jstrdup(name_beg);
      *s = c;

      if (*s == '=') {
	char *value_beg = ++s;
	bool go_next = FALSE;

	/* see for the translation prefix _() */
	if (strncmp(s, "_(\"", 3) == 0) {
	  translatable = TRUE;
	  s += 2;
	}

	if (*s == '\"') {
	  /* jump the double-quote */
	  value_beg = ++s;

	  /* read the attribute-value */
	  while (*s) {
	    if (*s == '\\') {
	      memmove(s, s+1, strlen(s)-1);
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
	  while (*s && !IS_BLANK(*s))
	    s++;
	}

	c = *s;
	*s = 0;
	value = jstrdup(value_beg);
	*s = c;

	if (go_next)
	  s++;
      }

      /* add the attribute to the tag */
      /* TODO translatable */
      jxmlelem_set_attr(elem, name, value);
      if (name) jfree(name);
      if (value) jfree(value);
    }
  }

  return elem;
}
