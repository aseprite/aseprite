// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "gfx/point.h"
#include "gfx/size.h"
#include "gui/draw.h"
#include "gui/font.h"
#include "gui/intern.h"
#include "gui/manager.h"
#include "gui/rect.h"
#include "gui/system.h"
#include "gui/theme.h"
#include "gui/view.h"
#include "gui/widget.h"

static Theme* current_theme = NULL;

static void draw_text(BITMAP *bmp, FONT *f, const char *text, int x, int y,
                      int fg_color, int bg_color, bool fill_bg);

Theme::Theme()
{
  this->name = "Theme";
  this->default_font = font;    // Default Allegro font
  this->desktop_color = 0;
  this->textbox_fg_color = 0;
  this->textbox_bg_color = 0;
  this->scrollbar_size = 0;
  this->guiscale = 1;
}

Theme::~Theme()
{
  if (default_font && default_font != font)
    destroy_font(default_font);

  if (current_theme == this)
    CurrentTheme::set(NULL);
}

void Theme::regenerate()
{
  int type = jmouse_get_cursor();
  jmouse_set_cursor(JI_CURSOR_NULL);

  onRegenerate();

  jmouse_set_cursor(type);
}

//////////////////////////////////////////////////////////////////////

void CurrentTheme::set(Theme* theme)
{
  current_theme = theme;

  if (current_theme) {
    current_theme->regenerate();

    gui::Manager* manager = gui::Manager::getDefault();
    if (manager && !manager->getTheme())
      manager->setTheme(theme);
  }
}

Theme* CurrentTheme::get()
{
  return current_theme;
}

int ji_color_foreground()
{
  return current_theme->color_foreground();
}

int ji_color_disabled()
{
  return current_theme->color_disabled();
}

int ji_color_face()
{
  return current_theme->color_face();
}

int ji_color_facelight()
{
  register int c = ji_color_face();
  return makecol(MIN(getr(c)+64, 255),
                 MIN(getg(c)+64, 255),
                 MIN(getb(c)+64, 255));
}

int ji_color_faceshadow()
{
  register int c = ji_color_face();
  return makecol(MAX(getr(c)-64, 0),
                 MAX(getg(c)-64, 0),
                 MAX(getb(c)-64, 0));
}

int ji_color_hotface()
{
  return current_theme->color_hotface();
}

int ji_color_selected()
{
  return current_theme->color_selected();
}

int ji_color_background()
{
  return current_theme->color_background();
}

BITMAP* ji_apply_guiscale(BITMAP* original)
{
  int scale = jguiscale();
  if (scale > 1) {
    BITMAP* scaled = create_bitmap_ex(bitmap_color_depth(original),
                                      original->w*scale,
                                      original->h*scale);

    for (int y=0; y<scaled->h; ++y)
      for (int x=0; x<scaled->w; ++x)
        putpixel(scaled, x, y, getpixel(original, x/scale, y/scale));

    destroy_bitmap(original);
    return scaled;
  }
  else
    return original;
}

void _ji_theme_draw_sprite_color(BITMAP *bmp, BITMAP *sprite,
                                 int x, int y, int color)
{
  int u, v, mask = bitmap_mask_color(sprite);

  for (v=0; v<sprite->h; v++)
    for (u=0; u<sprite->w; u++)
      if (getpixel(sprite, u, v) != mask)
        putpixel(bmp, x+u, y+v, color);
}

void _ji_theme_textbox_draw(BITMAP *bmp, JWidget widget,
                            int *w, int *h, int bg, int fg)
{
  View* view = View::getView(widget);
  char *text = (char*)widget->getText(); // TODO warning: removing const modifier
  char *beg, *end;
  int x1, y1, x2, y2;
  int x, y, chr, len;
  gfx::Point scroll;
  int viewport_w, viewport_h;
  int textheight = jwidget_get_text_height(widget);
  FONT *font = widget->getFont();
  char *beg_end, *old_end;
  int width;

  if (view) {
    gfx::Rect vp = view->getViewportBounds();
    x1 = vp.x;
    y1 = vp.y;
    viewport_w = vp.w;
    viewport_h = vp.h;
    scroll = view->getViewScroll();
  }
  else {
    x1 = widget->rc->x1 + widget->border_width.l;
    y1 = widget->rc->y1 + widget->border_width.t;
    viewport_w = jrect_w(widget->rc) - widget->border_width.l - widget->border_width.r;
    viewport_h = jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b;
    scroll.x = scroll.y = 0;
  }
  x2 = x1+viewport_w-1;
  y2 = y1+viewport_h-1;

  chr = 0;

  /* without word-wrap */
  if (!(widget->getAlign() & JI_WORDWRAP)) {
    width = jrect_w(widget->rc);
  }
  /* with word-wrap */
  else {
    if (w) {
      width = *w;
      *w = 0;
    }
    else {
      /* TODO modificable option? I don't think so, this is very internal stuff */
#if 0
      /* shows more information in x-scroll 0 */
      width = viewport_w;
#else
      /* make good use of the complete text-box */
      if (view) {
        gfx::Size maxSize = view->getScrollableSize();
        width = MAX(viewport_w, maxSize.w);
      }
      else {
        width = viewport_w;
      }
#endif
    }
  }

  /* draw line-by-line */
  y = y1 - scroll.y;
  for (beg=end=text; end; ) {
    x = x1 - scroll.x;

    /* without word-wrap */
    if (!(widget->getAlign() & JI_WORDWRAP)) {
      end = ustrchr(beg, '\n');
      if (end) {
        chr = *end;
        *end = 0;
      }
    }
    /* with word-wrap */
    else {
      old_end = NULL;
      for (beg_end=beg;;) {
        end = ustrpbrk(beg_end, " \n");
        if (end) {
          chr = *end;
          *end = 0;
        }

        /* to here we can print */
        if ((old_end) && (x+text_length(font, beg) > x1-scroll.x+width)) {
          if (end)
            *end = chr;

          end = old_end;
          chr = *end;
          *end = 0;
          break;
        }
        /* we can print one word more */
        else if (end) {
          /* force break */
          if (chr == '\n')
            break;

          *end = chr;
          beg_end = end+1;
        }
        /* we are in the end of text */
        else
          break;

        old_end = end;
      }
    }

    len = text_length(font, beg);

    /* render the text in the "bmp" */
    if (bmp) {
      int xout;

      if (widget->getAlign() & JI_CENTER)
        xout = x + width/2 - len/2;
      else if (widget->getAlign() & JI_RIGHT)
        xout = x + width - len;
      else                      /* left */
        xout = x;

      draw_text(bmp, font, beg, xout, y, fg, bg, true);

      jrectexclude(bmp,
                   x1, y, x2, y+textheight-1,
                   xout, y, xout+len-1, y+textheight-1, bg);
    }

    /* width */
    if (w)
      *w = MAX(*w, len);

    y += textheight;

    if (end) {
      *end = chr;
      beg = end+1;
    }
  }

  /* height */
  if (h)
    *h = (y-y1+scroll.y);

  if (w) *w += widget->border_width.l + widget->border_width.r;
  if (h) *h += widget->border_width.t + widget->border_width.b;

  /* fill bottom area */
  if (bmp) {
    if (y <= y2)
      rectfill(bmp, x1, y, x2, y2, bg);
  }
}

static void draw_text(BITMAP *bmp, FONT *f, const char *text, int x, int y,
                      int fg_color, int bg_color, bool fill_bg)
{
  /* TODO optional anti-aliased textout */
  ji_font_set_aa_mode(f, bg_color);
  textout_ex(bmp, f, text, x, y, fg_color, (fill_bg ? bg_color: -1));
  /* TODO */
}
