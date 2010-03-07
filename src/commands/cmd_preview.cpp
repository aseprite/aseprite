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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "commands/commands.h"
#include "app.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "util/render.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

#define PREVIEW_TILED		1
#define PREVIEW_FIT_ON_SCREEN	2

//////////////////////////////////////////////////////////////////////
// base class

class PreviewCommand : public Command
{
public:
  PreviewCommand(const char* short_name, const char* friendly_name);
  Command* clone() { return new PreviewCommand(*this); }

protected:
  bool enabled(Context* context);

  void preview_sprite(Context* context, int flags);
};

PreviewCommand::PreviewCommand(const char* short_name, const char* friendly_name)
  : Command(short_name, 
	    friendly_name, 
	    CmdUIOnlyFlag)
{
}

bool PreviewCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}


/**
 * Shows the sprite using the complete screen.
 */
void PreviewCommand::preview_sprite(Context* context, int flags)
{
  Editor* editor = current_editor;

  if (editor && editor->getSprite()) {
    Sprite *sprite = editor->getSprite();
    JWidget view = jwidget_get_view(editor);
    int old_mouse_x, old_mouse_y;
    int scroll_x, scroll_y;
    int u, v, x, y, w, h;
    int shiftx, shifty;
    Image *image;
    BITMAP *bmp;
    int redraw;
    JRect vp;
    int bg_color, index_bg_color = -1;
    TiledMode tiled;

    if (flags & PREVIEW_TILED) {
      tiled = context->getSettings()->getTiledMode();
      if (tiled == TILED_NONE)
	tiled = TILED_BOTH;
    }
    else
      tiled = TILED_NONE;

    jmanager_free_mouse();

    vp = jview_get_viewport_position(view);
    jview_get_scroll(view, &scroll_x, &scroll_y);

    old_mouse_x = jmouse_x(0);
    old_mouse_y = jmouse_y(0);

    bmp = create_bitmap(sprite->w, sprite->h);
    if (bmp) {
      /* print a informative text */
      statusbar_set_text(app_get_statusbar(), 1, _("Rendering..."));
      jwidget_flush_redraw(app_get_statusbar());
      jmanager_dispatch_messages(ji_get_default_manager());

      jmouse_set_cursor(JI_CURSOR_NULL);
      jmouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);

      /* render the sprite in the bitmap */
      image = RenderEngine::renderSprite(sprite, 0, 0, sprite->w, sprite->h,
					 sprite->frame, 0);
      if (image) {
	image_to_allegro(image, bmp, 0, 0);
	image_free(image);
      }

      if (!(flags & PREVIEW_TILED))
	bg_color = palette_color[index_bg_color=0];
      else
	bg_color = makecol(128, 128, 128);

      shiftx = - scroll_x + vp->x1 + editor->editor_get_offset_x();
      shifty = - scroll_y + vp->y1 + editor->editor_get_offset_y();

      w = sprite->w << editor->editor_get_zoom();
      h = sprite->h << editor->editor_get_zoom();

      redraw = true;
      do {
	/* update scroll */
	if (jmouse_poll()) {
	  shiftx += jmouse_x(0) - JI_SCREEN_W/2;
	  shifty += jmouse_y(0) - JI_SCREEN_H/2;
	  jmouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);
	  jmouse_poll();

	  redraw = true;
	}

	if (redraw) {
	  redraw = false;

	  /* fit on screen */
	  if (flags & PREVIEW_FIT_ON_SCREEN) {
	    double sx, sy, scale, outw, outh;

	    sx = (double)JI_SCREEN_W / (double)bmp->w;
	    sy = (double)JI_SCREEN_H / (double)bmp->h;
	    scale = MIN(sx, sy);

	    outw = (double)bmp->w * (double)scale;
	    outh = (double)bmp->h * (double)scale;

	    stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, 0, 0, outw, outh);
	    jrectexclude(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1,
			 0, 0, outw-1, outh-1, bg_color);
	  }
	  /* draw in normal size */
	  else {
	    if (tiled & TILED_X_AXIS)
	      x = SGN(shiftx) * (ABS(shiftx)%w);
	    else
	      x = shiftx;

	    if (tiled & TILED_Y_AXIS)
	      y = SGN(shifty) * (ABS(shifty)%h);
	    else
	      y = shifty;

	    if (tiled != TILED_BOTH) {
/* 	      rectfill_exclude(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1, */
/* 			       x, y, x+w-1, y+h-1, bg_color); */
	      clear_to_color(ji_screen, bg_color);
	    }

	    if (!editor->editor_get_zoom()) {
	      /* in the center */
	      if (!(flags & PREVIEW_TILED))
		draw_sprite(ji_screen, bmp, x, y);
	      /* tiled */
	      else {
		switch (tiled) {
		  case TILED_X_AXIS:
		    for (u=x-w; u<JI_SCREEN_W+w; u+=w)
		      blit(bmp, ji_screen, 0, 0, u, y, w, h);
		    break;
		  case TILED_Y_AXIS:
		    for (v=y-h; v<JI_SCREEN_H+h; v+=h)
		      blit(bmp, ji_screen, 0, 0, x, v, w, h);
		    break;
		  case TILED_BOTH:
		    for (v=y-h; v<JI_SCREEN_H+h; v+=h)
		      for (u=x-w; u<JI_SCREEN_W+w; u+=w)
			blit(bmp, ji_screen, 0, 0, u, v, w, h);
		    break;
		  case TILED_NONE:
		    assert(false);
		    break;
		}
	      }
	    }
	    else {
	      /* in the center */
	      if (!(flags & PREVIEW_TILED))
		masked_stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, x, y, w, h);
	      /* tiled */
	      else {
		switch (tiled) {
		  case TILED_X_AXIS:
		    for (u=x-w; u<JI_SCREEN_W+w; u+=w)
		      stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, u, y, w, h);
		    break;
		  case TILED_Y_AXIS:
		    for (v=y-h; v<JI_SCREEN_H+h; v+=h)
		      stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, x, v, w, h);
		    break;
		  case TILED_BOTH:
		    for (v=y-h; v<JI_SCREEN_H+h; v+=h)
		      for (u=x-w; u<JI_SCREEN_W+w; u+=w)
			stretch_blit(bmp, ji_screen, 0, 0, bmp->w, bmp->h, u, v, w, h);
		    break;
		  case TILED_NONE:
		    assert(false);
		    break;
		}
	      }
	    }
	  }
	}

	gui_feedback();

	if (keypressed()) {
	  int readkey_value = readkey();
	  JMessage msg = jmessage_new_key_related(JM_KEYPRESSED, readkey_value);
	  Command* command = get_command_from_key_message(msg);
	  jmessage_free(msg);

	  /* change frame */
	  if (command != NULL &&
	      (strcmp(command->short_name(), CommandId::goto_first_frame) == 0 ||
	       strcmp(command->short_name(), CommandId::goto_previous_frame) == 0 ||
	       strcmp(command->short_name(), CommandId::goto_next_frame) == 0 ||
	       strcmp(command->short_name(), CommandId::goto_last_frame) == 0)) {
	    /* execute the command */
	    context->execute_command(command);

	    /* redraw */
	    redraw = true;

	    /* render the sprite in the bitmap */
	    image = RenderEngine::renderSprite(sprite, 0, 0, sprite->w, sprite->h,
					       sprite->frame, 0);
	    if (image) {
	      image_to_allegro(image, bmp, 0, 0);
	      image_free(image);
	    }
	  }
	  /* play the animation */
	  else if (command != NULL &&
		   strcmp(command->short_name(), CommandId::play_animation) == 0) {
	    /* TODO */
	  }
	  /* change background color */
	  else if ((readkey_value>>8) == KEY_PLUS_PAD) {
	    if (index_bg_color < 255) {
	      bg_color = palette_color[++index_bg_color];
	      redraw = true;
	    }
	  }
	  else if ((readkey_value>>8) == KEY_MINUS_PAD) {
	    if (index_bg_color > 0) {
	      bg_color = palette_color[--index_bg_color];
	      redraw = true;
	    }
	  }
	  else
	    break;
	}
      } while (!jmouse_b(0));

      destroy_bitmap(bmp);
    }

    do {
      jmouse_poll();
      gui_feedback();
    } while (jmouse_b(0));
    clear_keybuf();

    jmouse_set_position(old_mouse_x, old_mouse_y);
    jmouse_set_cursor(JI_CURSOR_NORMAL);

    jmanager_refresh_screen();
    jrect_free(vp);
  }
}

//////////////////////////////////////////////////////////////////////
// preview_fit_to_screen

class PreviewFitToScreenCommand : public PreviewCommand
{
public:
  PreviewFitToScreenCommand();
  Command* clone() { return new PreviewFitToScreenCommand(*this); }

protected:
  void execute(Context* context);
};

PreviewFitToScreenCommand::PreviewFitToScreenCommand()
  : PreviewCommand("preview_fit_to_screen",
		   "Preview Fit to Screen")
{
}

void PreviewFitToScreenCommand::execute(Context* context)
{
  preview_sprite(context, PREVIEW_FIT_ON_SCREEN);
}

//////////////////////////////////////////////////////////////////////
// preview_normal

class PreviewNormalCommand : public PreviewCommand
{
public:
  PreviewNormalCommand();
  Command* clone() { return new PreviewNormalCommand(*this); }

protected:
  void execute(Context* context);
};

PreviewNormalCommand::PreviewNormalCommand()
  : PreviewCommand("preview_normal",
		   "Preview Normal")
{
}

void PreviewNormalCommand::execute(Context* context)
{
  preview_sprite(context, 0);
}

//////////////////////////////////////////////////////////////////////
// preview_tiled

class PreviewTiledCommand : public PreviewCommand
{
public:
  PreviewTiledCommand();
  Command* clone() { return new PreviewTiledCommand(*this); }

protected:
  void execute(Context* context);
};

PreviewTiledCommand::PreviewTiledCommand()
  : PreviewCommand("preview_tiled",
		   "Preview Tiled")
{
}

void PreviewTiledCommand::execute(Context* context)
{
  preview_sprite(context, PREVIEW_TILED);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_preview_fit_to_screen_command()
{
  return new PreviewFitToScreenCommand;
}

Command* CommandFactory::create_preview_normal_command()
{
  return new PreviewNormalCommand;
}

Command* CommandFactory::create_preview_tiled_command()
{
  return new PreviewTiledCommand;
}
