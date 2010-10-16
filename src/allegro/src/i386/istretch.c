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
 *      Bitmap stretching functions for the i386.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "opcodes.h"

#ifdef ALLEGRO_UNIX
   #include "allegro/platform/aintunix.h"   /* for _unix_get_page_size */
#endif

#ifdef ALLEGRO_WINDOWS
   #include "winalleg.h"   /* For VirtualProtect */
#endif     /* ifdef ALLEGRO_WINDOWS */



#ifdef ALLEGRO_HAVE_MPROTECT
   #include <sys/types.h>
   #include <sys/mman.h>
#endif     /* ifdef ALLEGRO_HAVE_MPROTECT */



/* helper macro for generating stretchers in each color depth */
#define DECLARE_STRETCHER(sz, mask, scale)                                   \
{                                                                            \
   int x, x2;                                                                \
   int c;                                                                    \
									     \
   if (sxd == itofix(1)) {          /* easy case for 1 -> 1 scaling */       \
      if (masked) {                                                          \
	 for (c=0; c<dest_width; c++) {                                      \
	    COMPILER_LODS##sz();                                             \
	    COMPILER_MASKED_STOS##sz(mask);                                  \
	 }                                                                   \
      }                                                                      \
      else {                                                                 \
	 COMPILER_MOV_ECX(dest_width);                                       \
	 COMPILER_REP_MOVS##sz();                                            \
      }                                                                      \
   }                                                                         \
   else if (sxd > itofix(1)) {      /* big -> little scaling */              \
      for (x=0; x<dest_width; x++) {                                         \
	 COMPILER_LODS##sz();                                                \
	 if (masked) {                                                       \
	    COMPILER_MASKED_STOS##sz(mask);                                  \
	 }                                                                   \
	 else {                                                              \
	    COMPILER_STOS##sz();                                             \
	 }                                                                   \
	 x2 = (sx >> 16) + 1;                                                \
	 sx += sxd;                                                          \
	 x2 = (sx >> 16) - x2;                                               \
	 if (x2*scale > 1) {                                                 \
	    COMPILER_ADD_ESI(x2*scale);                                      \
	 }                                                                   \
	 else if (x2*scale == 1) {                                           \
	    COMPILER_INC_ESI();                                              \
	 }                                                                   \
      }                                                                      \
   }                                                                         \
   else {                           /* little -> big scaling */              \
      x2 = sx >> 16;                                                         \
      COMPILER_LODS##sz();                                                   \
      for (x=0; x<dest_width; x++) {                                         \
	 if (masked) {                                                       \
	    COMPILER_MASKED_STOS##sz(mask);                                  \
	 }                                                                   \
	 else {                                                              \
	    COMPILER_STOS##sz();                                             \
	 }                                                                   \
	 sx += sxd;                                                          \
	 if ((sx >> 16) > x2) {                                              \
	    COMPILER_LODS##sz();                                             \
	    x2++;                                                            \
	 }                                                                   \
      }                                                                      \
   }                                                                         \
}



/* make_stretcher_256:
 *  Helper for constructing a 256 color stretcher routine.
 */
static int make_stretcher_256(int compiler_pos, fixed sx, fixed sxd, int dest_width, int masked)
{
   #ifdef ALLEGRO_COLOR8

      DECLARE_STRETCHER(B, mask, 1);

   #endif

   return compiler_pos;
}



/* make_stretcher_15:
 *  Helper for constructing a 15 bit stretcher routine.
 */
static int make_stretcher_15(int compiler_pos, fixed sx, fixed sxd, int dest_width, int masked)
{
   #ifdef ALLEGRO_COLOR16

      DECLARE_STRETCHER(W, MASK_COLOR_15, 2);

   #endif

   return compiler_pos;
}



/* make_stretcher_16:
 *  Helper for constructing a 16 bit stretcher routine.
 */
static int make_stretcher_16(int compiler_pos, fixed sx, fixed sxd, int dest_width, int masked)
{
   #ifdef ALLEGRO_COLOR16

      DECLARE_STRETCHER(W, MASK_COLOR_16, 2);

   #endif

   return compiler_pos;
}



/* make_stretcher_24:
 *  Helper for constructing a 24 bit stretcher routine.
 */
static int make_stretcher_24(int compiler_pos, fixed sx, fixed sxd, int dest_width, int masked)
{
   #ifdef ALLEGRO_COLOR24

      DECLARE_STRETCHER(L2, MASK_COLOR_24, 3);

   #endif

   return compiler_pos;
}



/* make_stretcher_32:
 *  Helper for constructing a 32 bit stretcher routine.
 */
static int make_stretcher_32(int compiler_pos, fixed sx, fixed sxd, int dest_width, int masked)
{
   #ifdef ALLEGRO_COLOR32

      DECLARE_STRETCHER(L, MASK_COLOR_32, 4);

   #endif

   return compiler_pos;
}



/* make_stretcher:
 *  Helper function for stretch_blit(). Builds a machine code stretching
 *  routine in the scratch memory area.
 */
static int make_stretcher(int compiler_pos, fixed sx, fixed sxd, int dest_width, int masked, int color_depth)
{
   if (dest_width > 0) {

      switch (color_depth) {

	 case 8:
	    return make_stretcher_256(compiler_pos, sx, sxd, dest_width, masked);

	 case 15:
	    return make_stretcher_15(compiler_pos, sx, sxd, dest_width, masked);

	 case 16:
	    return make_stretcher_16(compiler_pos, sx, sxd, dest_width, masked);

	 case 24:
	    return make_stretcher_24(compiler_pos, sx, sxd, dest_width, masked);

	 case 32:
	    return make_stretcher_32(compiler_pos, sx, sxd, dest_width, masked);
      }
   }

   return compiler_pos;
}



/* cache of previously constructed stretcher functions */
typedef struct STRETCHER_INFO
{
   fixed sx;
   fixed sxd;
   short dest_width;
   char depth;
   char flags;
   int lru;
   void *exec; /* xr_ mapping in the mmap case, normally both in one. */
   int size;
#ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF
   void *rw;   /* _rw mapping for the mmap case. */
   int fd;     /* mapping backing fd for the mmap case. */
#endif
} STRETCHER_INFO;


#define NUM_STRETCHERS  8


static STRETCHER_INFO stretcher_info[NUM_STRETCHERS];
static int stretcher_count = 0;

static int stretcher_virgin = TRUE;



/* free_stretchers:
 * Clean up atexit.
 */
static void free_stretchers(void)
{
   int i;

   for (i=0; i<NUM_STRETCHERS; i++)
      if (stretcher_info[i].exec != NULL) {
	 #ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF
	    munmap(stretcher_info[i].exec, stretcher_info[i].size);
	    munmap(stretcher_info[i].rw, stretcher_info[i].size);
	    close(stretcher_info[i].fd);
	 #else
	    _AL_FREE(stretcher_info[i].exec);
	 #endif
      }

   _remove_exit_func(free_stretchers);

   stretcher_virgin = TRUE;
}



/* do_stretch_blit:
 *  Like blit(), except it can scale images so the source and destination 
 *  rectangles don't need to be the same size. This routine doesn't do as 
 *  much safety checking as the regular blit: in particular you must take 
 *  care not to copy from areas outside the source bitmap, and you cannot 
 *  blit between overlapping regions, ie. you must use different bitmaps for 
 *  the source and the destination. This function can draw onto both linear
 *  and mode-X bitmaps.
 *
 *  This routine does some very dodgy stuff. It dynamically generates a
 *  chunk of machine code to scale a line of the bitmap, and then calls this. 
 *  I just _had_ to use self modifying code _somewhere_ in Allegro :-) 
 */
static void do_stretch_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int source_width, int source_height, int dest_x, int dest_y, int dest_width, int dest_height, int masked)
{
 #ifdef ALLEGRO_WINDOWS
   DWORD old_protect;
 #endif     /* ifdef ALLEGRO_WINDOWS */
 #ifndef ALLEGRO_USE_MMAP_GEN_CODE_BUF
   void *prev_scratch_mem;
   int prev_scratch_mem_size;
 #endif
   fixed sx, sy, sxd, syd;
   int compiler_pos = 0;
   int best, best_lru;
   char flags;
   int i;

   /* vtable hook; not called if dest is a memory surface */   
   if (source->vtable->do_stretch_blit && !is_memory_bitmap(dest)) {
      source->vtable->do_stretch_blit(source, dest, source_x, source_y, source_width, source_height, dest_x, dest_y, dest_width, dest_height, masked);
      return;
   }

   /* trivial reject for zero sizes */
   if ((source_width <= 0) || (source_height <= 0) || 
       (dest_width <= 0) || (dest_height <= 0))
      return;

   /* make sure all allocated memory is freed atexit */
   if (stretcher_virgin) {
      stretcher_virgin = FALSE;
      memset(stretcher_info, 0, sizeof(stretcher_info));
      _add_exit_func(free_stretchers, "free_stretchers");
   }

   /* convert to fixed point */
   sx = itofix(source_x);
   sy = itofix(source_y);

   /* calculate delta values */
   sxd = itofix(source_width) / dest_width;
   syd = itofix(source_height) / dest_height;

   /* clip? */
   if (dest->clip) {
      while (dest_x < dest->cl) {
	 dest_x++;
	 dest_width--;
	 sx += sxd;
      }

      while (dest_y < dest->ct) {
	 dest_y++;
	 dest_height--;
	 sy += syd;
      }

      if (dest_x+dest_width > dest->cr)
	 dest_width = dest->cr - dest_x;

      if (dest_y+dest_height > dest->cb)
	 dest_height = dest->cb - dest_y;

      if ((dest_width <= 0) || (dest_height <= 0))
	 return;
   }

   /* compensate for a problem where the first column or row can get
    * an extra pixel than it should do for little -> big scaling, due
    * to fixed point number representation imprecisions
    */
   if (sxd < itofix(1))
      sx += (sxd >> 1);
   if (syd < itofix(1))
      sy += (syd >> 1);

   /* search the cache */
   stretcher_count++;
   if (stretcher_count <= 0) {
      stretcher_count = 1;
      for (i=0; i<NUM_STRETCHERS; i++)
	 stretcher_info[i].lru = 0;
   }

   if (is_linear_bitmap(dest))
      flags = masked;
   else
      flags = masked | 2 | ((dest_x&3)<<2);

   best = 0;
   best_lru = INT_MAX;

   for (i=0; i<NUM_STRETCHERS; i++) {
      if ((stretcher_info[i].sx == sx) &&
	  (stretcher_info[i].sxd == sxd) &&
	  (stretcher_info[i].dest_width == dest_width) &&
	  (stretcher_info[i].depth == dest->vtable->color_depth) &&
	  (stretcher_info[i].flags == flags)) {
	 /* use a previously generated routine */
	 if (stretcher_info[i].flags & 2)
	    dest_x >>= 2;
	 _do_stretch(source, dest, stretcher_info[i].exec, sx>>16, sy, syd,
		     dest_x, dest_y, dest_height, dest->vtable->color_depth);
	 stretcher_info[i].lru = stretcher_count;
	 return;
      }
      else {
	 /* decide which cached routine to discard */
	 if (stretcher_info[i].lru < best_lru) {
	    best = i;
	    best_lru = stretcher_info[i].lru;
	 }
      }
   }

 #ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF
   _exec_map = stretcher_info[best].exec;
   _rw_map = stretcher_info[best].rw;
   _map_size = stretcher_info[best].size;
   _map_fd = stretcher_info[best].fd;
 #else
   prev_scratch_mem = _scratch_mem;
   prev_scratch_mem_size = _scratch_mem_size;

   _scratch_mem = stretcher_info[best].exec;
   _scratch_mem_size = stretcher_info[best].size;
 #endif

   if (is_linear_bitmap(dest)) { 
      /* build a simple linear stretcher */
      compiler_pos = make_stretcher(0, sx, sxd, dest_width, masked, dest->vtable->color_depth);
   }

 #ifdef ALLEGRO_GFX_HAS_VGA

   else { 
      int plane, d;

      /* build four stretchers, one for each mode-X plane */
      for (plane=0; plane<4; plane++) {
	 COMPILER_PUSH_ESI();
	 COMPILER_PUSH_EDI();

	 COMPILER_MOV_EAX((0x100<<((dest_x+plane)&3))|2);
	 COMPILER_MOV_EDX(0x3C4);
	 COMPILER_OUTW();

	 compiler_pos = make_stretcher(compiler_pos, sx+sxd*plane, sxd<<2,
				       (dest_width-plane+3)>>2, masked, 8);

	 COMPILER_POP_EDI();
	 COMPILER_POP_ESI();

	 if (((dest_x+plane) & 3) == 3) {
	    COMPILER_INC_EDI();
	 }

	 d = ((sx+sxd*(plane+1))>>16) - ((sx+sxd*plane)>>16);
	 if (d > 0) {
	    COMPILER_ADD_ESI(d);
	 }
      }

      dest_x >>= 2;
   }

 #endif     /* ifdef ALLEGRO_GFX_HAS_VGA */

   COMPILER_RET();

 /* Play nice with executable memory protection: VirtualProtect on Windows,
  * mprotect on UNIX based systems.
  */
 #ifdef ALLEGRO_WINDOWS
   VirtualProtect(_scratch_mem, _scratch_mem_size, PAGE_EXECUTE_READWRITE, &old_protect);
 #elif defined(ALLEGRO_HAVE_MPROTECT) && !defined(ALLEGRO_USE_MMAP_GEN_CODE_BUF)
   {
      char *p = (char *)((uintptr_t)_scratch_mem & ~(_unix_get_page_size() - 1ul));
      if (mprotect(p, _scratch_mem_size + ((char *)_scratch_mem - p),
            PROT_EXEC|PROT_READ|PROT_WRITE))
         perror("allegro-error: mprotect failed during stretched blit!");
   }
 #endif

   /* store it in the cache */
   stretcher_info[best].sx = sx;
   stretcher_info[best].sxd = sxd;
   stretcher_info[best].dest_width = dest_width;
   stretcher_info[best].depth = dest->vtable->color_depth;
   stretcher_info[best].flags = flags;
   stretcher_info[best].lru = stretcher_count;
 #ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF
   stretcher_info[best].exec = _exec_map;
   stretcher_info[best].rw = _rw_map;
   stretcher_info[best].size = _map_size;
   stretcher_info[best].fd = _map_fd;
 #else
   stretcher_info[best].exec = _scratch_mem;
   stretcher_info[best].size = _scratch_mem_size;

   _scratch_mem = prev_scratch_mem;
   _scratch_mem_size = prev_scratch_mem_size;
 #endif

   /* and call the stretcher */
   _do_stretch(source, dest, stretcher_info[best].exec, sx>>16, sy, syd, 
	       dest_x, dest_y, dest_height, dest->vtable->color_depth);
}



/* stretch_blit:
 *  Opaque bitmap scaling function.
 */
void stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h)
{
   do_stretch_blit(s, d, s_x, s_y, s_w, s_h, d_x, d_y, d_w, d_h, 0);
}



/* masked_stretch_blit:
 *  Masked bitmap scaling function.
 */
void masked_stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h)
{
   do_stretch_blit(s, d, s_x, s_y, s_w, s_h, d_x, d_y, d_w, d_h, 1);
}



/* stretch_sprite:
 *  Masked version of stretch_blit().
 */
void stretch_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int w, int h)
{
   do_stretch_blit(sprite, bmp, 0, 0, sprite->w, sprite->h, x, y, w, h, 1); 
}
