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
#include <math.h>

#include "jinete/jinete.h"

/* GUI */
void init_gui();
void shutdown_gui();
bool update_gui();
void draw_gui();
float get_speed();
float get_reaction_pos();
bool get_back_to_center();

/* Game */
void play_game();

/**********************************************************************/
/* Main routine */

int main(int argc, char *argv[])
{
  /* Allegro stuff */
  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  rgb_map = (RGB_MAP*)jmalloc(sizeof(RGB_MAP));
  color_map = (COLOR_MAP*)jmalloc(sizeof(COLOR_MAP));

  create_rgb_table(rgb_map, default_palette, NULL);
  create_trans_table(color_map, default_palette, 128, 128, 128, NULL);

  init_gui();
  play_game();
  shutdown_gui();

  jfree(rgb_map);
  jfree(color_map);
  return 0;
}

END_OF_MAIN();

/**********************************************************************/
/* GUI handle */

static JWidget window, manager, entry, check;

static bool my_manager_hook(JWidget widget, JMessage msg);

void init_gui()
{
  JWidget box1, box2, label, button;

  /* Jinete initialization */
  manager = jmanager_new();
  ji_set_standard_theme();

  /* hook manager behavior */
  jwidget_add_hook(manager, JI_WIDGET, my_manager_hook, NULL);

  /* create main window */
  window = jwindow_new("Game Manager");
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  label = jlabel_new("Speed:");
  entry = jentry_new(8, "4");
  check = jcheck_new("AI/Center after hit");
  button = jbutton_new("&Game Over");

  jwidget_expansive(box2, true);
  jwidget_expansive(entry, true);

  jwidget_add_child(window, box1);
  jwidget_add_children(box1, box2, check, button, NULL);
  jwidget_add_children(box2, label, entry, NULL);

  jwindow_open(window);
}

void shutdown_gui()
{
  jwidget_free(window);
  jmanager_free(manager);
  manager = NULL;
}

bool update_gui()
{
  if (jmanager_generate_messages(manager))
    jmanager_dispatch_messages(manager);

  return window->isVisible();
}

void draw_gui()
{
  jwidget_dirty(manager);
  jwidget_flush_redraw(manager);
  //jmanager_dispatch_draw_messages();
}

float get_speed()
{
  float speed = strtod(jwidget_get_text(entry), NULL);
  return speed;
}

float get_reaction_pos()
{
  return window->rc->x1-2;
}

bool get_back_to_center()
{
  return jwidget_is_selected(check);
}

static bool my_manager_hook(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_KEYPRESSED:
      /* don't use UP & DOWN keys for focus movement */
      if (msg->key.scancode == KEY_UP || msg->key.scancode == KEY_DOWN) {
	/* returning true means that we use that keys (we don't use
	   that keys here, but the game use them) */
	return true;
      }
      break;

    case JM_DRAW:
      /* draw nothing (this avoid the default behavior: paint with the
	 desktop color of the current manager's theme) */
      return true;
  }

  return false;
}

/**********************************************************************/
/* Game handle */

#define BS 8			/* ball size */
#define PS 40			/* player size */

static float ball_x, ball_y;
static float ball_dx, ball_dy;
static int p_y1, p_y2;

static int count = 0;
static void count_inc()
{
  count++;
}

END_OF_STATIC_FUNCTION(count_inc);

static void update_game();
static void draw_game(BITMAP *bmp);
static bool move_ball(float *x, float *y, float *dx, float *dy);
static void calc_ball_dest(float *y);

void play_game()
{
  bool gameover = false;
  bool trans_mode = false;
  BITMAP *bmp, *bmp2;

  /* create a temporary bitmap to make double-buffered technique */
  bmp = create_bitmap(SCREEN_W, SCREEN_H);
  bmp2 = create_bitmap(SCREEN_W, SCREEN_H);

  /* set Jinete output screen bitmap */
  ji_set_screen(bmp, bmp->w, bmp->h);
  show_mouse(NULL);

  /* init game state */
  ball_x = SCREEN_W / 2;
  ball_y = SCREEN_H / 2;
  ball_dx = cos(AL_PI/4);
  ball_dy = sin(AL_PI/4);
  p_y1 = p_y2 = SCREEN_H / 2 - PS / 2;

  LOCK_VARIABLE(count);
  LOCK_FUNCTION(count_inc);
  install_int_ex(count_inc, BPS_TO_TIMER(60));

  while (!gameover) {
    while (count > 0 && !gameover) {
      update_game();
      gameover = update_gui();
      count--;
    }

    /* use transparent bitmap */
    if (jwidget_pick(manager, mouse_x, mouse_y) == manager) {
      if (!trans_mode) {
	trans_mode = true;
	ji_set_screen(bmp2, bmp2->w, bmp2->h);
	show_mouse(NULL);
      }
    }
    else if (trans_mode) {
      trans_mode = false;
      ji_set_screen(bmp);
      show_mouse(NULL);
    }

    /* draw game state */
    draw_game(bmp);

    /* draw GUI */
    if (trans_mode)
      clear(bmp2);
    draw_gui();

    /* blit to screen (with the mouse on it) */
    show_mouse(ji_screen);
    freeze_mouse_flag = true;
    if (trans_mode)
      draw_trans_sprite (bmp, bmp2, 0, 0);
    blit(bmp, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
    freeze_mouse_flag = false;
    show_mouse(NULL);
  }

  ji_set_screen(screen, SCREEN_W, SCREEN_H);
  destroy_bitmap(bmp);
  destroy_bitmap(bmp2);
}

static void update_game()
{
  float y, angle;

  /* move ball */
  if (!move_ball(&ball_x, &ball_y, &ball_dx, &ball_dy)) {
    /* some player loses */
    ball_x = SCREEN_W / 2;
    ball_y = SCREEN_H / 2;
    ball_dx = cos(AL_PI/4);
    ball_dy = sin(AL_PI/4);
  }

  /* collision ball<->player */
  if (ball_dx < 0) {
    if (ball_x >= 2 && ball_y+BS/2 >= p_y1-PS/2 &&
	ball_x <= 12 && ball_y-BS/2 <= p_y1-PS/2+PS-1) {
      angle = -AL_PI/4 + AL_PI/2 * (ball_y-(p_y1-PS/2))/PS;
      ball_dx = cos(angle);
      ball_dy = sin(angle);
    }
  }
  else if (ball_dx > 0) {
    if (ball_x >= SCREEN_W-1-12 && ball_y+BS/2 >= p_y2-PS/2 &&
	ball_x <= SCREEN_W-1-2 && ball_y-BS/2 <= p_y2-PS/2+PS-1) {
      angle = -AL_PI*3/4 - AL_PI/2 * (ball_y-(p_y2-PS/2))/PS;
      ball_dx = cos(angle);
      ball_dy = sin(angle);
    }
  }

  /* player */
  if (key[KEY_UP])
    p_y1 -= 4;
  if (key[KEY_DOWN])
    p_y1 += 4;

  /* IA */
  if (ball_dx > 0 && ball_x > get_reaction_pos())
    calc_ball_dest(&y);
  else if (get_back_to_center())
    y = SCREEN_H/2;
  else
    y = p_y2;

  if (p_y2 > y) {
    p_y2 -= 4;
    if (p_y2 < y)
      p_y2 = y;
  }
  else if (p_y2 < y) {
    p_y2 += 4;
    if (p_y2 > y)
      p_y2 = y;
  }

  /* limit players movement */
  p_y1 = MID(PS/2, p_y1, SCREEN_H-1-PS/2);
  p_y2 = MID(PS/2, p_y2, SCREEN_H-1-PS/2);
}

static void draw_game(BITMAP *bmp)
{
  float x = get_reaction_pos();

  /* clear screen */
  clear(bmp);

  /* reaction position */
  vline(bmp, x, 0, SCREEN_H-1, makecol(255, 255, 255));
  textout(bmp, font, "AI reaction position", x+2, 0, makecol(255, 255, 255));

  /* draw ball */
  rectfill(bmp,
	   ball_x-BS/2, ball_y-BS/2,
	   ball_x-BS/2+BS-1, ball_y-BS/2+BS-1, makecol(255, 255, 255));

  /* draw left & right players */
  rectfill(bmp,
	   2, p_y1-PS/2,
	   12, p_y1-PS/2+PS-1, makecol(255, 255, 255));
  rectfill(bmp,
	   SCREEN_W-1-12, p_y2-PS/2,
	   SCREEN_W-1-2, p_y2-PS/2+PS-1, makecol(255, 255, 255));
}

static bool move_ball(float *x, float *y, float *dx, float *dy)
{
  float speed = get_speed();

  /* ball movement */
  *x += (*dx) * speed;
  *y += (*dy) * speed;

  /* bouncing with walls */
  if (*dy < 0 && (*y)-BS/2 < 0)
    *dy = -(*dy);
  else if (*dy > 0 && (*y)+BS/2 > SCREEN_H-1)
    *dy = -(*dy);

  /* ball goes out of screen */
  if (*x < 0 || *x > SCREEN_W-1)
    return false;
  else
    return true;
}

static void calc_ball_dest(float *y)
{
  float speed = get_speed();
  float x = ball_x;
  float dx = ball_dx;
  float dy = ball_dy;

  *y = ball_y;

  if (speed >= 1)
    while (move_ball(&x, y, &dx, &dy));
}

