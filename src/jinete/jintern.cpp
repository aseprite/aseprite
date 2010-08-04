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

#include <vector>

#include "jinete/jmanager.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

static std::vector<JWidget>* widgets;

int _ji_widgets_init()
{
  widgets = new std::vector<JWidget>;
  return 0;
}

void _ji_widgets_exit()
{
  delete widgets;
}

JWidget _ji_get_widget_by_id(JID widget_id)
{
  ASSERT((widget_id >= 0) && (widget_id < widgets->size()));

  return (*widgets)[widget_id];
}

JWidget* _ji_get_widget_array(int* n)
{
  *n = widgets->size();
  return &widgets->front();
}

void _ji_add_widget(JWidget widget)
{
  JID widget_id;

  // first widget
  if (widgets->empty()) {
    widgets->resize(2);

    // id=0 no widget
    (*widgets)[0] = NULL;

    // id>0 all widgets
    (*widgets)[1] = widget;
    (*widgets)[1]->id = widget_id = 1;
  }
  else {
    // find a free slot
    for (widget_id=1; widget_id<widgets->size(); widget_id++) {
      // is it free?
      if ((*widgets)[widget_id] == NULL)
	break;
    }

    // we need space for other widget more?
    if (widget_id == widgets->size())
      widgets->resize(widgets->size()+1);

    (*widgets)[widget_id] = widget;
    (*widgets)[widget_id]->id = widget_id;
  }
}

void _ji_remove_widget(JWidget widget)
{
  ASSERT_VALID_WIDGET(widget);

  (*widgets)[widget->id] = NULL;
}

bool _ji_is_valid_widget(JWidget widget)
{
  return (widget &&
	  widget->id >= 0 &&
	  widget->id < widgets->size() &&
	  (*widgets)[widget->id] &&
	  (*widgets)[widget->id]->id == widget->id);
}

void _ji_set_font_of_all_widgets(FONT* f)
{
  size_t c;

  // first of all, we have to set the font to all the widgets
  for (c=0; c<widgets->size(); c++)
    if (_ji_is_valid_widget((*widgets)[c]))
      (*widgets)[c]->setFont(f);

}

void _ji_reinit_theme_in_all_widgets()
{
  size_t c;

  // Then we can reinitialize the theme of each widget
  for (c=0; c<widgets->size(); c++)
    if (_ji_is_valid_widget((*widgets)[c])) {
      (*widgets)[c]->theme = ji_get_theme();
      jwidget_init_theme((*widgets)[c]);
    }

  // Remap the windows
  for (c=0; c<widgets->size(); c++)
    if (_ji_is_valid_widget((*widgets)[c])) {
      if ((*widgets)[c]->type == JI_FRAME)
	static_cast<Frame*>((*widgets)[c])->remap_window();
    }

  // Refresh the screen
  jmanager_refresh_screen();
}
