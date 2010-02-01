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

#include <allegro.h>

#include "jinete/jinete.h"

static const char *xml_src =
  "<root>"				"\n"
  "  <item>"				"\n"
  "    <value1>text"			"\n"
  "    </value1>"			"\n"
  "    more text"			"\n"
  "  </item>"				"\n"
  "</root>";

static JWidget textsrc, textdst;

static void read_xml(JWidget widget);
static void prt_xmlnode(JXmlNode node, int indent);
static void prt(const char *format, ...);

int main(int argc, char *argv[])
{
  JWidget manager, window, hbox, vbox, button;

  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  manager = jmanager_new();
  ji_set_standard_theme();

  window = jwindow_new("XML");
  hbox = jpanel_new(JI_VERTICAL);
  vbox = jbox_new(JI_VERTICAL);
  textsrc = jtextbox_new(xml_src, JI_WORDWRAP);
  textdst = jtextbox_new("", JI_WORDWRAP);
  button = jbutton_new("Read XML");

  jbutton_add_command(button, read_xml);

  jwidget_expansive(hbox, true);
  jwidget_expansive(textsrc, true);
  jwidget_expansive(textdst, true);
  jwidget_set_min_size(textsrc, 600, 200);
  jwidget_set_min_size(textdst, 600, 200);

  jwidget_add_child(hbox, textsrc);
  jwidget_add_child(hbox, textdst);
  jwidget_add_child(vbox, hbox);
  jwidget_add_child(vbox, button);
  jwidget_add_child(window, vbox);

  window->open_window_bg();

  jmanager_run(manager);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();

static void read_xml(JWidget widget)
{
  JXml xml;

  prt("Processing XML...\n\n");

  xml = jxml_new_from_string(jwidget_get_text(textsrc));
  if (!xml) {
    prt("ERROR\n");
    return;
  }

  prt_xmlnode((JXmlNode)xml->root, 0);

  jxml_free(xml);
}

static void prt_xmlnode(JXmlNode node, int indent)
{
  JLink link;
  int c;

  for (c=0; c<indent; c++)
    prt("  ");
  prt("JXmlNode->value = %s\n", node->value);

  JI_LIST_FOR_EACH(node->children, link)
    prt_xmlnode(reinterpret_cast<JXmlNode>(link->data), indent+1);
}

static void prt(const char *format, ...)
{
  const char *text;
  char buf[1024];
  char *final;
  va_list ap;

  va_start(ap, format);
  uvsprintf(buf, format, ap);
  va_end(ap);

  text = jwidget_get_text(textdst);
  if (!text)
    final = jstrdup(buf);
  else {
    final = (char*)jmalloc(ustrlen(text) + ustrlen(buf) + 1);

    ustrcpy(final, empty_string);
    ustrcat(final, text);
    ustrcat(final, buf);
  }

  jwidget_set_text(textdst, final);
  jfree(final);
}

