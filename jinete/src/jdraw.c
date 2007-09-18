/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/font.h"
#include "jinete/intern.h"
#include "jinete/list.h"
#include "jinete/rect.h"
#include "jinete/region.h"
#include "jinete/system.h"
#include "jinete/widget.h"

/* XXXX optional anti-aliased textout */
#define SETUP_ANTIALISING(f, bg, fill_bg)				\
  ji_font_set_aa_mode (f, fill_bg ||					\
			  bitmap_color_depth (ji_screen) == 8 ? bg: -1)

void jdraw_rect(const JRect r, int color)
{
  rect(ji_screen, r->x1, r->y1, r->x2-1, r->y2-1, color);
}

void jdraw_rectfill(const JRect r, int color)
{
  rectfill(ji_screen, r->x1, r->y1, r->x2-1, r->y2-1, color);
}

void jdraw_rectedge(const JRect r, int c1, int c2)
{
  vline(ji_screen, r->x1,   r->y1,   r->y2-1, c1);
  vline(ji_screen, r->x2-1, r->y1,   r->y2-1, c2);

  hline(ji_screen, r->x1+1, r->y1,   r->x2-2, c1);
  hline(ji_screen, r->x1+1, r->y2-1, r->x2-2, c2);
}

void jdraw_rectshade(const JRect rect, int c1, int c2, int align)
{
  int c, x1, y1, x2, y2, r[2], g[2], b[2];

  x1 = rect->x1;
  y1 = rect->y1;
  x2 = rect->x2-1;
  y2 = rect->y2-1;

  r[0] = getr (c1);
  g[0] = getg (c1);
  b[0] = getb (c1);

  r[1] = getr (c2);
  g[1] = getg (c2);
  b[1] = getb (c2);

  if (align & JI_VERTICAL) {
    if (y1 == y2)
      hline (ji_screen, x1, y1, x2, c1);
    else
      for (c=y1; c<=y2; c++)
	hline (ji_screen,
	       x1, c, x2,
	       makecol ((r[0] + (r[1] - r[0]) * (c - y1) / (y2 - y1)),
			(g[0] + (g[1] - g[0]) * (c - y1) / (y2 - y1)),
			(b[0] + (b[1] - b[0]) * (c - y1) / (y2 - y1))));
  }
  else if (align & JI_HORIZONTAL) {
    if (x1 == x2)
      vline (ji_screen, x1, y1, y2, c1);
    else
      for (c=x1; c<=x2; c++)
	vline(ji_screen,
	      c, y1, y2,
	      makecol((r[0] + (r[1] - r[0]) * (c - x1) / (x2 - x1)),
		      (g[0] + (g[1] - g[0]) * (c - x1) / (x2 - x1)),
		      (b[0] + (b[1] - b[0]) * (c - x1) / (x2 - x1))));
  }
}

void jdraw_rectexclude(const JRect rc, const JRect exclude, int color)
{
  _ji_theme_rectfill_exclude(ji_screen,
			     rc->x1, rc->y1,
			     rc->x2-1, rc->y2-1,
			     exclude->x1, exclude->y1,
			     exclude->x2-1, exclude->y2-1, color);
}

void jdraw_char(FONT *f, int chr, int x, int y, int fg, int bg, bool fill_bg)
{
  SETUP_ANTIALISING(f, bg, fill_bg);

  f->vtable->render_char(f, chr, fg, fill_bg ? bg: -1, ji_screen, x, y);
}

/* see ji_font_text_len */
void jdraw_text(FONT *font, const char *s, int x, int y,
		int fg_color, int bg_color, bool fill_bg)
{
  /* original code from allegro/src/guiproc.c */
  char tmp[1024];
  int hline_pos = -1;
  int len = 0;
  int in_pos = 0;
  int out_pos = 0;
  int c;

  while (((c = ugetc(s+in_pos)) != 0) && (out_pos<(int)(sizeof(tmp)-ucwidth(0)))) {
    if (c == '&') {
      in_pos += uwidth (s+in_pos);
      c = ugetc (s+in_pos);
      if (c == '&') {
	out_pos += usetc (tmp+out_pos, '&');
	in_pos += uwidth (s+in_pos);
	len++;
      }
      else
	hline_pos = len;
    }
    else {
      out_pos += usetc (tmp+out_pos, c);
      in_pos += uwidth (s+in_pos);
      len++;
    }
  }

  usetc (tmp+out_pos, 0);

  SETUP_ANTIALISING (font, bg_color, fill_bg);

  text_mode (fill_bg ? bg_color: -1);
  textout (ji_screen, font, tmp, x, y, fg_color);

  if (hline_pos >= 0) {
    c = ugetat (tmp, hline_pos);
    usetat (tmp, hline_pos, 0);
    hline_pos = text_length (font, tmp);
    c = usetc (tmp, c);
    usetc (tmp+c, 0);
    c = text_length (font, tmp);
    hline (ji_screen, x+hline_pos,
	   /* y+text_height (font)-1, */
	   y+text_height (font),
	   x+hline_pos+c-1, fg_color);
  }
}

void jdraw_widget_text(JWidget widget, int fg, int bg, bool fill_bg)
{
  if (widget->text) {
    struct jrect box, text, icon;
    jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);
    jdraw_text(widget->text_font, widget->text,
	       text.x1, text.y1, fg, bg, fill_bg);
  }
}

void ji_blit_region(JRegion region, int dx, int dy)
{
  int c, nrects = JI_REGION_NUM_RECTS(region);
  JRect rc;

  /* blit directly screen to screen *************************************/
  if (is_linear_bitmap(ji_screen) && nrects == 1) {
    rc = JI_REGION_RECTS(region);
    blit(ji_screen, ji_screen,
	 rc->x1, rc->y1,
	 rc->x1+dx, rc->y1+dy, jrect_w(rc), jrect_h(rc));
  }
  /* blit saving areas and copy them ************************************/
  else if (nrects > 1) {
    JList images = jlist_new();
    BITMAP *bmp;
    JLink link;

    for (c=0, rc=JI_REGION_RECTS(region);
	 c<nrects;
	 c++, rc++) {
      bmp = create_bitmap(jrect_w(rc), jrect_h(rc));
      blit(ji_screen, bmp,
	   rc->x1, rc->y1, 0, 0, bmp->w, bmp->h);
      jlist_append(images, bmp);
    }

    for (c=0, rc=JI_REGION_RECTS(region), link=jlist_first(images);
	 c<nrects;
	 c++, rc++, link=link->next) {
      bmp = link->data;
      blit(bmp, ji_screen, 0, 0, rc->x1+dx, rc->y1+dy, bmp->w, bmp->h);
      destroy_bitmap(bmp);
    }

    jlist_free(images);
  }
}

#if 0
typedef struct XPM_COLOR {
  char *chars;
  int color;
} XPM_COLOR;

BITMAP *ji_xpm_to_bitmap (const char *xpm_image, int depth)
{
  int w, h, colors, charsbycolor;
  int *colormap;
  const char *p;
  BITMAP *bmp;

  /* read header */
  p = xpm_image[0];
  w = ustrtol (p, (char *)&p, 10);
  h = ustrtol (p, (char *)&p, 10);
  colors = ustrtol (p, (char *)&p, 10);
  charsbycolor = ustrtol (p, (char *)&p, 10);

  /* error? */
  if (w < 1 || h < 1 || colors < 1 || charsbycolor < 1)
    return NULL;

  /* read color map */
  colormap = jmalloc (sizeof (XPM_COLOR) * colors);
  for (c=0; c<colors; c++) {
    colormap[c].chars = jmalloc (charsbycolor);
    colormap[c].color = makecol
  }

  /* read image */
  for (c=0; c<colors; c++) {
  }

  return bmp;
}
#endif
