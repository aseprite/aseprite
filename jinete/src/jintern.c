/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include "jinete/manager.h"
#include "jinete/theme.h"
#include "jinete/widget.h"
#include "jinete/window.h"

static JID nwidgets = 0;
static JWidget *widgets = NULL;

JWidget _ji_get_widget_by_id (JID widget_id)
{
  /* ji_assert ((widget_id >= 0) && (widget_id < nwidgets)); */

  return widgets[widget_id];
}

JWidget *_ji_get_widget_array (int *n)
{
  *n = nwidgets;
  return widgets;
}

JWidget _ji_get_new_widget (void)
{
  JID widget_id;

  /* first widget */
  if (!widgets) {
    nwidgets = 2;
    widgets = (JWidget *)jmalloc (sizeof (JWidget) * nwidgets);

    /* id=0 no widget */
    widgets[0] = NULL;

    /* id>0 all widgets */
    widgets[1] = (JWidget)jnew (struct jwidget, 1);
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
      widgets = (JWidget *)jrealloc (widgets,
				     sizeof (JWidget) * nwidgets);
      widgets[widget_id] = (JWidget)jnew (struct jwidget, 1);
    }

    /* using this */
    widgets[widget_id]->id = widget_id;
  }

  return widgets[widget_id];
}

void _ji_free_widget (JWidget widget)
{
  widgets[widget->id]->id = 0;
}

void _ji_free_all_widgets (void)
{
  int c;

  if (nwidgets) {
    for (c=0; c<nwidgets; c++)
      jfree (widgets[c]);

    jfree (widgets);
    widgets = NULL;
    nwidgets = 0;
  }
}

bool _ji_is_valid_widget (JWidget widget)
{
  return (widget &&
	  widget->id >= 0 &&
	  widget->id < nwidgets &&
	  widgets[widget->id] &&
	  widgets[widget->id]->id == widget->id);
}

void _ji_set_font_of_all_widgets (struct FONT *f)
{
  int c, l, t, r, b;

  for (c=0; c<nwidgets; c++)
    if (_ji_is_valid_widget (widgets[c])) {
      l = widgets[c]->border_width.l;
      t = widgets[c]->border_width.t;
      r = widgets[c]->border_width.r;
      b = widgets[c]->border_width.b;

      jwidget_set_font (widgets[c], f);
      jwidget_init_theme (widgets[c]);

      widgets[c]->border_width.l = l;
      widgets[c]->border_width.t = t;
      widgets[c]->border_width.r = r;
      widgets[c]->border_width.b = b;
    }

  for (c=0; c<nwidgets; c++)
    if (_ji_is_valid_widget (widgets[c])) {
      if (widgets[c]->type == JI_WINDOW)
	jwindow_remap (widgets[c]);
    }

  jmanager_refresh_screen ();
}



