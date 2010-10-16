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
 *      Compiled sprite routines for the i386.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

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



/* compile_sprite:
 *  Helper function for making compiled sprites.
 */
static void *compile_sprite(BITMAP *b, int l, int planar, int *len)
{
   int x, y;
   int offset;
   int run;
   int run_pos;
   int compiler_pos = 0;
   int xc = planar ? 4 : 1;
   unsigned long *addr;

   #ifdef ALLEGRO_COLOR16
      unsigned short *p16;
   #endif

   #if defined ALLEGRO_COLOR24 || defined ALLEGRO_COLOR32
      unsigned long *p32;
   #endif
   
   #ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF
      /* make sure we get a new map */
      _map_size = 0;
   #endif

   for (y=0; y<b->h; y++) {

      /* for linear bitmaps, time for some bank switching... */
      if (!planar) {
	 COMPILER_MOV_EDI_EAX();
	 COMPILER_CALL_ESI();
	 COMPILER_ADD_ECX_EAX();
      }

      offset = 0;
      x = l;

      /* compile a line of the sprite */
      while (x < b->w) {

	 switch (bitmap_color_depth(b)) {


	 #ifdef ALLEGRO_COLOR8

	    case 8:
	       /* 8 bit version */
	       if (b->line[y][x]) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (b->line[y][run_pos])) {
		     run++;
		     run_pos += xc;
		  }

		  while (run >= 4) {
		     COMPILER_MOVL_IMMED(offset, ((int)b->line[y][x]) |
						 ((int)b->line[y][x+xc] << 8) |
						 ((int)b->line[y][x+xc*2] << 16) |
						 ((int)b->line[y][x+xc*3] << 24));
		     x += xc*4;
		     offset += 4;
		     run -= 4;
		  }

		  if (run >= 2) {
		     COMPILER_MOVW_IMMED(offset, ((int)b->line[y][x]) |
						 ((int)b->line[y][x+xc] << 8));
		     x += xc*2;
		     offset += 2;
		     run -= 2;
		  }

		  if (run > 0) {
		     COMPILER_MOVB_IMMED(offset, b->line[y][x]);
		     x += xc;
		     offset++;
		  }
	       }
	       else {
		  x += xc;
		  offset++;
	       }
	       break;

	 #endif


	 #ifdef ALLEGRO_COLOR16

	    case 15:
	    case 16:
	       /* 16 bit version */
	       p16 = (unsigned short *)b->line[y];

	       if (p16[x] != b->vtable->mask_color) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (p16[run_pos] != b->vtable->mask_color)) {
		     run++;
		     run_pos++;
		  }

		  while (run >= 2) {
		     COMPILER_MOVL_IMMED(offset, ((int)p16[x]) |
						 ((int)p16[x+1] << 16));
		     x += 2;
		     offset += 4;
		     run -= 2;
		  }

		  if (run > 0) {
		     COMPILER_MOVW_IMMED(offset, p16[x]);
		     x++;
		     offset += 2;
		     run--;
		  }
	       }
	       else {
		  x++;
		  offset += 2;
	       }
	       break;

	 #endif


	 #ifdef ALLEGRO_COLOR24

	    case 24:
	       /* 24 bit version */
	       p32 = (unsigned long *)b->line[y];

	       if (((*((unsigned long *)(((unsigned char *)p32)+x*3)))&0xFFFFFF) != (unsigned)b->vtable->mask_color) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (((*((unsigned long *)(((unsigned char *)p32)+run_pos*3)))&0xFFFFFF) != (unsigned)b->vtable->mask_color)) {
		     run++;
		     run_pos++;
		  }

		  while (run >= 4) {
		     addr = ((unsigned long *)(((unsigned char *)p32)+x*3));
		     COMPILER_MOVL_IMMED(offset, addr[0]);
		     offset += 4;
		     COMPILER_MOVL_IMMED(offset, addr[1]);
		     offset += 4;
		     COMPILER_MOVL_IMMED(offset, addr[2]);
		     offset += 4;
		     x += 4;
		     run -= 4;
		  }

		  switch (run) {

		     case 3:
			addr = ((unsigned long *)(((unsigned char *)p32)+x*3));
			COMPILER_MOVL_IMMED(offset, addr[0]);
			offset += 4;
			COMPILER_MOVL_IMMED(offset, addr[1]);
			offset += 4;
			COMPILER_MOVB_IMMED(offset, *(((unsigned short *)addr)+4));
			offset++;
			x += 3;
			run -= 3;
			break;

		     case 2:
			addr = ((unsigned long *)(((unsigned char *)p32)+x*3));
			COMPILER_MOVL_IMMED(offset, addr[0]);
			offset += 4;
			COMPILER_MOVW_IMMED(offset, *(((unsigned short *)addr)+2));
			offset += 2;
			x += 2;
			run -= 2;
			break;

		     case 1:
			addr = ((unsigned long *)((unsigned char *)p32+x*3));
			COMPILER_MOVW_IMMED(offset, (unsigned short )addr[0]);
			offset += 2;
			COMPILER_MOVB_IMMED(offset, *((unsigned char *)(((unsigned short *)addr)+1)));
			offset++;
			x++;
			run--;
			break;

		     default:
			break;
		  } 
	       }
	       else {
		  x++;
		  offset += 3;
	       }
	       break;

	 #endif


	 #ifdef ALLEGRO_COLOR32

	    case 32:
	       /* 32 bit version */
	       p32 = (unsigned long *)b->line[y];

	       if (p32[x] != (unsigned)b->vtable->mask_color) {
		  run = 0;
		  run_pos = x;

		  while ((run_pos<b->w) && (p32[run_pos] != (unsigned)b->vtable->mask_color)) {
		     run++;
		     run_pos++;
		  }

		  while (run > 0) {
		     COMPILER_MOVL_IMMED(offset, p32[x]);
		     x++;
		     offset += 4;
		     run--;
		  }
	       }
	       else {
		  x++;
		  offset += 4;
	       }
	       break;

	 #endif

	 }
      }

      /* move on to the next line */
      if (y+1 < b->h) {
	 if (planar) {
	    COMPILER_ADD_ECX_EAX();
	 }
	 else {
	    COMPILER_INC_EDI();
	 }
      }
   }

   COMPILER_RET();

#ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF

   /* Lie about the size, return the size mapped which >= size used /
    * compiler_pos, because we need the size mapped for munmap.
    */
   *len = _map_size;
   /* we don't need _rw_map or _map_fd anymore */
   munmap(_rw_map, _map_size);
   close(_map_fd);
   return _exec_map;

#else
   {
      void *p = _AL_MALLOC(compiler_pos);
      if (p) {
	 memcpy(p, _scratch_mem, compiler_pos);
	 *len = compiler_pos;
	 #ifdef ALLEGRO_WINDOWS
	 {
	    DWORD old_protect;
	    /* Play nice with Windows executable memory protection */
	    VirtualProtect(p, compiler_pos, PAGE_EXECUTE_READWRITE, &old_protect);
	 }
	 #elif defined(ALLEGRO_HAVE_MPROTECT)
	 {
	    long page_size = _unix_get_page_size();
	    char *aligned_p = (char *)((unsigned long)p & ~(page_size-1ul));
	    if (mprotect(aligned_p, compiler_pos + ((char *)p - aligned_p),
		  PROT_EXEC|PROT_READ|PROT_WRITE)) {
	       perror("allegro-error: mprotect failed during compile sprite!");
	       _AL_FREE(p);
	       return NULL;
	    }
	 }
	 #endif
      }

      return p;
   }
#endif
}



/* get_compiled_sprite:
 *  Creates a compiled sprite based on the specified bitmap.
 */
COMPILED_SPRITE *get_compiled_sprite(BITMAP *bitmap, int planar)
{
   COMPILED_SPRITE *s;
   int plane;

   s = _AL_MALLOC(sizeof(COMPILED_SPRITE));
   if (!s)
      return NULL;

   s->planar = planar;
   s->color_depth = bitmap_color_depth(bitmap);
   s->w = bitmap->w;
   s->h = bitmap->h;

   for (plane=0; plane<4; plane++) {
      s->proc[plane].draw = NULL;
      s->proc[plane].len = 0;
   }

   for (plane=0; plane < (planar ? 4 : 1); plane++) {
      s->proc[plane].draw = compile_sprite(bitmap, plane, planar, &s->proc[plane].len);

      if (!s->proc[plane].draw) {
	 destroy_compiled_sprite(s);
	 return NULL;
      }
   }

   return s;
}



/* destroy_compiled_sprite:
 *  Destroys a compiled sprite structure returned by get_compiled_sprite().
 */
void destroy_compiled_sprite(COMPILED_SPRITE *sprite)
{
   int plane;

   if (sprite) {
      for (plane=0; plane<4; plane++) {
	 if (sprite->proc[plane].draw) {
	    #ifdef ALLEGRO_USE_MMAP_GEN_CODE_BUF
	       munmap(sprite->proc[plane].draw, sprite->proc[plane].len);
	    #else
	       _AL_FREE(sprite->proc[plane].draw);
	    #endif
	 }
      }
      _AL_FREE(sprite);
   }
}

