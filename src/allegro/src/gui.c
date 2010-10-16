/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      The core GUI routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Peter Pavlovic modified the drawing and positioning of menus.
 *
 *      Menu auto-opening added by Angelo Mottola.
 *
 *      Eric Botcazou added the support for non-blocking menus.
 *
 *      Elias Pschernig and Sven Sandberg improved the focus algorithm.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* if set, the input focus follows the mouse pointer */
int gui_mouse_focus = TRUE;


/* font alignment value */
int gui_font_baseline = 0;


/* Pointers to the currently active dialog and menu objects.
 *
 * Note: active_dialog_player always points to the currently active dialog
 * player. However, active_menu_player only ever points to menu players
 * started by a d_menu_proc. The code also assumes that that d_menu_proc can
 * be found in the currently active dialog.
 *
 * Note: active_dialog points to the whole dialog currently running. However,
 * active_menu points to the *current item* of the currently running menu,
 * and should really have been called active_menu_item.
 */
static DIALOG_PLAYER *active_dialog_player = NULL;
static MENU_PLAYER *active_menu_player = NULL;
static int active_menu_player_zombie = FALSE;
DIALOG *active_dialog = NULL;
MENU *active_menu = NULL;


static BITMAP *gui_screen = NULL;


/* list of currently active (initialized) dialog players */
struct al_active_dialog_player {
   DIALOG_PLAYER *player;
   struct al_active_dialog_player *next;
};

static struct al_active_dialog_player *first_active_dialog_player = 0;
static struct al_active_dialog_player *current_active_dialog_player = 0;


/* forward declarations */
static int shutdown_single_menu(MENU_PLAYER *, int *);



/* hook function for reading the mouse x position */
static int default_mouse_x(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_x;
}

END_OF_STATIC_FUNCTION(default_mouse_x);



/* hook function for reading the mouse y position */
static int default_mouse_y(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_y;
}

END_OF_STATIC_FUNCTION(default_mouse_y);



/* hook function for reading the mouse z position */
static int default_mouse_z(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_z;
}

END_OF_STATIC_FUNCTION(default_mouse_z);



/* hook function for reading the mouse button state */
static int default_mouse_b(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_b;
}

END_OF_STATIC_FUNCTION(default_mouse_b);



/* hook functions for reading the mouse state */
int (*gui_mouse_x)(void) = default_mouse_x;
int (*gui_mouse_y)(void) = default_mouse_y;
int (*gui_mouse_z)(void) = default_mouse_z;
int (*gui_mouse_b)(void) = default_mouse_b;


/* timer to handle menu auto-opening */
static int gui_timer;

static int gui_menu_opening_delay;


/* Checking for double clicks is complicated. The user could release the
 * mouse button at almost any point, and I might miss it if I am doing some
 * other processing at the same time (eg. sending the single-click message).
 * To get around this I install a timer routine to do the checking for me,
 * so it will notice double clicks whenever they happen.
 */

static volatile int dclick_status, dclick_time;

static int gui_install_count = 0;
static int gui_install_time = 0;

#define DCLICK_START      0
#define DCLICK_RELEASE    1
#define DCLICK_AGAIN      2
#define DCLICK_NOT        3



/* dclick_check:
 *  Double click checking user timer routine.
 */
static void dclick_check(void)
{
   gui_timer++;

   if (dclick_status==DCLICK_START) {              /* first click... */
      if (!gui_mouse_b()) {
	 dclick_status = DCLICK_RELEASE;           /* aah! released first */
	 dclick_time = 0;
	 return;
      }
   }
   else if (dclick_status==DCLICK_RELEASE) {       /* wait for second click */
      if (gui_mouse_b()) {
	 dclick_status = DCLICK_AGAIN;             /* yes! the second click */
	 dclick_time = 0;
	 return;
      }
   }
   else
      return;

   /* timeout? */
   if (dclick_time++ > 10)
      dclick_status = DCLICK_NOT;
}

END_OF_STATIC_FUNCTION(dclick_check);



/* gui_switch_callback:
 *  Sets the dirty flags whenever a display switch occurs.
 */
static void gui_switch_callback(void)
{
   if (active_dialog_player)
      active_dialog_player->res |= D_REDRAW_ALL;
}



/* position_dialog:
 *  Moves all the objects in a dialog to the specified position.
 */
void position_dialog(DIALOG *dialog, int x, int y)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int xc, yc;
   int c;
   ASSERT(dialog);

   /* locate the upper-left corner */
   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < min_x)
	 min_x = dialog[c].x;

      if (dialog[c].y < min_y)
	 min_y = dialog[c].y;
   }

   /* move the dialog */
   xc = min_x - x;
   yc = min_y - y;

   for (c=0; dialog[c].proc; c++) {
      dialog[c].x -= xc;
      dialog[c].y -= yc;
   }
}



/* centre_dialog:
 *  Moves all the objects in a dialog so that the dialog is centered in
 *  the screen.
 */
void centre_dialog(DIALOG *dialog)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int max_x = INT_MIN;
   int max_y = INT_MIN;
   int xc, yc;
   int c;
   ASSERT(dialog);

   /* find the extents of the dialog (done in many loops due to a bug
    * in MSVC that prevents the more sensible version from working)
    */
   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < min_x)
	 min_x = dialog[c].x;
   }

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].y < min_y)
	 min_y = dialog[c].y;
   }

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x + dialog[c].w > max_x)
	 max_x = dialog[c].x + dialog[c].w;
   }

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].y + dialog[c].h > max_y)
	 max_y = dialog[c].y + dialog[c].h;
   }

   /* how much to move by? */
   xc = (SCREEN_W - (max_x - min_x)) / 2 - min_x;
   yc = (SCREEN_H - (max_y - min_y)) / 2 - min_y;

   /* move it */
   for (c=0; dialog[c].proc; c++) {
      dialog[c].x += xc;
      dialog[c].y += yc;
   }
}



/* set_dialog_color:
 *  Sets the foreground and background colors of all the objects in a dialog.
 */
void set_dialog_color(DIALOG *dialog, int fg, int bg)
{
   int c;
   ASSERT(dialog);

   for (c=0; dialog[c].proc; c++) {
      dialog[c].fg = fg;
      dialog[c].bg = bg;
   }
}



/* find_dialog_focus:
 *  Searches the dialog for the object which has the input focus, returning
 *  its index, or -1 if the focus is not set. Useful after do_dialog() exits
 *  if you need to know which object was selected.
 */
int find_dialog_focus(DIALOG *dialog)
{
   int c;
   ASSERT(dialog);

   for (c=0; dialog[c].proc; c++)
      if (dialog[c].flags & D_GOTFOCUS)
	 return c;

   return -1;
}



/* object_message:
 *  Sends a message to a widget, automatically scaring and unscaring
 *  the mouse if the message is MSG_DRAW.
 */
int object_message(DIALOG *dialog, int msg, int c)
{
#ifdef ALLEGRO_WINDOWS
   /* exported address of d_clear_proc */
   extern int (*_d_clear_proc)(int, DIALOG *, int);
#endif

   int ret;
   ASSERT(dialog);

   if (msg == MSG_DRAW) {
      if (dialog->flags & D_HIDDEN)
	 return D_O_K;

#ifdef ALLEGRO_WINDOWS
      if (dialog->proc == _d_clear_proc)
#else
      if (dialog->proc == d_clear_proc)
#endif
	 scare_mouse();
      else
	 scare_mouse_area(dialog->x, dialog->y, dialog->w, dialog->h);

      acquire_screen();
   }

   ret = dialog->proc(msg, dialog, c);

   if (msg == MSG_DRAW) {
      release_screen();
      unscare_mouse();
   }

   if (ret & D_REDRAWME) {
      dialog->flags |= D_DIRTY;
      ret &= ~D_REDRAWME;
   }

   return ret;
}



/* dialog_message:
 *  Sends a message to all the objects in a dialog. If any of the objects
 *  return values other than D_O_K, returns the value and sets obj to the
 *  object which produced it.
 */
int dialog_message(DIALOG *dialog, int msg, int c, int *obj)
{
   int count, res, r, force, try;
   DIALOG *menu_dialog = NULL;
   ASSERT(dialog);

   /* Note: don't acquire the screen in here.  A deadlock develops in a
    * situation like this:
    *
    * 1. this thread: acquires the screen;
    * 2. timer thread: wakes up, locks the timer_mutex, and calls a
    *    callback to redraw the software mouse cursor, which tries to
    *    acquire the screen;
    * 3. this thread: object_message(MSG_DRAW) calls scare_mouse()
    *    which calls remove_int().
    *
    * So this thread has the screen acquired and wants the timer_mutex,
    * whereas the timer thread holds the timer_mutex, but wants to acquire
    * the screen.  The problem is calling scare_mouse() with the screen
    * acquired.
    */

   force = ((msg == MSG_START) || (msg == MSG_END) || (msg >= MSG_USER));

   res = D_O_K;

   /* If a menu spawned by a d_menu_proc object is active, the dialog engine
    * has effectively been shutdown for the sake of safety. This means that
    * we can't send the message to the other objects in the dialog. So try
    * first to send the message to the d_menu_proc object and, if the menu
    * is then not active anymore, send it to the other objects as well.
    */
   if (active_menu_player) {
      try = 2;
      menu_dialog = active_menu_player->dialog;
   }
   else
      try = 1;

   for (; try > 0; try--) {
      for (count=0; dialog[count].proc; count++) {
         if ((try == 2) && (&dialog[count] != menu_dialog))
	    continue;

	 if ((force) || (!(dialog[count].flags & D_HIDDEN))) {
	    r = object_message(&dialog[count], msg, c);

	    if (r != D_O_K) {
	       res |= r;
	       if (obj)
		  *obj = count;
	    }

	    if ((msg == MSG_IDLE) && (dialog[count].flags & (D_DIRTY | D_HIDDEN)) == D_DIRTY) {
	       dialog[count].flags &= ~D_DIRTY;
	       object_message(dialog+count, MSG_DRAW, 0);
	    }
	 }
      }

      if (active_menu_player)
	 break;
   }

   return res;
}



/* broadcast_dialog_message:
 *  Broadcasts a message to all the objects in the active dialog. If any of
 *  the dialog procedures return values other than D_O_K, it returns that
 *  value.
 */
int broadcast_dialog_message(int msg, int c)
{
   int nowhere;

   if (active_dialog)
      return dialog_message(active_dialog, msg, c, &nowhere);
   else
      return D_O_K;
}



/* find_mouse_object:
 *  Finds which object the mouse is on top of.
 */
static int find_mouse_object(DIALOG *d)
{
   int mouse_object = -1;
   int c;
   int res;
   ASSERT(d);

   for (c=0; d[c].proc; c++) {
      if ((gui_mouse_x() >= d[c].x) && (gui_mouse_y() >= d[c].y) &&
         (gui_mouse_x() < d[c].x + d[c].w) && (gui_mouse_y() < d[c].y + d[c].h) &&
         (!(d[c].flags & (D_HIDDEN | D_DISABLED)))) {
         /* check if this object wants the mouse */
         res = object_message(d+c, MSG_WANTMOUSE, 0);
         if (!(res & D_DONTWANTMOUSE)) {
            mouse_object = c;
         }
      }
   }

   return mouse_object;
}



/* offer_focus:
 *  Offers the input focus to a particular object.
 */
int offer_focus(DIALOG *dialog, int obj, int *focus_obj, int force)
{
   int res = D_O_K;
   ASSERT(dialog);
   ASSERT(focus_obj);

   if ((obj == *focus_obj) ||
       ((obj >= 0) && (dialog[obj].flags & (D_HIDDEN | D_DISABLED))))
      return D_O_K;

   /* check if object wants the focus */
   if (obj >= 0) {
      res = object_message(dialog+obj, MSG_WANTFOCUS, 0);
      if (res & D_WANTFOCUS)
	 res ^= D_WANTFOCUS;
      else
	 obj = -1;
   }

   if ((obj >= 0) || (force)) {
      /* take focus away from old object */
      if (*focus_obj >= 0) {
	 res |= object_message(dialog+*focus_obj, MSG_LOSTFOCUS, 0);
	 if (res & D_WANTFOCUS) {
	    if (obj < 0)
	       return D_O_K;
	    else
	       res &= ~D_WANTFOCUS;
	 }
	 dialog[*focus_obj].flags &= ~D_GOTFOCUS;
	 res |= object_message(dialog+*focus_obj, MSG_DRAW, 0);
      }

      *focus_obj = obj;

      /* give focus to new object */
      if (obj >= 0) {
	 dialog[obj].flags |= D_GOTFOCUS;
	 res |= object_message(dialog+obj, MSG_GOTFOCUS, 0);
	 res |= object_message(dialog+obj, MSG_DRAW, 0);
      }
   }

   return res;
}



#define MAX_OBJECTS     512

typedef struct OBJ_LIST
{
   int index;
   int diff;
} OBJ_LIST;


/* Weight ratio between the orthogonal direction and the main direction
   when calculating the distance for the focus algorithm. */
#define DISTANCE_RATIO  8

/* Maximum size (in bytes) of a dialog array. */
#define MAX_SIZE  0x10000  /* 64 kb */

enum axis { X_AXIS, Y_AXIS };



/* obj_list_cmp:
 *  Callback function for qsort().
 */
static int obj_list_cmp(AL_CONST void *e1, AL_CONST void *e2)
{
   return (((OBJ_LIST *)e1)->diff - ((OBJ_LIST *)e2)->diff);
}



/* cmp_tab:
 *  Comparison function for tab key movement.
 */
static int cmp_tab(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (int)((AL_CONST unsigned long)d2 - (AL_CONST unsigned long)d1);

   /* Wrap around if d2 is before d1 in the dialog array. */
   if (ret < 0)
      ret += MAX_SIZE;

   return ret;
}



/* cmp_shift_tab:
 *  Comparison function for shift+tab key movement.
 */
static int cmp_shift_tab(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (int)((AL_CONST unsigned long)d1 - (AL_CONST unsigned long)d2);

   /* Wrap around if d2 is after d1 in the dialog array. */
   if (ret < 0)
      ret += MAX_SIZE;

   return ret;
}



/* min_dist:
 *  Returns the minimum distance between dialogs 'd1' and 'd2'. 'main_axis'
 *  is taken account to give different weights to the axes in the distance
 *  formula, as well as to shift the actual position of 'd2' along the axis
 *  by the amount specified by 'bias'.
 */
static int min_dist(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2, enum axis main_axis, int bias)
{
   int x_left = d1->x - d2->x - d2->w + 1;
   int x_right = d2->x - d1->x - d1->w + 1;
   int y_top = d1->y - d2->y - d2->h + 1;
   int y_bottom = d2->y - d1->y - d1->h + 1;

   if (main_axis == X_AXIS) {
      x_left -= bias;
      x_right += bias;
      y_top *= DISTANCE_RATIO;
      y_bottom *= DISTANCE_RATIO;
   }
   else {
      x_left *= DISTANCE_RATIO;
      x_right *= DISTANCE_RATIO;
      y_top -= bias;
      y_bottom += bias;
   }

   if (x_left > 0) { /* d2 is left of d1 */
      if (y_top > 0)  /* d2 is above d1 */
         return x_left + y_top;
      else if (y_bottom > 0)  /* d2 is below d1 */
         return x_left + y_bottom;
      else  /* vertically overlapping */
         return x_left;
   }
   else if (x_right > 0) { /* d2 is right of d1 */
      if (y_top > 0)  /* d2 is above d1 */
         return x_right + y_top;
      else if (y_bottom > 0)  /* d2 is below d1 */
         return x_right + y_bottom;
      else  /* vertically overlapping */
         return x_right;
   }
   /* horizontally overlapping */
   else if (y_top > 0)  /* d2 is above d1 */
      return y_top;
   else if (y_bottom > 0)  /* d2 is below d1 */
      return y_bottom;
   else  /* overlapping */
      return 0;
}



/* cmp_right:
 *  Comparison function for right arrow key movement.
 */
static int cmp_right(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int bias;

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's right edge and not containing it. */
   if (d2->x < d1->x + d1->w)
      bias = +SCREEN_W;
   else
      bias = 0;

   return min_dist(d1, d2, X_AXIS, bias);
}



/* cmp_left:
 *  Comparison function for left arrow key movement.
 */
static int cmp_left(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int bias;

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's left edge and not containing it. */
   if (d2->x + d2->w > d1->x)
      bias = -SCREEN_W;
   else
      bias = 0;

   return min_dist(d1, d2, X_AXIS, bias);
}



/* cmp_down:
 *  Comparison function for down arrow key movement.
 */
static int cmp_down(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int bias;

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's bottom edge and not containing it. */
   if (d2->y < d1->y + d1->h)
      bias = +SCREEN_H;
   else
      bias = 0;

   return min_dist(d1, d2, Y_AXIS, bias);
}



/* cmp_up:
 *  Comparison function for up arrow key movement.
 */
static int cmp_up(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int bias;

   /* Wrap around if d2 is not fully contained in the half-plan
      delimited by d1's top edge and not containing it. */
   if (d2->y + d2->h > d1->y)
      bias = -SCREEN_H;
   else
      bias = 0;

   return min_dist(d1, d2, Y_AXIS, bias);
}



/* move_focus:
 *  Handles arrow key and tab movement through a dialog, deciding which
 *  object should be given the input focus.
 */
static int move_focus(DIALOG *d, int ascii, int scan, int *focus_obj)
{
   int (*cmp)(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2);
   OBJ_LIST obj[MAX_OBJECTS];
   int obj_count = 0;
   int fobj, c;
   int res = D_O_K;

   /* choose a comparison function */
   switch (scan) {
      case KEY_TAB:   cmp = (ascii == '\t') ? cmp_tab : cmp_shift_tab; break;
      case KEY_RIGHT: cmp = cmp_right; break;
      case KEY_LEFT:  cmp = cmp_left;  break;
      case KEY_DOWN:  cmp = cmp_down;  break;
      case KEY_UP:    cmp = cmp_up;    break;
      default:        return D_O_K;
   }

   /* fill temporary table */
   for (c=0; d[c].proc; c++) {
      if (((*focus_obj < 0) || (c != *focus_obj))
	  && !(d[c].flags & (D_DISABLED | D_HIDDEN))) {
	 obj[obj_count].index = c;
	 if (*focus_obj >= 0)
	    obj[obj_count].diff = cmp(d+*focus_obj, d+c);
	 else
	    obj[obj_count].diff = c;
	 obj_count++;
	 if (obj_count >= MAX_OBJECTS)
	    break;
      }
   }

   /* sort table */
   qsort(obj, obj_count, sizeof(OBJ_LIST), obj_list_cmp);

   /* find an object to give the focus to */
   fobj = *focus_obj;
   for (c=0; c<obj_count; c++) {
      res |= offer_focus(d, obj[c].index, focus_obj, FALSE);
      if (fobj != *focus_obj)
	 break;
   }

   return res;
}



#define MESSAGE(i, msg, c) {                       \
   r = object_message(player->dialog+i, msg, c);   \
   if (r != D_O_K) {                               \
      player->res |= r;                            \
      player->obj = i;                             \
   }                                               \
}



/* do_dialog:
 *  The basic dialog manager. The list of dialog objects should be
 *  terminated by one with a null dialog procedure. Returns the index of
 *  the object which caused it to exit.
 */
int do_dialog(DIALOG *dialog, int focus_obj)
{
   BITMAP *mouse_screen = _mouse_screen;
   BITMAP *gui_bmp = gui_get_screen();
   int screen_count = _gfx_mode_set_count;
   void *player;
   ASSERT(dialog);

   if (!is_same_bitmap(_mouse_screen, gui_bmp) && !(gfx_capabilities&GFX_HW_CURSOR))
      show_mouse(gui_bmp);

   player = init_dialog(dialog, focus_obj);

   while (update_dialog(player)) {
      /* If a menu is active, we yield here, since the dialog
       * engine is shut down so no user code can be running.
       */
      if (active_menu_player)
         rest(1);
   }

   if (_gfx_mode_set_count == screen_count && !(gfx_capabilities&GFX_HW_CURSOR))
      show_mouse(mouse_screen);

   return shutdown_dialog(player);
}



/* popup_dialog:
 *  Like do_dialog(), but it stores the data on the screen before drawing
 *  the dialog and restores it when the dialog is closed. The screen area
 *  to be stored is calculated from the dimensions of the first object in
 *  the dialog, so all the other objects should lie within this one.
 */
int popup_dialog(DIALOG *dialog, int focus_obj)
{
   BITMAP *bmp;
   BITMAP *gui_bmp;
   int ret;
   ASSERT(dialog);

   bmp = create_bitmap(dialog->w, dialog->h);
   gui_bmp = gui_get_screen();
   if (bmp) {
      scare_mouse_area(dialog->x, dialog->y, dialog->w, dialog->h);
      blit(gui_bmp, bmp, dialog->x, dialog->y, 0, 0, dialog->w, dialog->h);
      unscare_mouse();
   }
   else
      *allegro_errno = ENOMEM;

   ret = do_dialog(dialog, focus_obj);

   if (bmp) {
      scare_mouse_area(dialog->x, dialog->y, dialog->w, dialog->h);
      blit(bmp, gui_bmp, 0, 0, dialog->x, dialog->y, dialog->w, dialog->h);
      unscare_mouse();
      destroy_bitmap(bmp);
   }

   return ret;
}



/* init_dialog:
 *  Sets up a dialog, returning a player object that can be used with
 *  the update_dialog() and shutdown_dialog() functions.
 */
DIALOG_PLAYER *init_dialog(DIALOG *dialog, int focus_obj)
{
   DIALOG_PLAYER *player;
   BITMAP *gui_bmp = gui_get_screen();
   struct al_active_dialog_player *n;
   char tmp1[64], tmp2[64];
   int c;
   ASSERT(dialog);

   /* close any menu opened by a d_menu_proc */
   if (active_menu_player)
      object_message(active_menu_player->dialog, MSG_LOSTMOUSE, 0);

   player = _AL_MALLOC(sizeof(DIALOG_PLAYER));
   if (!player) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   /* append player to the list */
   n = _AL_MALLOC(sizeof(struct al_active_dialog_player));
   if (!n) {
      *allegro_errno = ENOMEM;
      _AL_FREE (player);
      return NULL;
   }

   n->next = NULL;
   n->player = player;

   if (!current_active_dialog_player) {
      current_active_dialog_player = first_active_dialog_player = n;
   }
   else {
      current_active_dialog_player->next = n;
      current_active_dialog_player = n;
   }

   player->res = D_REDRAW;
   player->joy_on = TRUE;
   player->click_wait = FALSE;
   player->dialog = dialog;
   player->obj = -1;
   player->mouse_obj = -1;
   player->mouse_oz = gui_mouse_z();
   player->mouse_b = gui_mouse_b();

   /* set up the global  dialog pointer */
   active_dialog_player = player;
   active_dialog = dialog;

   /* set up dclick checking code */
   if (gui_install_count <= 0) {
      LOCK_VARIABLE(gui_timer);
      LOCK_VARIABLE(dclick_status);
      LOCK_VARIABLE(dclick_time);
      LOCK_VARIABLE(gui_mouse_x);
      LOCK_VARIABLE(gui_mouse_y);
      LOCK_VARIABLE(gui_mouse_z);
      LOCK_VARIABLE(gui_mouse_b);
      LOCK_FUNCTION(default_mouse_x);
      LOCK_FUNCTION(default_mouse_y);
      LOCK_FUNCTION(default_mouse_z);
      LOCK_FUNCTION(default_mouse_b);
      LOCK_FUNCTION(dclick_check);

      install_int(dclick_check, 20);

      switch (get_display_switch_mode()) {
         case SWITCH_AMNESIA:
         case SWITCH_BACKAMNESIA:
            set_display_switch_callback(SWITCH_IN, gui_switch_callback);
      }

      /* gets menu auto-opening delay (in milliseconds) from config file */
      gui_menu_opening_delay = get_config_int(uconvert_ascii("system", tmp1), uconvert_ascii("menu_opening_delay", tmp2), 300);
      if (gui_menu_opening_delay >= 0) {
         /* adjust for actual timer speed */
         gui_menu_opening_delay /= 20;
      }
      else {
         /* no auto opening */
         gui_menu_opening_delay = -1;
      }

      gui_install_count = 1;
      gui_install_time = _allegro_count;
   }
   else
      gui_install_count++;

   /* initialise the dialog */
   set_clip_rect(gui_bmp, 0, 0, SCREEN_W-1, SCREEN_H-1);
   set_clip_state(gui_bmp, TRUE);
   player->res |= dialog_message(dialog, MSG_START, 0, &player->obj);

   player->mouse_obj = find_mouse_object(dialog);
   if (player->mouse_obj >= 0)
      dialog[player->mouse_obj].flags |= D_GOTMOUSE;

   for (c=0; dialog[c].proc; c++)
      dialog[c].flags &= ~D_GOTFOCUS;

   if (focus_obj >= 0)
      c = focus_obj;
   else
      c = player->mouse_obj;

   if ((c >= 0) && ((object_message(dialog+c, MSG_WANTFOCUS, 0)) & D_WANTFOCUS)) {
      dialog[c].flags |= D_GOTFOCUS;
      player->focus_obj = c;
   }
   else
      player->focus_obj = -1;

   return player;
}



/* gui_set_screen:
 *  Changes the target bitmap for GUI drawing operations
 */
void gui_set_screen(BITMAP *bmp)
{
   gui_screen = bmp;
}



/* gui_get_screen:
 *  Returns the gui_screen, or the default surface if gui_screen is NULL
 */
BITMAP *gui_get_screen(void)
{
   return gui_screen?gui_screen:screen;
}



/* check_for_redraw:
 *  Checks whether any parts of the current dialog need to be redraw.
 */
static void check_for_redraw(DIALOG_PLAYER *player)
{
   struct al_active_dialog_player *iter;
   int c, r;
   ASSERT(player);

   /* need to redraw all active dialogs? */
   if (player->res & D_REDRAW_ALL) {
      for (iter = first_active_dialog_player; iter != current_active_dialog_player; iter = iter->next)
	 dialog_message(iter->player->dialog, MSG_DRAW, 0, NULL);

      player->res &= ~D_REDRAW_ALL;
      player->res |= D_REDRAW;
   }

   /* need to draw it? */
   if (player->res & D_REDRAW) {
      player->res ^= D_REDRAW;
      player->res |= dialog_message(player->dialog, MSG_DRAW, 0, &player->obj);
   }

   /* check if any widget has to be redrawn */
   for (c=0; player->dialog[c].proc; c++) {
      if ((player->dialog[c].flags & (D_DIRTY | D_HIDDEN)) == D_DIRTY) {
	 player->dialog[c].flags &= ~D_DIRTY;
	 MESSAGE(c, MSG_DRAW, 0);
      }
   }
}



/* update_dialog:
 *  Updates the status of a dialog object returned by init_dialog(),
 *  returning TRUE if it is still active or FALSE if it has finished.
 */
int update_dialog(DIALOG_PLAYER *player)
{
   int c, cascii, cscan, ccombo, r, ret, nowhere, z;
   int new_mouse_b;
   ASSERT(player);

   /* redirect to update_menu() whenever a menu is activated */
   if (active_menu_player) {
      if (!active_menu_player_zombie) {
	 if (update_menu(active_menu_player))
	    return TRUE;
      }

      /* make sure all buttons are released before folding the menu */
      if (gui_mouse_b()) {
	 active_menu_player_zombie = TRUE;
	 return TRUE;
      }
      else {
	 active_menu_player_zombie = FALSE;

	 for (c=0; player->dialog[c].proc; c++)
	    if (&player->dialog[c] == active_menu_player->dialog)
	       break;
	 ASSERT(player->dialog[c].proc);

	 MESSAGE(c, MSG_LOSTMOUSE, 0);
	 goto getout;
      }
   }

   if (player->res & D_CLOSE)
      return FALSE;

   /* deal with mouse buttons presses and releases */
   new_mouse_b = gui_mouse_b();
   if (new_mouse_b != player->mouse_b) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 /* send press and release messages */
         if ((new_mouse_b & 1) && !(player->mouse_b & 1))
	    MESSAGE(player->mouse_obj, MSG_LPRESS, new_mouse_b);
         if (!(new_mouse_b & 1) && (player->mouse_b & 1))
	    MESSAGE(player->mouse_obj, MSG_LRELEASE, new_mouse_b);

         if ((new_mouse_b & 4) && !(player->mouse_b & 4))
	    MESSAGE(player->mouse_obj, MSG_MPRESS, new_mouse_b);
         if (!(new_mouse_b & 4) && (player->mouse_b & 4))
	    MESSAGE(player->mouse_obj, MSG_MRELEASE, new_mouse_b);

         if ((new_mouse_b & 2) && !(player->mouse_b & 2))
	    MESSAGE(player->mouse_obj, MSG_RPRESS, new_mouse_b);
         if (!(new_mouse_b & 2) && (player->mouse_b & 2))
	    MESSAGE(player->mouse_obj, MSG_RRELEASE, new_mouse_b);

         player->mouse_b = new_mouse_b;
      }
      else
	 player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);
   }

   /* need to reinstall the dclick and switch handlers? */
   if (gui_install_time != _allegro_count) {
      install_int(dclick_check, 20);

      switch (get_display_switch_mode()) {
         case SWITCH_AMNESIA:
         case SWITCH_BACKAMNESIA:
            set_display_switch_callback(SWITCH_IN, gui_switch_callback);
      }

      gui_install_time = _allegro_count;
   }

   /* are we dealing with a mouse click? */
   if (player->click_wait) {
      if ((ABS(player->mouse_ox-gui_mouse_x()) > 8) ||
	  (ABS(player->mouse_oy-gui_mouse_y()) > 8))
	 dclick_status = DCLICK_NOT;

      /* waiting... */
      if ((dclick_status != DCLICK_AGAIN) && (dclick_status != DCLICK_NOT)) {
	 player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);
	 check_for_redraw(player);
	 return TRUE;
      }

      player->click_wait = FALSE;

      /* double click! */
      if ((dclick_status==DCLICK_AGAIN) &&
	  (gui_mouse_x() >= player->dialog[player->mouse_obj].x) &&
	  (gui_mouse_y() >= player->dialog[player->mouse_obj].y) &&
	  (gui_mouse_x() <= player->dialog[player->mouse_obj].x + player->dialog[player->mouse_obj].w) &&
	  (gui_mouse_y() <= player->dialog[player->mouse_obj].y + player->dialog[player->mouse_obj].h)) {
	 MESSAGE(player->mouse_obj, MSG_DCLICK, 0);
      }

      goto getout;
   }

   player->res &= ~D_USED_CHAR;

   /* need to give the input focus to someone? */
   if (player->res & D_WANTFOCUS) {
      player->res ^= D_WANTFOCUS;
      player->res |= offer_focus(player->dialog, player->obj, &player->focus_obj, FALSE);
   }

   /* has mouse object changed? */
   c = find_mouse_object(player->dialog);
   if (c != player->mouse_obj) {
      if (player->mouse_obj >= 0) {
	 player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;
	 MESSAGE(player->mouse_obj, MSG_LOSTMOUSE, 0);
      }
      if (c >= 0) {
	 player->dialog[c].flags |= D_GOTMOUSE;
	 MESSAGE(c, MSG_GOTMOUSE, 0);
      }
      player->mouse_obj = c;

      /* move the input focus as well? */
      if ((gui_mouse_focus) && (player->mouse_obj != player->focus_obj))
	 player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, TRUE);
   }

   /* deal with mouse button clicks */
   if (new_mouse_b) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 dclick_time = 0;
	 dclick_status = DCLICK_START;
	 player->mouse_ox = gui_mouse_x();
	 player->mouse_oy = gui_mouse_y();

	 /* send click message */
	 MESSAGE(player->mouse_obj, MSG_CLICK, new_mouse_b);

	 if (player->res == D_O_K)
	    player->click_wait = TRUE;
      }
      else
	 player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);

      /* goto getout; */  /* to avoid an updating delay */
   }

   /* deal with mouse wheel clicks */
   z = gui_mouse_z();

   if (z != player->mouse_oz) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 MESSAGE(player->mouse_obj, MSG_WHEEL, z-player->mouse_oz);
      }
      else
	 player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);

      player->mouse_oz = z;

      /* goto getout; */  /* to avoid an updating delay */
   }

   /* fake joystick input by converting it to key presses */
   if (player->joy_on)
      rest(20);

   poll_joystick();

   if (player->joy_on) {
      if ((!joy[0].stick[0].axis[0].d1) && (!joy[0].stick[0].axis[0].d2) &&
	  (!joy[0].stick[0].axis[1].d1) && (!joy[0].stick[0].axis[1].d2) &&
	  (!joy[0].button[0].b) && (!joy[0].button[1].b)) {
	 player->joy_on = FALSE;
	 rest(20);
      }
      cascii = cscan = 0;
   }
   else {
      if (joy[0].stick[0].axis[0].d1) {
	 cascii = 0;
	 cscan = KEY_LEFT;
	 player->joy_on = TRUE;
      }
      else if (joy[0].stick[0].axis[0].d2) {
	 cascii = 0;
	 cscan = KEY_RIGHT;
	 player->joy_on = TRUE;
      }
      else if (joy[0].stick[0].axis[1].d1) {
	 cascii = 0;
	 cscan = KEY_UP;
	 player->joy_on = TRUE;
      }
      else if (joy[0].stick[0].axis[1].d2) {
	 cascii = 0;
	 cscan = KEY_DOWN;
	 player->joy_on = TRUE;
      }
      else if ((joy[0].button[0].b) || (joy[0].button[1].b)) {
	 cascii = ' ';
	 cscan = KEY_SPACE;
	 player->joy_on = TRUE;
      }
      else
	 cascii = cscan = 0;
   }

   /* deal with keyboard input */
   if ((cascii) || (cscan) || (keypressed())) {
      if ((!cascii) && (!cscan))
	 cascii = ureadkey(&cscan);

      ccombo = (cscan<<8) | ((cascii <= 255) ? cascii : '^');

      /* let object deal with the key */
      if (player->focus_obj >= 0) {
	 MESSAGE(player->focus_obj, MSG_CHAR, ccombo);
	 if (player->res & (D_USED_CHAR | D_CLOSE))
	    goto getout;

	 MESSAGE(player->focus_obj, MSG_UCHAR, cascii);
	 if (player->res & (D_USED_CHAR | D_CLOSE))
	    goto getout;
      }

      /* keyboard shortcut? */
      for (c=0; player->dialog[c].proc; c++) {
	 if ((((cascii > 0) && (cascii <= 255) &&
	       (utolower(player->dialog[c].key) == utolower((cascii)))) ||
	      ((!cascii) && (player->dialog[c].key == (cscan<<8)))) &&
	     (!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED)))) {
	    MESSAGE(c, MSG_KEY, ccombo);
	    goto getout;
	 }
      }

      /* broadcast in case any other objects want it */
      for (c=0; player->dialog[c].proc; c++) {
	 if (!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED))) {
	    MESSAGE(c, MSG_XCHAR, ccombo);
	    if (player->res & D_USED_CHAR)
	       goto getout;
	 }
      }

      /* pass <CR> or <SPACE> to selected object */
      if (((cascii == '\r') || (cascii == '\n') || (cascii == ' ')) &&
	  (player->focus_obj >= 0)) {
	 MESSAGE(player->focus_obj, MSG_KEY, ccombo);
	 goto getout;
      }

      /* ESC closes dialog */
      if (cascii == 27) {
	 player->res |= D_CLOSE;
	 player->obj = -1;
	 goto getout;
      }

      /* move focus around the dialog */
      player->res |= move_focus(player->dialog, cascii, cscan, &player->focus_obj);
   }

   /* redraw? */
   check_for_redraw(player);

   /* send idle messages */
   player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &player->obj);

   getout:

   ret = (!(player->res & D_CLOSE));
   player->res &= ~D_CLOSE;
   return ret;
}



/* shutdown_dialog:
 *  Destroys a dialog object returned by init_dialog(), returning the index
 *  of the object that caused it to exit.
 */
int shutdown_dialog(DIALOG_PLAYER *player)
{
   struct al_active_dialog_player *iter, *prev;
   int obj;
   ASSERT(player);

   /* send the finish messages */
   dialog_message(player->dialog, MSG_END, 0, &player->obj);

   /* remove the double click handler */
   gui_install_count--;

   if (gui_install_count <= 0) {
      remove_int(dclick_check);
      remove_display_switch_callback(gui_switch_callback);
   }

   if (player->mouse_obj >= 0)
      player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;

   /* remove dialog player from the list of active players */
   for (iter = first_active_dialog_player, prev = 0; iter != 0; prev = iter, iter = iter->next) {
      if (iter->player == player) {
	 if (prev)
	    prev->next = iter->next;
	 else
	    first_active_dialog_player = iter->next;

	 if (iter == current_active_dialog_player)
	    current_active_dialog_player = prev;

	 _AL_FREE (iter);
	 break;
      }
   }

   if (current_active_dialog_player)
      active_dialog_player = current_active_dialog_player->player;
   else
      active_dialog_player = NULL;

   if (active_dialog_player)
      active_dialog = active_dialog_player->dialog;
   else
      active_dialog = NULL;

   obj = player->obj;

   _AL_FREE(player);

   return obj;
}



void (*gui_menu_draw_menu)(int x, int y, int w, int h) = NULL;
void (*gui_menu_draw_menu_item)(MENU *m, int x, int y, int w, int h, int bar, int sel) = NULL;



/* split_around_tab:
 *  Helper function for splitting a string into two tokens
 *  delimited by the first TAB character.
 */
static char* split_around_tab(const char *s, char **tok1, char **tok2)
{
   char *buf, *last;
   char tmp[16];

   buf = _al_ustrdup(s);
   *tok1 = ustrtok_r(buf, uconvert_ascii("\t", tmp), &last);
   *tok2 = ustrtok_r(NULL, empty_string, &last);

   return buf;
}



/* bar_entry_lengh:
 *  Helper function for calculating the length of a menu bar entry.
 */
static int bar_entry_length(const char *text)
{
   char *buf, *tok1, *tok2;
   int len;

   buf = split_around_tab(text, &tok1, &tok2);
   len = gui_strlen(tok1) + 16;
   if (tok2)
      len += gui_strlen(tok2) + 16;
   _AL_FREE(buf);

   return len;
}



/* get_menu_pos:
 *  Calculates the coordinates of an object within a top level menu bar.
 */
static void get_menu_pos(MENU_PLAYER *m, int c, int *x, int *y, int *w)
{
   int c2;

   if (m->bar) {
      *x = m->x+1;

      for (c2=0; c2<c; c2++)
	 *x += bar_entry_length(m->menu[c2].text);

      *y = m->y+1;
      *w = bar_entry_length(m->menu[c].text);
   }
   else {
      *x = m->x+1;
      *y = m->y+c*(text_height(font)+4)+1;
      *w = m->w-3;
   }
}



/* draw_menu_item:
 *  Draws an item from a popup menu onto the screen.
 */
static void draw_menu_item(MENU_PLAYER *m, int c)
{
   int fg, bg;
   int x, y, w;
   char *buf, *tok1, *tok2;
   int my;
   BITMAP *gui_bmp = gui_get_screen();

   get_menu_pos(m, c, &x, &y, &w);

   if (gui_menu_draw_menu_item) {
      gui_menu_draw_menu_item(&m->menu[c], x, y, w, text_height(font)+4,
			      m->bar, (c == m->sel) ? TRUE : FALSE);
      return;
   }

   if (m->menu[c].flags & D_DISABLED) {
      if (c == m->sel) {
	 fg = gui_mg_color;
	 bg = gui_fg_color;
      }
      else {
	 fg = gui_mg_color;
	 bg = gui_bg_color;
      }
   }
   else {
      if (c == m->sel) {
	 fg = gui_bg_color;
	 bg = gui_fg_color;
      }
      else {
	 fg = gui_fg_color;
	 bg = gui_bg_color;
      }
   }

   rectfill(gui_bmp, x, y, x+w-1, y+text_height(font)+3, bg);

   if (ugetc(m->menu[c].text)) {
      buf = split_around_tab(m->menu[c].text, &tok1, &tok2);
      gui_textout_ex(gui_bmp, tok1, x+8, y+1, fg, bg, FALSE);
      if (tok2)
 	 gui_textout_ex(gui_bmp, tok2, x+w-gui_strlen(tok2)-10, y+1, fg, bg, FALSE);

      if ((m->menu[c].child) && (!m->bar)) {
         my = y + text_height(font)/2;
         hline(gui_bmp, x+w-8, my+1, x+w-4, fg);
         hline(gui_bmp, x+w-8, my+0, x+w-5, fg);
         hline(gui_bmp, x+w-8, my-1, x+w-6, fg);
         hline(gui_bmp, x+w-8, my-2, x+w-7, fg);
         putpixel(gui_bmp, x+w-8, my-3, fg);
         hline(gui_bmp, x+w-8, my+2, x+w-5, fg);
         hline(gui_bmp, x+w-8, my+3, x+w-6, fg);
         hline(gui_bmp, x+w-8, my+4, x+w-7, fg);
         putpixel(gui_bmp, x+w-8, my+5, fg);
      }

      _AL_FREE(buf);
   }
   else
      hline(gui_bmp, x, y+text_height(font)/2+2, x+w, fg);

   if (m->menu[c].flags & D_SELECTED) {
      line(gui_bmp, x+1, y+text_height(font)/2+1, x+3, y+text_height(font)+1, fg);
      line(gui_bmp, x+3, y+text_height(font)+1, x+6, y+2, fg);
   }
}



/* draw_menu:
 *  Draws a popup menu onto the screen.
 */
static void draw_menu(MENU_PLAYER *m)
{
   int c;

   if (gui_menu_draw_menu)
      gui_menu_draw_menu(m->x, m->y, m->w, m->h);
   else {
      BITMAP *gui_bmp = gui_get_screen();
      rectfill(gui_bmp, m->x, m->y, m->x+m->w-2, m->y+m->h-2, gui_bg_color);
      rect(gui_bmp, m->x, m->y, m->x+m->w-2, m->y+m->h-2, gui_fg_color);
      vline(gui_bmp, m->x+m->w-1, m->y+1, m->y+m->h-1, gui_fg_color);
      hline(gui_bmp, m->x+1, m->y+m->h-1, m->x+m->w-1, gui_fg_color);
   }

   for (c=0; m->menu[c].text; c++)
      draw_menu_item(m, c);
}



/* menu_mouse_object:
 *  Returns the index of the object the mouse is currently on top of.
 */
static int menu_mouse_object(MENU_PLAYER *m)
{
   int c;
   int x, y, w;

   for (c=0; c<m->size; c++) {
      get_menu_pos(m, c, &x, &y, &w);

      if ((gui_mouse_x() >= x) && (gui_mouse_x() < x+w) &&
	  (gui_mouse_y() >= y) && (gui_mouse_y() < y+(text_height(font)+4)))
	 return (ugetc(m->menu[c].text)) ? c : -1;
   }

   return -1;
}



/* mouse_in_single_menu:
 *  Checks if the mouse is inside a single menu.
 */
static INLINE int mouse_in_single_menu(MENU_PLAYER *m)
{
   if ((gui_mouse_x() >= m->x) && (gui_mouse_x() < m->x+m->w) &&
       (gui_mouse_y() >= m->y) && (gui_mouse_y() < m->y+m->h))
      return TRUE;
   else
      return FALSE;
}



/* mouse_in_parent_menu:
 *  Recursively checks if the mouse is inside a menu (or any of its parents)
 *  and simultaneously not on the selected item of the menu.
 */
static int mouse_in_parent_menu(MENU_PLAYER *m)
{
   int c;

   if (!m)
      return FALSE;

   c = menu_mouse_object(m);
   if ((c >= 0) && (c != m->sel))
      return TRUE;

   return mouse_in_parent_menu(m->parent);
}



/* layout_menu:
 *  Calculates the layout of the menu.
 */
static void layout_menu(MENU_PLAYER *m, MENU *menu, int bar, int x, int y, int minw, int minh)
{
   char *buf, *tok1, *tok2;
   int extra = 0;
   int c;
   int child = FALSE;

   m->menu = menu;
   m->bar = bar;
   m->x = x;
   m->y = y;
   m->w = 3;
   m->h = (m->bar) ? (text_height(font)+7) : 3;
   m->proc = NULL;
   m->sel = -1;

   /* calculate size of the menu */
   for (m->size=0; m->menu[m->size].text; m->size++) {

      if (m->bar) {
	 m->w += bar_entry_length(m->menu[m->size].text);
      }
      else {
	 if (m->menu[m->size].child)
	    child = TRUE;

	 if (ugetc(m->menu[m->size].text)) {
	    buf = split_around_tab(m->menu[m->size].text, &tok1, &tok2);
	    c = gui_strlen(tok1);
	 }
	 else {
	    buf = NULL;
	    c = 0;
	 }

	 m->h += text_height(font)+4;
	 m->w = MAX(m->w, c+16);

	 if (buf) {
	    if (tok2) {
	       c = gui_strlen(tok2);
	       extra = MAX(extra, c);
	    }

	    _AL_FREE(buf);
	 }
      }
   }

   if (extra)
      m->w += extra+16;

   if (child)
      m->w += 22;

   m->w = MAX(m->w, minw);
   m->h = MAX(m->h, minh);
}



/* menu_key_shortcut:
 *  Returns true if c is indicated as a keyboard shortcut by a '&' character
 *  in the specified string.
 */
static int menu_key_shortcut(int c, AL_CONST char *s)
{
   int d;

   while ((d = ugetxc(&s)) != 0) {
      if (d == '&') {
	 d = ugetc(s);
	 if ((d != '&') && (utolower(d) == utolower(c & 0xFF)))
	    return TRUE;
      }
   }

   return FALSE;
}



/* menu_alt_key:
 *  Searches a menu for keyboard shortcuts, for the alt+letter to bring
 *  up a menu.
 */
static int menu_alt_key(int k, MENU *m)
{
   static unsigned char alt_table[] =
   {
      KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
      KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R,
      KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
   };

   AL_CONST char *s;
   int c, d;

   if (k & 0xFF)
      return 0;

   k >>= 8;

   c = scancode_to_ascii(k);
   if (c) {
      k = c;
   }
   else {
      for (c=0; c<(int)sizeof(alt_table); c++) {
	 if (k == alt_table[c]) {
	    k = c + 'a';
	    break;
	 }
      }

      if (c >= (int)sizeof(alt_table))
	 return 0;
   }

   for (c=0; m[c].text; c++) {
      s = m[c].text;
      while ((d = ugetxc(&s)) != 0) {
	 if (d == '&') {
	    d = ugetc(s);
	    if ((d != '&') && (utolower(d) == utolower(k)))
	       return k;
	 }
      }
   }

   return 0;
}



/* do_menu:
 *  Displays and animates a popup menu at the specified screen position,
 *  returning the index of the item that was selected, or -1 if it was
 *  dismissed. If the menu crosses the edge of the screen it will be moved.
 */
int do_menu(MENU *menu, int x, int y)
{
   MENU_PLAYER *player;
   int ret;
   ASSERT(menu);

   player = init_menu(menu, x ,y);

   while (update_menu(player))
      rest(1);

   ret = shutdown_menu(player);

   do {
   } while (gui_mouse_b());

   return ret;
}



/* init_single_menu:
 *  Worker function for initialising a menu.
 */
static MENU_PLAYER *init_single_menu(MENU *menu, MENU_PLAYER *parent, DIALOG *dialog, int bar, int x, int y, int repos, int minw, int minh)
{
   BITMAP *gui_bmp = gui_get_screen();
   int scare = is_same_bitmap(gui_bmp, _mouse_screen);
   MENU_PLAYER *player;
   ASSERT(menu);

   player = _AL_MALLOC(sizeof(MENU_PLAYER));
   if (!player) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   layout_menu(player, menu, bar, x, y, minw, minh);

   if (repos) {
      if(parent && !parent->bar) {
         if(player->x + player->w >= SCREEN_W)
            player->x = parent->x - player->w + 1;
      }
      player->x = CLAMP(0, player->x, SCREEN_W-player->w-1);
      player->y = CLAMP(0, player->y, SCREEN_H-player->h-1);
   }

   if (scare)
      scare_mouse_area(player->x, player->y, player->w, player->h);

   /* save screen under the menu */
   player->saved = create_bitmap(player->w, player->h);

   if (player->saved)
      blit(gui_bmp, player->saved, player->x, player->y, 0, 0, player->w, player->h);
   else
      *allegro_errno = ENOMEM;

   /* setup state variables */
   player->sel = menu_mouse_object(player);

   if (scare)
      unscare_mouse();

   player->mouse_button_was_pressed = gui_mouse_b();
   player->back_from_child = FALSE;
   player->timestamp = gui_timer;
   player->mouse_sel = player->sel;
   player->redraw = TRUE;
   player->auto_open = TRUE;
   player->ret = -1;

   player->dialog = dialog;

   player->parent = parent;
   player->child = NULL;

   return player;
}



/* init_menu:
 *  Sets up a menu, returning a menu player object that can be used
 *  with the update_menu() and shutdown_menu() functions.
 */
MENU_PLAYER *init_menu(MENU *menu, int x, int y)
{
   return init_single_menu(menu, NULL, NULL, FALSE, x, y, TRUE, 0, 0);
}



/* update_menu:
 *  Updates the status of a menu player object returned by init_menu(),
 *  returning TRUE if it is still active or FALSE if it has finished.
 *
 *  The navigation through the arborescence of menus can be done:
 *   - with the arrow keys,
 *   - with mouse point-and-clicks,
 *   - with mouse movements when the mouse button is being held down,
 *   - with mouse movements only if gui_menu_opening_delay is non negative.
 */
int update_menu(MENU_PLAYER *player)
{
   MENU_PLAYER *i;
   int c, c2;
   int old_sel, child_ret;
   int child_x, child_y;
   ASSERT(player);

   /* find activated menu */
   while (player->child)
      player = player->child;

   old_sel = player->sel;

   c = menu_mouse_object(player);

   if ((gui_mouse_b()) || (c != player->mouse_sel)) {
      player->sel = player->mouse_sel = c;
      player->auto_open = TRUE;
   }

   if (gui_mouse_b()) {  /* button pressed? */
      /* Dismiss menu if:
       *  - the mouse cursor is outside the menu and inside the parent menu, or
       *  - the mouse cursor is outside the menu and the button has just been pressed.
       */
      if (!mouse_in_single_menu(player)) {
	 if (mouse_in_parent_menu(player->parent) || (!player->mouse_button_was_pressed)) {
	    player->ret = -2;
	    goto End;
	 }
      }

      if ((player->sel >= 0) && (player->menu[player->sel].child))  /* bring up child menu? */
	 player->ret = player->sel;

      /* don't trigger the 'select' event on button press for non menu item */
      player->mouse_button_was_pressed = TRUE;

      clear_keybuf();
   }
   else {   /* button not pressed */
      /* trigger the 'select' event only on button release for non menu item */
      if (player->mouse_button_was_pressed) {
	 player->ret = player->sel;
	 player->mouse_button_was_pressed = FALSE;
      }

      if (keypressed()) {  /* keyboard input */
	 player->timestamp = gui_timer;
	 player->auto_open = FALSE;

	 c = readkey();

	 if ((c & 0xFF) == 27) {
	    player->ret = -2;
	    goto End;
	 }

	 switch (c >> 8) {

	    case KEY_LEFT:
	       if (player->parent) {
		  if (player->parent->bar) {
		     simulate_keypress(KEY_LEFT<<8);
		     simulate_keypress(KEY_DOWN<<8);
		  }
		  player->ret = -2;
		  goto End;
	       }
	       /* fall through */

	    case KEY_UP:
	       if ((((c >> 8) == KEY_LEFT) && (player->bar)) ||
		   (((c >> 8) == KEY_UP) && (!player->bar))) {
		  c = player->sel;
		  do {
		     c--;
		     if (c < 0)
			c = player->size - 1;
		  } while ((!ugetc(player->menu[c].text)) && (c != player->sel));
		  player->sel = c;
	       }
	       break;

	    case KEY_RIGHT:
	       if (((player->sel < 0) || (!player->menu[player->sel].child)) &&
		   (player->parent) && (player->parent->bar)) {
		  simulate_keypress(KEY_RIGHT<<8);
		  simulate_keypress(KEY_DOWN<<8);
		  player->ret = -2;
		  goto End;
	       }
	       /* fall through */

	    case KEY_DOWN:
	       if ((player->sel >= 0) && (player->menu[player->sel].child) &&
		   ((((c >> 8) == KEY_RIGHT) && (!player->bar)) ||
		    (((c >> 8) == KEY_DOWN) && (player->bar)))) {
		  player->ret = player->sel;
	       }
	       else if ((((c >> 8) == KEY_RIGHT) && (player->bar)) ||
			(((c >> 8) == KEY_DOWN) && (!player->bar))) {
		  c = player->sel;
		  do {
		     c++;
		     if (c >= player->size)
			c = 0;
		  } while ((!ugetc(player->menu[c].text)) && (c != player->sel));
		  player->sel = c;
	       }
	       break;

	    case KEY_SPACE:
	    case KEY_ENTER:
	       if (player->sel >= 0)
		  player->ret = player->sel;
	       break;

	    default:
	       if ((!player->parent) && ((c & 0xFF) == 0))
		  c = menu_alt_key(c, player->menu);
	       for (c2=0; player->menu[c2].text; c2++) {
		  if (menu_key_shortcut(c, player->menu[c2].text)) {
		     player->ret = player->sel = c2;
		     break;
		  }
	       }
	       if (player->parent) {
		  i = player->parent;
		  for (c2=0; i->parent; c2++)
		     i = i->parent;
		  c = menu_alt_key(c, i->menu);
		  if (c) {
		     while (c2-- > 0)
			simulate_keypress(27);
		     simulate_keypress(c);
		     player->ret = -2;
		     goto End;
		  }
	       }
	       break;
	 }
      }
   }  /* end of input processing */

   if ((player->redraw) || (player->sel != old_sel)) {  /* selection changed? */
      BITMAP *gui_bmp = gui_get_screen();
      int scare = is_same_bitmap(gui_bmp, _mouse_screen);
      player->timestamp = gui_timer;

      if (scare)
	 scare_mouse_area(player->x, player->y, player->w, player->h);
      acquire_bitmap(gui_bmp);

      if (player->redraw) {
	 draw_menu(player);
	 player->redraw = FALSE;
      }
      else {
	 if (old_sel >= 0)
	    draw_menu_item(player, old_sel);

	 if (player->sel >= 0)
	    draw_menu_item(player, player->sel);
      }

      release_bitmap(gui_bmp);
      if (scare)
	 unscare_mouse();
   }

   if (player->auto_open && (gui_menu_opening_delay >= 0)) {  /* menu auto-opening on? */
      if (!mouse_in_single_menu(player)) {
	 if (mouse_in_parent_menu(player->parent)) {
	    /* automatically goes back to parent */
	    player->ret = -3;
	    goto End;
	 }
      }

      if ((player->mouse_sel >= 0) && (player->menu[player->mouse_sel].child)) {
	 if (player->bar) {
	    /* top level menu auto-opening if back from child */
	    if (player->back_from_child) {
	       player->timestamp = gui_timer;
	       player->ret = player->mouse_sel;
	    }
	 }
	 else {
	    /* sub menu auto-opening if enough time has passed */
	    if ((gui_timer - player->timestamp) > gui_menu_opening_delay)
	       player->ret = player->mouse_sel;
	 }
      }

      player->back_from_child = FALSE;
   }

 End:
   if (player->ret >= 0) {  /* item selected? */
      if (player->menu[player->ret].flags & D_DISABLED) {
	 return TRUE;  /* continue */
      }
      else if (player->menu[player->ret].child) {  /* child menu? */
	 if (player->bar) {
	    get_menu_pos(player, player->ret, &child_x, &child_y, &c);
	    child_x += 6;
	    child_y += text_height(font) + 7;
	 }
	 else {
	    child_x = player->x+player->w - 3;
	    child_y = player->y + (text_height(font)+4)*player->ret + text_height(font)/4 + 1;
	 }

	 /* recursively call child menu */
	 player->child = init_single_menu(player->menu[player->ret].child, player, NULL, FALSE, child_x, child_y, TRUE, 0, 0);
	 return TRUE;  /* continue */
      }

      while (player->parent) {  /* parent menu? */
	 player = player->parent;
	 shutdown_single_menu(player->child, NULL);
	 player->child = NULL;
      }

      return FALSE;  /* item selected */
   }

   if (player->ret < -1) {  /* dismiss menu ? */
      if (player->parent) {
	 child_ret = player->ret;  /* needed below */
	 player = player->parent;
	 shutdown_single_menu(player->child, NULL);
	 player->child = NULL;
	 player->ret = -1;
	 player->mouse_button_was_pressed = FALSE;
	 player->mouse_sel = menu_mouse_object(player);

	 if (child_ret == -3) {  /* return caused by mouse movement? */
	    player->sel = player->mouse_sel;
	    player->redraw = TRUE;
	    player->timestamp = gui_timer;
	    player->back_from_child = TRUE;
	 }

	 return TRUE;  /* return to parent */
      }

      return FALSE;  /* menu dismissed */
   }

   /* special kludge for menu bar */
   if ((player->bar) && (!gui_mouse_b()) && (!keypressed()) && (!mouse_in_single_menu(player)))
      return FALSE;

   return TRUE;
}



/* shutdown_single_menu:
 *  Worker function for shutting down a menu.
 */
static int shutdown_single_menu(MENU_PLAYER *player, int *dret)
{
   int ret;
   ASSERT(player);

   if (dret)
      *dret = 0;

   if ((!player->proc) && (player->ret >= 0)) {   /* callback function? */
      active_menu = &player->menu[player->ret];
      player->proc = active_menu->proc;
   }

   if (player->ret >= 0) {
      if (player->parent)
	 player->parent->proc = player->proc;
      else  {
	 if (player->proc) {
	    ret = player->proc();
	    if (dret)
	       *dret = ret;
	 }
      }
   }

   /* restore screen */
   if (player->saved) {
      BITMAP *gui_bmp = gui_get_screen();
      int scare = is_same_bitmap(gui_bmp, _mouse_screen);
      if (scare)
         scare_mouse_area(player->x, player->y, player->w, player->h);
      blit(player->saved, gui_bmp, 0, 0, player->x, player->y, player->w, player->h);
      if (scare)
         unscare_mouse();
      destroy_bitmap(player->saved);
   }

   ret = player->ret;

   _AL_FREE(player);

   return ret;
}



/* shutdown_tree_menu:
 *  Destroys a menu player object returned by init_single_menu(), after
 *  recursively closing all the sub-menus if necessary, and returns the
 *  index of the item that was selected, or -1 if it was dismissed.
 */
static int shutdown_tree_menu(MENU_PLAYER *player, int *dret)
{
   ASSERT(player);

   if (player->child) {
      shutdown_tree_menu(player->child, dret);
      player->child = NULL;
   }

   return shutdown_single_menu(player, dret);
}



/* shutdown_menu:
 *  Destroys a menu player object returned by init_menu() and returns
 *  the index of the item that was selected, or -1 if it was dismissed.
 */
int shutdown_menu(MENU_PLAYER *player)
{
   return shutdown_tree_menu(player, NULL);
}



/* d_menu_proc:
 *  Dialog procedure for adding drop down menus to a GUI dialog. This
 *  displays the top level menu items as a horizontal bar (eg. across the
 *  top of the screen), and pops up child menus when they are clicked.
 *  When it executes one of the menu callback routines, it passes the
 *  return value back to the dialog manager, so these can return D_O_K,
 *  D_CLOSE, D_REDRAW, etc.
 */
int d_menu_proc(int msg, DIALOG *d, int c)
{
   MENU_PLAYER m, *mp;
   int ret = D_O_K;
   int x, i;
   ASSERT(d);

   switch (msg) {

      case MSG_START:
	 layout_menu(&m, d->dp, TRUE, d->x, d->y, d->w, d->h);
	 d->w = m.w;
	 d->h = m.h;
	 break;

      case MSG_DRAW:
	 layout_menu(&m, d->dp, TRUE, d->x, d->y, d->w, d->h);
	 draw_menu(&m);
	 break;

      case MSG_XCHAR:
	 x = menu_alt_key(c, d->dp);
	 if (!x)
	    break;

	 ret |= D_USED_CHAR;
	 simulate_keypress(x);
	 /* fall through */

      case MSG_GOTMOUSE:
      case MSG_CLICK:
	 /* steal the mouse */
	 for (i=0; active_dialog[i].proc; i++)
	    if (active_dialog[i].flags & D_GOTMOUSE) {
	       active_dialog[i].flags &= ~D_GOTMOUSE;
	       object_message(active_dialog+i, MSG_LOSTMOUSE, 0);
	       break;
	    }

	 /* initialize the menu */
	 active_menu_player = init_single_menu(d->dp, NULL, d, TRUE, d->x, d->y, FALSE, d->w, d->h);
	 break;

      case MSG_LOSTMOUSE:
      case MSG_END:
	 if (active_menu_player) {
	    /* shutdown_tree_menu may call nested dialogs */
	    mp = active_menu_player;
	    active_menu_player = NULL;
	    shutdown_tree_menu(mp, &x);
	    ret |= x;

	    /* put the mouse */
	    i = find_mouse_object(active_dialog);
	    if ((i >= 0) && (&active_dialog[i] != d)) {
	       active_dialog[i].flags |= D_GOTMOUSE;
	       object_message(active_dialog+i, MSG_GOTMOUSE, 0);
	    }
	 }
	 break;
   }

   return ret;
}



static DIALOG alert_dialog[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)  (dp2) (dp3) */
   { _gui_shadow_box_proc, 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL, NULL, NULL  },
   { _gui_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL, NULL, NULL  },
   { _gui_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL, NULL, NULL  },
   { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { NULL,                 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  }
};


#define A_S1  1
#define A_S2  2
#define A_S3  3
#define A_B1  4
#define A_B2  5
#define A_B3  6



/* alert3:
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one, two, or three buttons. The text for these buttons
 *  is passed in b1, b2, and b3 (NULL for buttons which are not used), and
 *  the keyboard shortcuts in c1 and c2. Returns 1, 2, or 3 depending on
 *  which button was selected.
 */
int alert3(AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, AL_CONST char *b3, int c1, int c2, int c3)
{
   char tmp[16];
   int avg_w, avg_h;
   int len1, len2, len3;
   int maxlen = 0;
   int buttons = 0;
   int b[3];
   int c;

   #define SORT_OUT_BUTTON(x) {                                            \
      if (b##x) {                                                          \
	 alert_dialog[A_B##x].flags &= ~D_HIDDEN;                          \
	 alert_dialog[A_B##x].key = c##x;                                  \
	 alert_dialog[A_B##x].dp = (char *)b##x;                           \
	 len##x = gui_strlen(b##x);                                        \
	 b[buttons++] = A_B##x;                                            \
      }                                                                    \
      else {                                                               \
	 alert_dialog[A_B##x].flags |= D_HIDDEN;                           \
	 len##x = 0;                                                       \
      }                                                                    \
   }

   usetc(tmp+usetc(tmp, ' '), 0);

   avg_w = text_length(font, tmp);
   avg_h = text_height(font);

   alert_dialog[A_S1].dp = alert_dialog[A_S2].dp = alert_dialog[A_S3].dp =
   alert_dialog[A_B1].dp = alert_dialog[A_B2].dp = empty_string;

   if (s1) {
      alert_dialog[A_S1].dp = (char *)s1;
      maxlen = text_length(font, s1);
   }

   if (s2) {
      alert_dialog[A_S2].dp = (char *)s2;
      len1 = text_length(font, s2);
      if (len1 > maxlen)
	 maxlen = len1;
   }

   if (s3) {
      alert_dialog[A_S3].dp = (char *)s3;
      len1 = text_length(font, s3);
      if (len1 > maxlen)
	 maxlen = len1;
   }

   SORT_OUT_BUTTON(1);
   SORT_OUT_BUTTON(2);
   SORT_OUT_BUTTON(3);

   len1 = MAX(len1, MAX(len2, len3)) + avg_w*3;
   if (len1*buttons > maxlen)
      maxlen = len1*buttons;

   maxlen += avg_w*4;
   alert_dialog[0].w = maxlen;
   alert_dialog[A_S1].w = alert_dialog[A_S2].w = alert_dialog[A_S3].w = maxlen - avg_w*2;
   alert_dialog[A_S1].x = alert_dialog[A_S2].x = alert_dialog[A_S3].x =
						alert_dialog[0].x + avg_w;

   alert_dialog[A_B1].w = alert_dialog[A_B2].w = alert_dialog[A_B3].w = len1;

   alert_dialog[A_B1].x = alert_dialog[A_B2].x = alert_dialog[A_B3].x =
				       alert_dialog[0].x + maxlen/2 - len1/2;

   if (buttons == 3) {
      alert_dialog[b[0]].x = alert_dialog[0].x + maxlen/2 - len1*3/2 - avg_w;
      alert_dialog[b[2]].x = alert_dialog[0].x + maxlen/2 + len1/2 + avg_w;
   }
   else if (buttons == 2) {
      alert_dialog[b[0]].x = alert_dialog[0].x + maxlen/2 - len1 - avg_w;
      alert_dialog[b[1]].x = alert_dialog[0].x + maxlen/2 + avg_w;
   }

   alert_dialog[0].h = avg_h*8;
   alert_dialog[A_S1].y = alert_dialog[0].y + avg_h;
   alert_dialog[A_S2].y = alert_dialog[0].y + avg_h*2;
   alert_dialog[A_S3].y = alert_dialog[0].y + avg_h*3;
   alert_dialog[A_S1].h = alert_dialog[A_S2].h = alert_dialog[A_S3].h = avg_h;
   alert_dialog[A_B1].y = alert_dialog[A_B2].y = alert_dialog[A_B3].y = alert_dialog[0].y + avg_h*5;
   alert_dialog[A_B1].h = alert_dialog[A_B2].h = alert_dialog[A_B3].h = avg_h*2;

   centre_dialog(alert_dialog);
   set_dialog_color(alert_dialog, gui_fg_color, gui_bg_color);
   for (c = 0; alert_dialog[c].proc; c++)
      if (alert_dialog[c].proc == _gui_ctext_proc)
	 alert_dialog[c].bg = -1;

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   c = popup_dialog(alert_dialog, A_B1);

   if (c == A_B1)
      return 1;
   else if (c == A_B2)
      return 2;
   else
      return 3;
}



/* alert:
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one or two buttons. The text for these buttons is passed
 *  in b1 and b2 (b2 may be null), and the keyboard shortcuts in c1 and c2.
 *  Returns 1 or 2 depending on which button was selected.
 */
int alert(AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, int c1, int c2)
{
   int ret;

   ret = alert3(s1, s2, s3, b1, b2, NULL, c1, c2, 0);

   if (ret > 2)
      ret = 2;

   return ret;
}
