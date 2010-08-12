/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/jinete.h"

#include "console.h"
#include "app.h"
#include "core/core.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/palette.h"
#include "raster/quant.h"
#include "raster/rotate.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "settings/settings.h"
#include "sprite_wrappers.h"
#include "ui_context.h"
#include "undoable.h"
#include "util/clipboard.h"
#include "util/misc.h"
#include "widgets/colbar.h"
#include "widgets/statebar.h"

#if defined ALLEGRO_WINDOWS
  #include <winalleg.h>
  #include "util/clipboard_win32.h"
#endif

#define SCALE_MODE	0
#define ROTATE_MODE	1

enum {
  ACTION_SETMODE,
  ACTION_MOVE,
  ACTION_SCALE_TL,
  ACTION_SCALE_T,
  ACTION_SCALE_TR,
  ACTION_SCALE_L,
  ACTION_SCALE_R,
  ACTION_SCALE_BL,
  ACTION_SCALE_B,
  ACTION_SCALE_BR,
  ACTION_ROTATE_TL,
  ACTION_ROTATE_T,
  ACTION_ROTATE_TR,
  ACTION_ROTATE_L,
  ACTION_ROTATE_R,
  ACTION_ROTATE_BL,
  ACTION_ROTATE_B,
  ACTION_ROTATE_BR,
};

static void set_clipboard(Image* image, Palette* palette, bool set_system_clipboard);
static bool copy_from_sprite(const Sprite* sprite);

static bool interactive_transform(Editor* widget, Image *dest_image, Image *image,
				  int x, int y, int xout[4], int yout[4]);
static void apply_rotation(int x1, int y1, int x2, int y2,
			   fixed angle, int cx, int cy,
			   int xout[4], int yout[4]);
static void draw_box(BITMAP *bmp,
		     int cx1, int cy1, int cx2, int cy2,
		     int x1, int y1, int x2, int y2, BITMAP *preview,
		     int mode, fixed angle, int cx, int cy);
static void draw_icon(BITMAP *bmp, int x, int y, int mode, fixed angle);
static void fill_in_vars(int *in_box,
			 int *in_left, int *in_center, int *in_right,
			 int *in_top, int *in_middle, int *in_bottom,
			 int x1, int y1, int x2, int y2, fixed angle,
			 int cx, int cy);
static void update_status_bar(Editor* editor, Image *image,
			      int x1, int y1, int x2, int y2, fixed angle);

static bool first_time = true;

static Palette* clipboard_palette = NULL;
static Image* clipboard_image = NULL;

static void on_exit_delete_clipboard()
{
  delete clipboard_palette;
  delete clipboard_image;
}

static void set_clipboard(Image* image, Palette* palette, bool set_system_clipboard)
{
  if (first_time) {
    first_time = false;
    App::instance()->Exit.connect(&on_exit_delete_clipboard);
  }

  delete clipboard_palette;
  delete clipboard_image;

  clipboard_palette = palette;
  clipboard_image = image;

  // copy to the Windows clipboard
#ifdef ALLEGRO_WINDOWS
  if (set_system_clipboard)
    set_win32_clipboard_bitmap(image, palette);
#endif
}

static bool copy_from_sprite(const Sprite* sprite)
{
  ASSERT(sprite != NULL);
  Image* image = NewImageFromMask(sprite);
  if (!image)
    return false;

  const Palette* pal = sprite->getPalette(sprite->getCurrentFrame());
  set_clipboard(image, pal ? new Palette(*pal): NULL, true);
  return true;
}

bool clipboard::can_paste()
{
#ifdef ALLEGRO_WINDOWS
  if (win32_clipboard_contains_bitmap())
    return true;
#endif
  return clipboard_image != NULL;
}

void clipboard::cut(SpriteWriter& sprite)
{
  ASSERT(sprite != NULL);
  ASSERT(sprite->getCurrentLayer() != NULL);

  if (!copy_from_sprite(sprite)) {
    Console console;
    console.printf("Can't copying an image portion from the current layer\n");
  }
  else {
    {
      Undoable undoable(sprite, "Cut");
      undoable.clear_mask(app_get_color_to_clear_layer(sprite->getCurrentLayer()));
      undoable.deselect_mask();
      undoable.commit();
    }
    sprite->generateMaskBoundaries();
    update_screen_for_sprite(sprite);
  }
}

void clipboard::copy(const SpriteReader& sprite)
{
  ASSERT(sprite != NULL);

  if (!copy_from_sprite(sprite)) {
    Console console;
    console.printf(_("Can't copying an image portion from the current layer\n"));
  }
}

void clipboard::copy_image(Image* image, Palette* pal)
{
  set_clipboard(image_new_copy(image),
		pal ? new Palette(*pal): NULL, true);
}

void clipboard::paste(SpriteWriter& sprite)
{
  int xout[4], yout[4];
  int dst_x, dst_y;
  Image *src_image;
  Image* dst_image;
  bool paste;

#ifdef ALLEGRO_WINDOWS
  {
    Image* win32_image = NULL;
    Palette* win32_palette = NULL;
    get_win32_clipboard_bitmap(win32_image, win32_palette);
    if (win32_image != NULL)
      set_clipboard(win32_image, win32_palette, false);
  }
#endif
  if (clipboard_image == NULL)
    return;

  ASSERT(sprite != NULL);

  // destination image (where to put this image)
  dst_image = sprite->getCurrentImage(&dst_x, &dst_y);
  if (!dst_image) {
    Console console;
    console.printf(_("Error: no destination image\n"));
    return;
  }

  // source image (clipboard or a converted copy to the destination 'imgtype')
  if (clipboard_image->imgtype == sprite->getImgType())
    src_image = clipboard_image;
  else {
    RgbMap* rgbmap = sprite->getRgbMap();
    src_image = image_set_imgtype(clipboard_image, sprite->getImgType(), DITHERING_NONE,
				  rgbmap, sprite->getPalette(sprite->getCurrentFrame()));
  }

  // do the interactive-transform loop (where the user can move the floating image)
  {
    JWidget view = jwidget_get_view(current_editor);
    JRect vp = jview_get_viewport_position(view);
    int x, y, x1, y1, x2, y2;

    current_editor->screen_to_editor(vp->x1, vp->y1, &x1, &y1);
    current_editor->screen_to_editor(vp->x2-1, vp->y2-1, &x2, &y2);
    x = (x1+x2)/2-src_image->w/2;
    y = (y1+y2)/2-src_image->h/2;

    paste = interactive_transform(current_editor,
				  dst_image, src_image, x, y, xout, yout);

    jrect_free(vp);
  }

  if (paste) {
    int c, w, h, u1, v1, u2, v2;

    /* align to the destination cel-position */
    for (c=0; c<4; ++c) {
      xout[c] -= dst_x;
      yout[c] -= dst_y;
    }

    /* clip the box for the undo */
    u1 = MAX(0, MIN(xout[0], MIN(xout[1], MIN(xout[2], xout[3]))));
    v1 = MAX(0, MIN(yout[0], MIN(yout[1], MIN(yout[2], yout[3]))));
    u2 = MIN(dst_image->w-1, MAX(xout[0], MAX(xout[1], MAX(xout[2], xout[3]))));
    v2 = MIN(dst_image->h-1, MAX(yout[0], MAX(yout[1], MAX(yout[2], yout[3]))));

    w = u2-u1+1;
    h = v2-v1+1;

    if (w >= 1 && h >= 1) {
      /* undo region */
      if (undo_is_enabled(sprite->getUndo()))
	undo_image(sprite->getUndo(), dst_image, u1, v1, w, h);

      /* draw the transformed image */
      image_parallelogram(dst_image, src_image,
			  xout[0], yout[0], xout[1], yout[1],
			  xout[2], yout[2], xout[3], yout[3]);
    }
  }

  if (src_image != clipboard_image)
      image_free(src_image);
  update_screen_for_sprite(sprite);
}

/**********************************************************************/
/* interactive transform */

enum { DONE_NONE, DONE_CANCEL, DONE_PASTE };

static bool interactive_transform(Editor* editor,
				  Image *dest_image, Image *image,
				  int x, int y,
				  int xout[4], int yout[4])
{
#define UPDATE()							\
  jmouse_hide();							\
  old_screen = ji_screen;						\
  ji_screen = bmp1;							\
  jmanager_dispatch_messages(ji_get_default_manager());			\
  ji_screen = old_screen;						\
  REDRAW();								\
  jmouse_show();

#define REDRAW()							\
  jmouse_hide();							\
  blit(bmp1, bmp2, vp->x1, vp->y1, 0, 0, jrect_w(vp), jrect_h(vp));	\
  draw_box(bmp2,							\
	   0, 0, jrect_w(vp)-1, jrect_h(vp)-1,				\
	   x1-vp->x1, y1-vp->y1, x2-vp->x1, y2-vp->y1,			\
	   preview, mode, angle, cx-vp->x1, cy-vp->y1);			\
  blit(bmp2, ji_screen, 0, 0, vp->x1, vp->y1, jrect_w(vp), jrect_h(vp)); \
  update_status_bar(editor, image, x1, y1, x2, y2, angle);		\
  jmouse_show();

  int x1, y1, x2, y2;
  int u1, v1, u2, v2;
  int action = ACTION_SETMODE;
  int mode = SCALE_MODE;
  BITMAP *bmp1, *bmp2, *preview, *old_screen;
  JRect vp = jview_get_viewport_position(jwidget_get_view(editor));
  int done = DONE_NONE;
  fixed angle = 0;
  int cx, cy;
  int mask_color;

  editor->hide_drawing_cursor();

  editor->editor_to_screen(x, y, &x1, &y1);
  editor->editor_to_screen(x+image->w, y+image->h, &x2, &y2);
  cx = (x1+x2)/2;
  cy = (y1+y2)/2;

  /* generate a bitmap to save the viewport content and other to make
     double-buffered */
  bmp1 = create_bitmap(JI_SCREEN_W, JI_SCREEN_H);
  bmp2 = create_bitmap(jrect_w(vp), jrect_h(vp));

  jmouse_hide();
  blit(ji_screen, bmp1, 0, 0, 0, 0, JI_SCREEN_W, JI_SCREEN_H);
  jmouse_show();

  /* generate the preview bitmap (for fast-blitting) */
  preview = create_bitmap(image->w, image->h);
  mask_color = bitmap_mask_color(preview);
  image_to_allegro(image, preview, 0, 0, get_current_palette());

  switch (image->imgtype) {

    case IMAGE_RGB: {
      int x, y;
      for (y=0; y<image->h; y++)
	for (x=0; x<image->w; x++)
	  if (_rgba_geta(image_getpixel_fast<RgbTraits>(image, x, y)) < 128)
	    putpixel(preview, x, y, mask_color);
      break;
    }

    case IMAGE_GRAYSCALE: {
      int x, y;
      for (y=0; y<image->h; y++)
	for (x=0; x<image->w; x++)
	  if (_graya_geta(image_getpixel_fast<GrayscaleTraits>(image, x, y)) < 128)
	    putpixel(preview, x, y, mask_color);
      break;
    }

    case IMAGE_INDEXED: {
      int x, y;
      for (y=0; y<image->h; y++)
	for (x=0; x<image->w; x++)
	  if (image_getpixel_fast<IndexedTraits>(image, x, y) == 0)
	    putpixel(preview, x, y, mask_color);
      break;
    }
  }

  /* update the bitmaps */
  UPDATE();

  while (done == DONE_NONE) {
    poll_keyboard();

    if (keypressed()) {
      int c = readkey();
      fixed old_angle = angle;

      switch (c>>8) {
	case KEY_ESC:   done = DONE_CANCEL; break;	/* cancel */
	case KEY_ENTER: done = DONE_PASTE; break;	/* paste */
	case KEY_LEFT:  angle = fixadd(angle, itofix(1));  break;
	case KEY_RIGHT: angle = fixsub(angle, itofix(1));  break;
	case KEY_UP:    angle = fixadd(angle, itofix(32)); break;
	case KEY_DOWN:  angle = fixsub(angle, itofix(32)); break;
      }

      if (old_angle != angle) {
	angle &= 255<<16;
	REDRAW();
      }
    }

    /* mouse moved */
    if (jmouse_poll()) {
      int in_left, in_center, in_right;
      int in_top, in_middle, in_bottom;
      int in_box;

      fill_in_vars(&in_box,
		   &in_left, &in_center, &in_right,
		   &in_top, &in_middle, &in_bottom,
		   x1, y1, x2, y2, angle, cx, cy);

      if (in_box) {
	jmouse_set_cursor(JI_CURSOR_SCROLL);
	action = ACTION_MOVE;
      }
      else {
	/* top */
	if (in_top && in_left) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_TL);
	  action = mode == SCALE_MODE ? ACTION_SCALE_TL: ACTION_ROTATE_TL;
	}
	else if (in_top && in_center) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_T);
	  action = mode == SCALE_MODE ? ACTION_SCALE_T: ACTION_ROTATE_T;
	}
	else if (in_top && in_right) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_TR);
	  action = mode == SCALE_MODE ? ACTION_SCALE_TR: ACTION_ROTATE_TR;
	}
	/* middle */
	else if (in_middle && in_left) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_L);
	  action = mode == SCALE_MODE ? ACTION_SCALE_L: ACTION_ROTATE_L;
	}
	else if (in_middle && in_right) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_R);
	  action = mode == SCALE_MODE ? ACTION_SCALE_R: ACTION_ROTATE_R;
	}
	/* bottom */
	else if (in_bottom && in_left) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_BL);
	  action = mode == SCALE_MODE ? ACTION_SCALE_BL: ACTION_ROTATE_BL;
	}
	else if (in_bottom && in_center) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_B);
	  action = mode == SCALE_MODE ? ACTION_SCALE_B: ACTION_ROTATE_B;
	}
	else if (in_bottom && in_right) {
	  jmouse_set_cursor(JI_CURSOR_SIZE_BR);
	  action = mode == SCALE_MODE ? ACTION_SCALE_BR: ACTION_ROTATE_BR;
	}
	/* normal */
	else {
	  jmouse_set_cursor(JI_CURSOR_NORMAL);
	  action = ACTION_SETMODE;
	}
      }
    }

    /* button pressed */
    if (jmouse_b(0)) {
      /* left button+shift || middle button = scroll movement */
      if ((jmouse_b(0) == 1 && (key_shifts & KB_SHIFT_FLAG)) ||
	  (jmouse_b(0) == 4)) {
	JWidget view = jwidget_get_view(editor);
	int scroll_x, scroll_y;

	x = jmouse_x(0) - jmouse_x(1);
	y = jmouse_y(0) - jmouse_y(1);

/* 	screen_to_editor (widget, x1, y1, &x1, &y1); */
/* 	screen_to_editor (widget, x2, y2, &x2, &y2); */

	/* TODO */

	jview_get_scroll(view, &scroll_x, &scroll_y);
	editor->editor_set_scroll(scroll_x-x, scroll_y-y, true);

/* 	editor_to_screen (widget, x1, y1, &x1, &y1); */
/* 	editor_to_screen (widget, x2, y2, &x2, &y2); */

	jmouse_control_infinite_scroll(vp);

	jwidget_dirty(view);
	jwidget_flush_redraw(view);
	UPDATE();

	/* recenter the pivot (cx, cy) */
/* 	{ */
/* 	  MATRIX m; */
/* 	  fixed fx, fy, fz; */
/* 	  /\* new pivot position with transformation *\/ */
/* 	  int ncx = (x1+x2)/2; */
/* 	  int ncy = (y1+y2)/2; */

/* 	  get_rotation_matrix (&m, 0, 0, angle); */

/* 	  /\* new pivot position in the screen *\/ */
/* 	  apply_matrix (&m, itofix(ncx-cx), itofix(ncy-cy), 0, &fx, &fy, &fz); */
/* 	  cx = cx+fixtoi(fx); */
/* 	  cy = cy+fixtoi(fy); */

/* 	  /\* move all vertices to leave the pivot as the center  *\/ */
/* 	  x1 += cx - ncx; */
/* 	  y1 += cy - ncy; */
/* 	  x2 += cx - ncx; */
/* 	  y2 += cy - ncy; */

/* 	  jmouse_hide(); */
/* 	  blit (bmp1, bmp2, 0, 0, 0, 0, vp->w, vp->h); */
/* 	  draw_box (bmp2, */
/* 		    0, 0, vp->w-1, vp->h-1, */
/* 		    x1-vp->x, y1-vp->y, x2-vp->x, y2-vp->y, */
/* 		    preview, mode, angle, cx-vp->x, cy-vp->y); */
/* 	  blit (bmp2, ji_screen, 0, 0, vp->x, vp->y, vp->w, vp->h); */
/* 	  update_status_bar (widget, image, x1, y1, x2, y2, angle); */
/* 	  jmouse_show(); */
/* 	} */
      }
      /* right button = paste */
      else if (jmouse_b(0) == 2) {
	done = DONE_PASTE; 		/* paste */
      }
      /* change mode */
      else if (action == ACTION_SETMODE) {
	mode = (mode == SCALE_MODE) ? ROTATE_MODE: SCALE_MODE;
	REDRAW();

	do {
	  poll_keyboard();
	  jmouse_poll();
	  gui_feedback();
	} while (jmouse_b(0));
      }
      /* modify selection */
      else {
	int mx = jmouse_x(0);
	int my = jmouse_y(0);
	fixed angle1 = angle;
	fixed angle2 = fixatan2(itofix(jmouse_y(0)-cy),
				itofix(jmouse_x(0)-cx));
	angle2 = fixsub(0, angle2);

	u1 = x1;
	v1 = y1;
	u2 = x2;
	v2 = y2;

	do {
	  poll_keyboard();
	  if (jmouse_poll()) {

	    if (action == ACTION_MOVE) {
	      x = jmouse_x(0) - mx;
	      y = jmouse_y(0) - my;
	    }
	    else if (action >= ACTION_SCALE_TL &&
		     action <= ACTION_SCALE_BR) {
	      x = fixtoi(fixmul(itofix(jmouse_x(0) - mx), fixcos(angle)))
		+ fixtoi(fixmul(itofix(jmouse_y(0) - my),-fixsin(angle)));
	      y = fixtoi(fixmul(itofix(jmouse_x(0) - mx), fixsin(angle)))
		+ fixtoi(fixmul(itofix(jmouse_y(0) - my), fixcos(angle)));
	    }
	    else
	      x = y = 0;

	    x1 = u1;
	    y1 = v1;
	    x2 = u2;
	    y2 = v2;

	    switch (action) {
	      case ACTION_MOVE:
		x1 += x;
		y1 += y;
		x2 += x;
		y2 += y;
		cx = (x1+x2)/2;
		cy = (y1+y2)/2;
		break;
	      case ACTION_SCALE_L:
		x1 = MIN(x1+x, x2);
		break;
	      case ACTION_SCALE_T:
		y1 = MIN(y1+y, y2);
		break;
	      case ACTION_SCALE_R:
		x2 = MAX(x2+x, x1);
		break;
	      case ACTION_SCALE_B:
		y2 = MAX(y2+y, y1);
		break;
	      case ACTION_SCALE_TL:
		x1 = MIN(x1+x, x2);
		y1 = MIN(y1+y, y2);
		break;
	      case ACTION_SCALE_TR:
		x2 = MAX(x2+x, x1);
		y1 = MIN(y1+y, y2);
		break;
	      case ACTION_SCALE_BL:
		x1 = MIN(x1+x, x2);
		y2 = MAX(y2+y, y1);
		break;
	      case ACTION_SCALE_BR:
		x2 = MAX(x2+x, x1);
		y2 = MAX(y2+y, y1);
		break;
	      case ACTION_ROTATE_TL:
	      case ACTION_ROTATE_T:
	      case ACTION_ROTATE_TR:
	      case ACTION_ROTATE_L:
	      case ACTION_ROTATE_R:
	      case ACTION_ROTATE_BL:
	      case ACTION_ROTATE_B:
	      case ACTION_ROTATE_BR:
		angle = fixatan2(itofix(jmouse_y(0)-cy),
				 itofix(jmouse_x(0)-cx));
		angle &= 255<<16;
		angle = fixsub(0, angle);

		angle = fixadd(angle1, fixsub (angle, angle2));
		break;
	    }

	    editor->screen_to_editor(x1, y1, &x1, &y1);
	    editor->screen_to_editor(x2, y2, &x2, &y2);

	    // if (UIContext::instance()->getSettings()->get_snap_to_grid() && angle == 0) {
	    //   int ox = x1;
	    //   int oy = y1;
	    //   apply_grid(&x1, &y1, false);
	    //   x2 += x1 - ox;
	    //   y2 += y1 - oy;
	    // }

	    editor->editor_to_screen(x1, y1, &x1, &y1);
	    editor->editor_to_screen(x2, y2, &x2, &y2);

	    /* redraw the screen */
	    REDRAW();
	  }

	  gui_feedback();
	} while (jmouse_b(0));

	/* recenter the pivot (cx, cy) */
	{
	  MATRIX m;
	  fixed fx, fy, fz;
	  /* new pivot position with transformation */
	  int ncx = (x1+x2)/2;
	  int ncy = (y1+y2)/2;

	  get_rotation_matrix(&m, 0, 0, angle);

	  /* new pivot position in the screen */
	  apply_matrix(&m, itofix(ncx-cx), itofix(ncy-cy), 0, &fx, &fy, &fz);
	  cx = cx+fixtoi(fx);
	  cy = cy+fixtoi(fy);

	  /* move all vertices to leave the pivot as the center  */
	  x1 += cx - ncx;
	  y1 += cy - ncy;
	  x2 += cx - ncx;
	  y2 += cy - ncy;

	  REDRAW();
	}
      }
    }

    gui_feedback();
  }

  if (done == DONE_PASTE) {
    int c;
    apply_rotation(x1, y1, x2, y2, angle, cx, cy, xout, yout);
    for (c=0; c<4; c++)
      editor->screen_to_editor(xout[c], yout[c], xout+c, yout+c);
  }

  destroy_bitmap(bmp1);
  destroy_bitmap(bmp2);
  destroy_bitmap(preview);

  clear_keybuf();

  /* restore the cursor */
  editor->show_drawing_cursor();

  jrect_free(vp);
  return done == DONE_PASTE;
}

static void apply_rotation(int x1, int y1, int x2, int y2,
			   fixed angle, int cx, int cy,
			   int xout[4], int yout[4])
{
#define APPLYMATRIX(_x,_y,n)						\
  apply_matrix (&m, itofix (_x-cx), itofix (_y-cy), 0, &fx, &fy, &fz);	\
  xout[n] = cx+fixtoi(fx);						\
  yout[n] = cy+fixtoi(fy);

  MATRIX m;
  fixed fx, fy, fz;

  get_rotation_matrix (&m, 0, 0, angle);
  APPLYMATRIX(x1,y1,0);
  APPLYMATRIX(x2,y1,1);
  APPLYMATRIX(x2,y2,2);
  APPLYMATRIX(x1,y2,3);
}

static void draw_box(BITMAP *bmp,
		     int cx1, int cy1, int cx2, int cy2,
		     int x1, int y1, int x2, int y2,
		     BITMAP *preview, int mode, fixed angle,
		     int cx, int cy)
{
  fixed xs[4], ys[4];
  int x[4], y[4];
  int c;

  set_clip_rect(bmp, cx1, cy1, cx2, cy2);

  /* calculate corner positions */
  apply_rotation(x1, y1, x2, y2, angle, cx, cy, x, y);

  /* draw the preview */
  for (c=0; c<4; c++) {
    xs[c] = itofix(x[c]);
    ys[c] = itofix(y[c]);
  }
  _parallelogram_map_standard(bmp, preview, xs, ys);

  /* draw bounds */
#if 1
  simple_dotted_mode(bmp, makecol(0, 0, 0), makecol(255, 255, 255));
  line(bmp, x[0], y[0], x[1], y[1], 0xffffff);
  line(bmp, x[0], y[0], x[3], y[3], 0xffffff);
  line(bmp, x[2], y[2], x[1], y[1], 0xffffff);
  line(bmp, x[2], y[2], x[3], y[3], 0xffffff);
  solid_mode();
#endif

  /* draw icons */
#define DRAWICON(n1,n2,_angle)						\
  draw_icon(bmp, (x[n1]+x[n2])/2, (y[n1]+y[n2])/2, mode, _angle)

  DRAWICON(1, 2, angle);
  DRAWICON(1, 1, fixadd(angle, itofix(32)));
  DRAWICON(0, 1, fixadd(angle, itofix(64)));
  DRAWICON(0, 0, fixadd(angle, itofix(96)));
  DRAWICON(0, 3, fixadd(angle, itofix(128)));
  DRAWICON(3, 3, fixadd(angle, itofix(160)));
  DRAWICON(3, 2, fixadd(angle, itofix(192)));
  DRAWICON(2, 2, fixadd(angle, itofix(224)));

  set_clip_rect(bmp, 0, 0, bmp->w-1, bmp->h-1);
}

static void draw_icon(BITMAP *bmp, int x, int y, int mode, fixed angle)
{
  BITMAP *gfx;

  angle &= (255<<16);

  /* 0 degree */
  if ((angle > ((256-16)<<16)) || (angle <= ((0+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_3: GFX_ROTATE_3);
    draw_sprite_h_flip (bmp, gfx, x, y-gfx->h/2);
  }
  /* 45 degree */
  else if ((angle >= ((32-16)<<16)) && (angle <= ((32+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_1: GFX_ROTATE_1);
    draw_sprite_h_flip (bmp, gfx, x, y-gfx->h);
  }
  /* 90 degree */
  else if ((angle >= ((64-16)<<16)) && (angle <= ((64+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_2: GFX_ROTATE_2);
    draw_sprite (bmp, gfx, x-gfx->w/2, y-gfx->h);
  }
  /* 135 degree */
  else if ((angle >= ((96-16)<<16)) && (angle <= ((96+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_1: GFX_ROTATE_1);
    draw_sprite (bmp, gfx, x-gfx->w, y-gfx->h);
  }
  /* 180 degree */
  else if ((angle >= ((128-16)<<16)) && (angle <= ((128+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_3: GFX_ROTATE_3);
    draw_sprite (bmp, gfx, x-gfx->w, y-gfx->h/2);
  }
  /* 225 degree */
  else if ((angle >= ((160-16)<<16)) && (angle <= ((160+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_1: GFX_ROTATE_1);
    draw_sprite_v_flip (bmp, gfx, x-gfx->w, y);
  }
  /* 270 degree */
  else if ((angle >= ((192-16)<<16)) && (angle <= ((192+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_2: GFX_ROTATE_2);
    draw_sprite_v_flip (bmp, gfx, x-gfx->w/2, y);
  }
  /* 315 degree */
  else if ((angle >= ((224-16)<<16)) && (angle <= ((224+16)<<16))) {
    gfx = get_gfx (mode == SCALE_MODE ? GFX_SCALE_1: GFX_ROTATE_1);
    draw_sprite_vh_flip (bmp, gfx, x, y);
  }
}

static void fill_in_vars(int *in_box,
			 int *in_left, int *in_center, int *in_right,
			 int *in_top, int *in_middle, int *in_bottom,
			 int x1, int y1, int x2, int y2, fixed angle,
			 int cx, int cy)
{
  MATRIX m;
  int mx = jmouse_x(0);
  int my = jmouse_y(0);
  fixed fx, fy, fz;

  get_rotation_matrix (&m, 0, 0, fixsub (0, angle));
  apply_matrix (&m, itofix (mx-cx), itofix (my-cy), 0, &fx, &fy, &fz);
  mx = cx+fixtoi (fx);
  my = cy+fixtoi (fy);

  *in_box = (mx >= x1 && my >= y1 && mx <= x2 && my <= y2);
  *in_left = (mx >= x1-12 && mx < x1);
  *in_top  = (my >= y1-12 && my < y1);
  *in_right  = (mx > x2 && mx <= x2+12);
  *in_bottom = (my > y2 && my <= y2+12);
  *in_center = (mx > (x1+x2)/2-6 && mx < (x1+x2)/2+6);
  *in_middle = (my > (y1+y2)/2-6 && my < (y1+y2)/2+6);
}

static void update_status_bar(Editor* editor, Image *image,
			      int x1, int y1, int x2, int y2, fixed angle)
{
  int u1, v1, u2, v2;
  int iangle = 360*(fixtoi (angle & (255<<16)))/256;

  editor->screen_to_editor(x1, y1, &u1, &v1);
  editor->screen_to_editor(x2, y2, &u2, &v2);

  app_get_statusbar()->setStatusText
    (0,
     "Pos: %3d %3d Size: %3d %3d Orig: %3d %3d (%.02f%% %.02f%%) Angle: %3d",
     u1, v1, u2-u1, v2-v1,
     image->w, image->h,
     (double)(u2-u1)*100/image->w,
     (double)(v2-v1)*100/image->h,
     iangle);

  jwidget_flush_redraw(app_get_statusbar());
  jmanager_dispatch_messages(ji_get_default_manager());
}
