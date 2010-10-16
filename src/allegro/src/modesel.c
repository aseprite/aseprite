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
 *      GUI routines.
 *
 *      The graphics mode selection dialog.
 *
 *      By Shawn Hargreaves.
 *
 *      Rewritten by Henrik Stokseth.
 *
 *      Magnus Henoch modified it to keep the current selection
 *      across the changes as much as possible.
 *
 *      gfx_mode_select_filter() added by Vincent Penquerc'h.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"


static AL_CONST char *gfx_mode_getter(int index, int *list_size);
static AL_CONST char *gfx_card_getter(int index, int *list_size);
static AL_CONST char *gfx_depth_getter(int index, int *list_size);

static int change_proc(int msg, DIALOG *d, int c);


#define ALL_BPP(w, h) { w, h, { TRUE, TRUE, TRUE, TRUE, TRUE }}

#define N_COLOR_DEPTH  5

static int bpp_value_list[N_COLOR_DEPTH] = {8, 15, 16, 24, 32};

typedef struct MODE_LIST {
   int  w, h;
   char has_bpp[N_COLOR_DEPTH];
} MODE_LIST;

#define DRVNAME_SIZE  128

typedef struct DRIVER_LIST {
   int       id;
   char      name[DRVNAME_SIZE];
   int       mode_list_owned;
   MODE_LIST *mode_list;
   int       mode_count;
} DRIVER_LIST;

static MODE_LIST default_mode_list[] =
{
   ALL_BPP(320,  200 ),
   ALL_BPP(320,  240 ),
   ALL_BPP(640,  400 ),
   ALL_BPP(640,  480 ),
   ALL_BPP(800,  600 ),
   ALL_BPP(1024, 768 ),
   ALL_BPP(1280, 960 ),
   ALL_BPP(1280, 1024),
   ALL_BPP(1600, 1200),
   ALL_BPP(80,   80  ),
   ALL_BPP(160,  120 ),
   ALL_BPP(256,  200 ),
   ALL_BPP(256,  224 ),
   ALL_BPP(256,  240 ),
   ALL_BPP(256,  256 ),
   ALL_BPP(320,  100 ),
   ALL_BPP(320,  350 ),
   ALL_BPP(320,  400 ),
   ALL_BPP(320,  480 ),
   ALL_BPP(320,  600 ),
   ALL_BPP(360,  200 ),
   ALL_BPP(360,  240 ),
   ALL_BPP(360,  270 ),
   ALL_BPP(360,  360 ),
   ALL_BPP(360,  400 ),
   ALL_BPP(360,  480 ),
   ALL_BPP(360,  600 ),
   ALL_BPP(376,  282 ),
   ALL_BPP(376,  308 ),
   ALL_BPP(376,  564 ),
   ALL_BPP(400,  150 ),
   ALL_BPP(400,  300 ),
   ALL_BPP(400,  600 ),
   ALL_BPP(0,    0   )
};


typedef int (*FILTER_FUNCTION)(int, int, int, int);


static DRIVER_LIST *driver_list;
static int driver_count;

static char mode_string[64];
static DIALOG *what_dialog;



static DIALOG gfx_mode_dialog[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)                     (dp2) (dp3) */
   { _gui_shadow_box_proc, 0,    0,    313,  159,  0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { change_proc,          0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    0,                       0,    0     },
   { _gui_ctext_proc,      156,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  105,  101,  17,   0,    0,    0,    D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  127,  101,  17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_list_proc,       16,   28,   165,  116,  0,    0,    0,    D_EXIT,  0,    0,    (void*)gfx_card_getter,  NULL, NULL  },
   { _gui_list_proc,       196,  28,   101,  68,   0,    0,    0,    D_EXIT,  3,    0,    (void*)gfx_mode_getter,  NULL, NULL  },
   { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { NULL,                 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  }
};



static DIALOG gfx_mode_ex_dialog[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)                     (dp2) (dp3) */
   { _gui_shadow_box_proc, 0,    0,    313,  159,  0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { change_proc,          0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    0,                       0,    0     },
   { _gui_ctext_proc,      156,  8,    1,    1,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  105,  101,  17,   0,    0,    0,    D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_button_proc,     196,  127,  101,  17,   0,    0,    27,   D_EXIT,  0,    0,    NULL,                    NULL, NULL  },
   { _gui_list_proc,       16,   28,   165,  68,   0,    0,    0,    D_EXIT,  0,    0,    (void*)gfx_card_getter,  NULL, NULL  },
   { _gui_list_proc,       196,  28,   101,  68,   0,    0,    0,    D_EXIT,  3,    0,    (void*)gfx_mode_getter,  NULL, NULL  },
   { _gui_list_proc,       16,   105,  165,  44,   0,    0,    0,    D_EXIT,  0,    0,    (void*)gfx_depth_getter, NULL, NULL  },
   { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  },
   { NULL,                 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,                    NULL, NULL  }
};


#define GFX_CHANGEPROC     1
#define GFX_TITLE          2
#define GFX_OK             3
#define GFX_CANCEL         4
#define GFX_DRIVERLIST     5
#define GFX_MODELIST       6
#define GFX_DEPTHLIST      7



/* bpp_value:
 *  Returns the bpp value of the color depth pointed to by INDEX.
 */
static INLINE int bpp_value(int index)
{
   ASSERT(index < N_COLOR_DEPTH);

   return bpp_value_list[index];
}



/* bpp_value_for_mode:
 *  Returns the bpp value of the color depth pointed to by INDEX
 *  for the specified MODE of the specified DRIVER or -1 if INDEX
 *  is out of bound.
 */
static int bpp_value_for_mode(int index, int driver, int mode)
{
   int i, j = -1;

   ASSERT(index < N_COLOR_DEPTH);

   for (i=0; i < N_COLOR_DEPTH; i++) {
      if (driver_list[driver].mode_list[mode].has_bpp[i]) {
         if (++j == index)
            return bpp_value(i);
      }
   }

   return -1;
}



/* bpp_index:
 *  Returns the bpp index of the color depth given by DEPTH.
 */
static int bpp_index(int depth)
{
   int i;

   for (i=0; i < N_COLOR_DEPTH; i++) {
      if (bpp_value_list[i] == depth)
         return i;
   }

   ASSERT(FALSE);
   return -1;
}



/* bpp_index_for_mode:
 *  Returns the bpp index of the color depth given by DEPTH for
 *  the specified MODE of the specified DRIVER or -1 if DEPTH
 *  is not supported.
 */
static int bpp_index_for_mode(int depth, int driver, int mode)
{
   int i, index = -1;

   for (i=0; i < N_COLOR_DEPTH; i++) {
      if (driver_list[driver].mode_list[mode].has_bpp[i]) {
         index++;
         if (bpp_value_list[i] == depth)
            return index;
      }
   }

   return -1;
}



/* change_proc:
 *  Stores the current driver in d1 and graphics mode in d2; if a new
 *  driver is selected in the listbox, it updates the resolution and 
 *  color depth listboxes so that they redraw; if a new resolution is
 *  selected in the listbox, it updates the color depth listbox. The
 *  current selection is kept across the changes as much as possible.
 */
static int change_proc(int msg, DIALOG *d, int c)
{
   int width = driver_list[d->d1].mode_list[d->d2].w;
   int height = driver_list[d->d1].mode_list[d->d2].h;
   int depth = bpp_value_for_mode(what_dialog[GFX_DEPTHLIST].d1, d->d1, d->d2);
   int i;

   ASSERT(d);

   if (msg != MSG_IDLE)
      return D_O_K;

   if (what_dialog[GFX_DRIVERLIST].d1 != d->d1) {
      /* record the new setting */
      d->d1 = what_dialog[GFX_DRIVERLIST].d1;

      /* try to preserve the resolution */      
      what_dialog[GFX_MODELIST].d1 = 0;

      for (i = 0; i < driver_list[d->d1].mode_count; i++) {
         if (driver_list[d->d1].mode_list[i].w == width &&
             driver_list[d->d1].mode_list[i].h == height) {
           what_dialog[GFX_MODELIST].d1 = i;
           break;
         }
      }

      what_dialog[GFX_MODELIST].d2 = 0;
      object_message(&what_dialog[GFX_MODELIST], MSG_DRAW, 0);

      /* record the new setting */
      d->d2 = what_dialog[GFX_MODELIST].d1;

      if (what_dialog == gfx_mode_ex_dialog) {
         /* try to preserve the color depth */
         what_dialog[GFX_DEPTHLIST].d1 = bpp_index_for_mode(depth, d->d1, d->d2);
         if (what_dialog[GFX_DEPTHLIST].d1 < 0)
            what_dialog[GFX_DEPTHLIST].d1 = 0;

         what_dialog[GFX_DEPTHLIST].d2 = 0;
         object_message(&what_dialog[GFX_DEPTHLIST], MSG_DRAW, 0);
      }
   }

   if (what_dialog[GFX_MODELIST].d1 != d->d2) {
      /* record the new setting */
      d->d2 = what_dialog[GFX_MODELIST].d1;

      if (what_dialog == gfx_mode_ex_dialog) {
         /* try to preserve the color depth */
         what_dialog[GFX_DEPTHLIST].d1 = bpp_index_for_mode(depth, d->d1, d->d2);
         if (what_dialog[GFX_DEPTHLIST].d1 < 0)
            what_dialog[GFX_DEPTHLIST].d1 = 0;

         what_dialog[GFX_DEPTHLIST].d2 = 0;
         object_message(&what_dialog[GFX_DEPTHLIST], MSG_DRAW, 0);
      }
   }

   return D_O_K;
}



/* add_mode:
 *  Helper function for adding a mode to a mode list.
 *  Returns 0 on success and -1 on failure.
 */
static int add_mode(MODE_LIST **list, int *size, int w, int h, int bpp)
{
   int mode, n;

   /* Resolution already there? */
   for (mode = 0; mode < *size; mode++) {
      if ((w == (*list)[mode].w) && (h == (*list)[mode].h)) {
	 (*list)[mode].has_bpp[bpp_index(bpp)] = TRUE;
	 return 0;
      }
   }

   /* Add new resolution. */
   *list = _al_sane_realloc(*list, sizeof(MODE_LIST) * ++(*size));
   if (!list)
      return -1;

   mode = *size - 1;
   (*list)[mode].w = w;
   (*list)[mode].h = h;
   for (n = 0; n < N_COLOR_DEPTH; n++)
      (*list)[mode].has_bpp[n] = (bpp == bpp_value(n));

   return 0;
}



/* terminate_list:
 *  Helper function for terminating a mode list.
 */
static int terminate_list(MODE_LIST **list, int size)
{
   *list = _al_sane_realloc(*list, sizeof(MODE_LIST) * (size + 1));
   if (!list)
      return -1;

   /* Add terminating marker. */
   (*list)[size].w = 0;
   (*list)[size].h = 0;

   return 0;
}



/* create_mode_list:
 *  Creates a mode list table.
 *  Returns 0 on success and -1 on failure.
 */
static int create_mode_list(DRIVER_LIST *driver_list_entry, FILTER_FUNCTION filter)
{
   MODE_LIST *temp_mode_list = NULL;
   GFX_MODE_LIST *gfx_mode_list;
   GFX_MODE *gfx_mode_entry;
   int mode, n, w, h, bpp;
   int is_autodetected = ((driver_list_entry->id == GFX_AUTODETECT)
			  || (driver_list_entry->id == GFX_AUTODETECT_WINDOWED)
			  || (driver_list_entry->id == GFX_AUTODETECT_FULLSCREEN));

   /* Start with zero modes. */
   driver_list_entry->mode_count = 0;

   /* We use the default mode list in two cases:
    *  - the driver is GFX_AUTODETECT*,
    *  - the driver doesn't support get_gfx_mode_list().
    */
   if (is_autodetected || !(gfx_mode_list = get_gfx_mode_list(driver_list_entry->id))) {
      /* Simply return the default mode list if we can. */
      if (!filter) {
	 driver_list_entry->mode_count = sizeof(default_mode_list) / sizeof(MODE_LIST) - 1;
	 driver_list_entry->mode_list = default_mode_list;
	 driver_list_entry->mode_list_owned = FALSE;
	 return 0;
      }

      /* Build mode list from the default list. */
      for (mode = 0; default_mode_list[mode].w; mode++) {
	 w = default_mode_list[mode].w;
	 h = default_mode_list[mode].h;

	 for (n = 0; n < N_COLOR_DEPTH; n++) {
	    bpp = bpp_value(n);

	    if (filter(driver_list_entry->id, w, h, bpp) == 0) {
	        if (add_mode(&temp_mode_list, &driver_list_entry->mode_count, w, h, bpp) != 0)
		  return -1;
	    }
	 }
      }

      /* We should not terminate the list if the list is empty since the caller
       * of this function will simply discard this driver_list entry in that
       * case.
       */
      if (driver_list_entry->mode_count > 0) {
	 if (terminate_list(&temp_mode_list, driver_list_entry->mode_count) != 0)
	    return -1;
      }

      driver_list_entry->mode_list = temp_mode_list;
      driver_list_entry->mode_list_owned = TRUE;
      return 0;
   }

   /* Build mode list from the fetched list. */
   for (gfx_mode_entry = gfx_mode_list->mode; gfx_mode_entry->width; gfx_mode_entry++) {
      w = gfx_mode_entry->width;
      h = gfx_mode_entry->height;
      bpp = gfx_mode_entry->bpp;

      if (!filter || filter(driver_list_entry->id, w, h, bpp) == 0) {
	 if (add_mode(&temp_mode_list, &driver_list_entry->mode_count, w, h, bpp) != 0) {
	    destroy_gfx_mode_list(gfx_mode_list);
	    return -1;
	 }
      } 
   }

   if (driver_list_entry->mode_count > 0) {
      if (terminate_list(&temp_mode_list, driver_list_entry->mode_count) != 0) {
	 destroy_gfx_mode_list(gfx_mode_list);
	 return -1;
      }
   }

   driver_list_entry->mode_list = temp_mode_list;
   driver_list_entry->mode_list_owned = TRUE;
   destroy_gfx_mode_list(gfx_mode_list);
   return 0;
}



/* create_driver_list:
 *  Fills the list of video cards with info about the available drivers.
 *  Returns -1 on failure.
 */
static int create_driver_list(FILTER_FUNCTION filter)
{
   _DRIVER_INFO *driver_info;
   GFX_DRIVER *gfx_driver;
   int i;
   int list_pos;

   if (system_driver->gfx_drivers)
      driver_info = system_driver->gfx_drivers();
   else
      driver_info = _gfx_driver_list;

   driver_list = _AL_MALLOC(sizeof(DRIVER_LIST) * 3);
   if (!driver_list) return -1;

   list_pos = 0;

   driver_list[list_pos].id = GFX_AUTODETECT;
   ustrzcpy(driver_list[list_pos].name, DRVNAME_SIZE, get_config_text("Autodetect"));
   create_mode_list(&driver_list[list_pos], filter);
   if (driver_list[list_pos].mode_count > 0)
      list_pos++;

   driver_list[list_pos].id = GFX_AUTODETECT_FULLSCREEN;
   ustrzcpy(driver_list[list_pos].name, DRVNAME_SIZE, get_config_text("Auto fullscreen"));
   create_mode_list(&driver_list[list_pos], filter);
   if (driver_list[list_pos].mode_count > 0)
      list_pos++;

   driver_list[list_pos].id = GFX_AUTODETECT_WINDOWED;
   ustrzcpy(driver_list[list_pos].name, DRVNAME_SIZE, get_config_text("Auto windowed"));
   create_mode_list(&driver_list[list_pos], filter);
   if (driver_list[list_pos].mode_count > 0)
      list_pos++;

   for (i = 0; driver_info[i].driver; i++) {
      driver_list = _al_sane_realloc(driver_list, sizeof(DRIVER_LIST) * (list_pos + 1));
      if (!driver_list) return -1;
      driver_list[list_pos].id = driver_info[i].id;
      gfx_driver = driver_info[i].driver;
      do_uconvert(gfx_driver->ascii_name, U_ASCII, driver_list[list_pos].name, U_CURRENT, DRVNAME_SIZE);

      create_mode_list(&driver_list[list_pos], filter);

      if (driver_list[list_pos].mode_count > 0) {
	 list_pos++;
      }
      else {
	 ASSERT(driver_list[list_pos].mode_list == NULL);
      }
   }

   /* update global variable */
   driver_count = list_pos;

   return 0;
}



/* destroy_driver_list:
 *  Frees allocated memory used by driver lists and mode lists.
 */
static void destroy_driver_list(void)
{
   int driver;

   for (driver=0; driver < driver_count; driver++) {
      if (driver_list[driver].mode_list_owned)
         _AL_FREE(driver_list[driver].mode_list);
   }

   _AL_FREE(driver_list);
   driver_list = NULL;
   driver_count = 0;
}



/* gfx_card_getter:
 *  Listbox data getter routine for the graphics card list.
 */
static AL_CONST char *gfx_card_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
         *list_size = driver_count;
      return NULL;
   }

   return driver_list[index].name;
}



/* gfx_mode_getter:
 *  Listbox data getter routine for the graphics mode list.
 */
static AL_CONST char *gfx_mode_getter(int index, int *list_size)
{
   int entry;
   char tmp[32];

   entry = what_dialog[GFX_DRIVERLIST].d1;

   if (index < 0) {
      if (list_size) {
         *list_size = driver_list[entry].mode_count;
         return NULL;
      }
   }

   uszprintf(mode_string, sizeof(mode_string), uconvert_ascii("%ix%i", tmp),
             driver_list[entry].mode_list[index].w, driver_list[entry].mode_list[index].h);

   return mode_string;
}



/* gfx_depth_getter:
 *  Listbox data getter routine for the color depth list.
 */
static AL_CONST char *gfx_depth_getter(int index, int *list_size)
{
   static char *bpp_string_list[N_COLOR_DEPTH] = {"256", "32K", "64K", "16M", "16M"};
   MODE_LIST *mode;
   int card_entry, mode_entry, bpp_entry, bpp_count, bpp_index;
   char tmp[128];

   card_entry = what_dialog[GFX_DRIVERLIST].d1;
   mode_entry = what_dialog[GFX_MODELIST].d1;
   mode = &driver_list[card_entry].mode_list[mode_entry];

   if (index < 0) {
      if (list_size) {
         /* Count the number of BPP entries for the mode. */
         for (bpp_count = 0, bpp_entry = 0; bpp_entry < N_COLOR_DEPTH; bpp_entry++) {
            if (mode->has_bpp[bpp_entry])
               bpp_count++;
         }

         *list_size = bpp_count;
         return NULL;
      }
   }

   /* Find the BPP entry for the mode corresponding to the zero-based index. */
   bpp_index = -1;
   bpp_entry = -1;
   while (bpp_index < index) {
      if (mode->has_bpp[++bpp_entry])
         bpp_index++;
   }

   uszprintf(mode_string, sizeof(mode_string), uconvert_ascii("%2d ", tmp), bpp_value(bpp_entry));
   ustrzcat(mode_string, sizeof(mode_string), get_config_text("bpp"));
   ustrzcat(mode_string, sizeof(mode_string), uconvert_ascii(" (", tmp));
   ustrzcat(mode_string, sizeof(mode_string), uconvert_ascii(bpp_string_list[bpp_entry], tmp));
   ustrzcat(mode_string, sizeof(mode_string), uconvert_ascii(" ", tmp));
   ustrzcat(mode_string, sizeof(mode_string), get_config_text("colors"));
   ustrzcat(mode_string, sizeof(mode_string), uconvert_ascii(")", tmp));
 
   return mode_string;
}



/* gfx_mode_select_filter:
 *  Extended version of the graphics mode selection dialog, which allows the 
 *  user to select the color depth as well as the resolution and hardware 
 *  driver. An optional filter can be passed to check whether a particular
 *  entry should be displayed or not. Initial values for the selections may be
 *  given at the addresses passed to the function, and the user's selection
 *  will be stored at those addresses if the dialog is OK'd. Initial values
 *  at the addresses passed to the function may be set to 0 or -1 to indicate
 *  not to search for the values but to default to the first entries in each 
 *  selection list.
 *  In the case of an error, *card is set to GFX_NONE and FALSE is returned.
 *  In the case that the filter filtered out all of the modes, *card is set to
 *  GFX_NONE and TRUE is returned.
 */
int gfx_mode_select_filter(int *card, int *w, int *h, int *color_depth, FILTER_FUNCTION filter)
{
   int i, ret, what_driver, what_mode, what_bpp, extd;
   MODE_LIST* mode_iter;
   
   ASSERT(card);
   ASSERT(w);
   ASSERT(h);

   clear_keybuf();

   extd = color_depth ? TRUE : FALSE;

   while (gui_mouse_b());

   what_dialog = extd ? gfx_mode_ex_dialog : gfx_mode_dialog;

   what_dialog[GFX_TITLE].dp = (void*)get_config_text("Graphics Mode");
   what_dialog[GFX_OK].dp = (void*)get_config_text("OK");
   what_dialog[GFX_CANCEL].dp = (void*)get_config_text("Cancel");

   ret = create_driver_list(filter);

   if (ret == -1) {
      *card = GFX_NONE;
      return FALSE;
   }

   if (!driver_count) {
      *card = GFX_NONE;
      return TRUE;
   }

   /* The data stored at the addresses passed to this function is used to
    * search for initial selections in the dialog lists. If the requested
    * entries are not found, default to the first selection in each list in
    * order of the driver, the mode w/h, and also the color depth when in
    * extended mode (called directly or from gfx_mode_select_ex).
    */
   /* Check for the user suggested driver first */
   what_driver = 0;/* Default to first entry if not found */
   /* Don't search for initial values if *card is 0 or -1 */
   if (!((*card == 0) || (*card == -1))) {
      for (i = 0 ; i < driver_count ; ++i) {
         if (driver_list[i].id == *card) {
            what_driver = i;
         }
      }
   }
   what_dialog[GFX_DRIVERLIST].d1 = what_driver;
   what_dialog[GFX_CHANGEPROC].d1 = what_driver;
   
   /* Check for suggested resolution dimensions second */
   what_mode = 0;/* Default to first entry if not found */
   mode_iter = &(driver_list[what_driver].mode_list[0]);
   /* Don't search for initial values if *w or *h is 0 or -1 */
   if (!(((*w == 0) || (*w == -1)) || ((*h == 0) || (*h == -1)))) {
      for (i = 0 ; i < driver_list[what_driver].mode_count ; ++i) {
         if ((mode_iter->w == *w) && (mode_iter->h == *h)) {
            what_mode = i;
            break;
         }
         ++mode_iter;
      }
   }
   what_dialog[GFX_MODELIST].d1 = what_mode;
   what_dialog[GFX_CHANGEPROC].d2 = what_mode;
   
   /* Check for suggested color depth when in extended mode */
   if (extd) {
      what_bpp = 0;
      /* Don't search for initial values if *color_depth is 0 or -1 */
      if (!((*color_depth == 0) || (*color_depth == -1))) {
         what_bpp = bpp_index_for_mode(*color_depth , what_driver , what_mode);
         if (what_bpp < 0) {what_bpp = 0;} /* Default to first entry if not found */
      }
      what_dialog[GFX_DEPTHLIST].d1 = what_bpp;
   }

   centre_dialog(what_dialog);
   set_dialog_color(what_dialog, gui_fg_color, gui_bg_color);
   ret = popup_dialog(what_dialog, GFX_DRIVERLIST);

   what_driver = what_dialog[GFX_DRIVERLIST].d1;
   what_mode = what_dialog[GFX_MODELIST].d1;

   if (extd)
      what_bpp = what_dialog[GFX_DEPTHLIST].d1;
   else
      what_bpp = 0;

   if (ret != GFX_CANCEL) {
      *card = driver_list[what_driver].id;
      *w = driver_list[what_driver].mode_list[what_mode].w;
      *h = driver_list[what_driver].mode_list[what_mode].h;
      if (extd) {
         *color_depth = bpp_value_for_mode(what_bpp, what_driver, what_mode);
      }
   }

   destroy_driver_list();

   if (ret == GFX_CANCEL)
      return FALSE;
   else 
      return TRUE;
}



/* gfx_mode_select_ex:
 *  Extended version of the graphics mode selection dialog, which allows the
 *  user to select the color depth as well as the resolution and hardware
 *  driver. Initial values for the selections will be looked for at the
 *  addresses passed to the function, and selections will be stored there if
 *  the user does not cancel the dialog. See gfx_mode_select_filter for
 *  details and return values.
 */
int gfx_mode_select_ex(int *card, int *w, int *h, int *color_depth)
{
   ASSERT(card);
   ASSERT(w);
   ASSERT(h);
   ASSERT(color_depth);
   return gfx_mode_select_filter(card, w, h, color_depth, NULL);
}



/* gfx_mode_select:
 *  Displays the Allegro graphics mode selection dialog, which allows the
 *  user to select a screen mode and graphics card. Initial values for the
 *  selection will be looked for at the addresses passed to the function, and
 *  the selection will be stored in the three variables if the dialog is not
 *  cancelled. See gfx_mode_select_filter for details and return values.
 */
int gfx_mode_select(int *card, int *w, int *h)
{
   ASSERT(card);
   ASSERT(w);
   ASSERT(h);
   return gfx_mode_select_filter(card, w, h, NULL, NULL);
}


