/* Jinete - a GUI library
 * Copyright (C) 2003, 2004, 2005, 2007, 2008 David A. Capello.
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

#ifndef JINETE_XML_H
#define JINETE_XML_H

#include "jinete/jbase.h"

JI_BEGIN_DECLS

enum { JI_XML_ELEM, JI_XML_TEXT };

struct jxml
{
  JXmlElem root;
};

struct jxmlattr
{
  char *name;
  char *value;
};

struct jxmlnode
{
  int type;
  char *value;
  JXmlNode parent;
  JList children;
};

struct jxmlelem
{
  struct jxmlnode head;
  JList attributes;
};

struct jxmltext
{
  struct jxmlnode head;
};

/* JXml **********************************************/

JXml jxml_new(void);
JXml jxml_new_from_file(const char *filename);
JXml jxml_new_from_string(const char *buffer);
JXml jxml_new_from_stream(JStream stream);
void jxml_free(JXml xml);

JXmlElem jxml_get_root(JXml xml);
JXmlElem jxml_get_elem_by_id(JXml xml, const char *id);

/* JXmlAttr ******************************************/

JXmlAttr jxmlattr_new(const char *name, const char *value);
void jxmlattr_free(JXmlAttr attr);

const char *jxmlattr_get_name(JXmlAttr attr);
const char *jxmlattr_get_value(JXmlAttr attr);
void jxmlattr_set_name(JXmlAttr attr, const char *name);
void jxmlattr_set_value(JXmlAttr attr, const char *value);

/* JXmlNode ******************************************/

JXmlNode jxmlnode_new(int type, const char *value);
void jxmlnode_free(JXmlNode node);

const char *jxmlnode_get_value(JXmlNode node);
void jxmlnode_set_value(JXmlNode node, const char *value);

void jxmlnode_add_child(JXmlNode node, JXmlNode child);
void jxmlnode_remove_child(JXmlNode node, JXmlNode child);

/* JXmlElem ******************************************/

JXmlElem jxmlelem_new(const char *name);
void jxmlelem_free(JXmlElem elem);

const char *jxmlelem_get_name(JXmlElem elem);
void jxmlelem_set_name(JXmlElem elem, const char *name);

bool jxmlelem_has_attr(JXmlElem elem, const char *name);
const char *jxmlelem_get_attr(JXmlElem elem, const char *name);
void jxmlelem_set_attr(JXmlElem elem, const char *name, const char *value);

JXmlElem jxmlelem_get_elem_by_id(JXmlElem elem, const char *id);

/* JXmlText ******************************************/

JXmlText jxmltext_new(const char *value);
void jxmltext_free(JXmlText text);

const char *jxmltext_get_text(JXmlText text);
void jxmltext_set_text(JXmlText text, const char *value);

JI_END_DECLS

#endif /* JINETE_XML_H */
