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

#include <assert.h>

#include "jinete/jmanager.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

static JID nwidgets = 0;
static JWidget *widgets = NULL;

JWidget _ji_get_widget_by_id(JID widget_id)
{
  assert((widget_id >= 0) && (widget_id < nwidgets));

  return widgets[widget_id];
}

JWidget *_ji_get_widget_array(int *n)
{
  *n = nwidgets;
  return widgets;
}

JWidget _ji_get_new_widget(void)
{
  JID widget_id;

  /* first widget */
  if (!widgets) {
    nwidgets = 2;
    widgets = (JWidget *)jmalloc(sizeof(JWidget) * nwidgets);

    /* id=0 no widget */
    widgets[0] = NULL;

    /* id>0 all widgets */
    widgets[1] = (JWidget)jnew(struct jwidget, 1);
    widgets[1]->id = widget_id = 1;
  }
  else {
    /* find a free slot */
    for (widget_id=1; widget_id<nwidgets; widget_id++) {
      /* is it free? */
      if (widgets[widget_id]->id != widget_id)
	/* yeah */
	break;
    }

    /* we need make other widget? */
    if (widget_id == nwidgets) {
      nwidgets++;
      widgets = (JWidget *)jrealloc(widgets,
				    sizeof(JWidget) * nwidgets);
      widgets[widget_id] = (JWidget)jnew(struct jwidget, 1);
    }

    /* using this */
    widgets[widget_id]->id = widget_id;
  }

  return widgets[widget_id];
}

void _ji_free_widget(JWidget widget)
{
  widgets[widget->id]->id = 0;
}

void _ji_free_all_widgets(void)
{
  int c;

  if (nwidgets) {
    for (c=0; c<nwidgets; c++)
      if (widgets[c] != NULL)
	jfree(widgets[c]);

    jfree(widgets);
    widgets = NULL;
    nwidgets = 0;
  }
}

bool _ji_is_valid_widget(JWidget widget)
{
  return (widget &&
	  widget->id >= 0 &&
	  widget->id < nwidgets &&
	  widgets[widget->id] &&
	  widgets[widget->id]->id == widget->id);
}

void _ji_set_font_of_all_widgets(struct FONT *f)
{
  int c;

  for (c=0; c<nwidgets; c++)
    if (_ji_is_valid_widget(widgets[c])) {
      jwidget_set_font(widgets[c], f);
      jwidget_init_theme(widgets[c]);
    }

  for (c=0; c<nwidgets; c++)
    if (_ji_is_valid_widget(widgets[c])) {
      if (widgets[c]->type == JI_WINDOW)
	jwindow_remap(widgets[c]);
    }

  jmanager_refresh_screen();
}
