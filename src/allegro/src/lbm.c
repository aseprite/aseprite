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
 *      LBM reader.
 *
 *      By Adrian Oboroc.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* load_lbm:
 *  Loads IFF ILBM/PBM files with up to 8 bits per pixel, returning
 *  a bitmap structure and storing the palette data in the specified
 *  palette (this should be an array of at least 256 RGB structures).
 */
BITMAP *load_lbm(AL_CONST char *filename, RGB *pal)
{
   #define IFF_FORM     0x4D524F46     /* 'FORM' - IFF FORM structure  */
   #define IFF_ILBM     0x4D424C49     /* 'ILBM' - interleaved bitmap  */
   #define IFF_PBM      0x204D4250     /* 'PBM ' - new DP2e format     */
   #define IFF_BMHD     0x44484D42     /* 'BMHD' - bitmap header       */
   #define IFF_CMAP     0x50414D43     /* 'CMAP' - color map (palette) */
   #define IFF_BODY     0x59444F42     /* 'BODY' - bitmap data         */

   PACKFILE *f;
   PALETTE tmppal;
   BITMAP *b = NULL;
   int w, h, i, x, y, bpl, ppl, pbm_mode;
   char ch, cmp_type, bit_plane, color_depth;
   unsigned char uc, check_flags, bit_mask, *line_buf;
   long data_id, len, l;
   int dest_depth = _color_load_depth(8, FALSE);
   ASSERT(filename);

   if (!pal)
      pal = tmppal;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   data_id = pack_igetl(f);         /* read file header    */
   if (data_id != IFF_FORM) {       /* check for 'FORM' id */
      pack_fclose(f);
      return NULL;
   }

   pack_igetl(f);                   /* skip FORM length    */

   data_id = pack_igetl(f);         /* read id             */

   /* check image type ('ILBM' or 'PBM ') */
   if ((data_id != IFF_ILBM) && (data_id != IFF_PBM)) {
      pack_fclose(f);
      return NULL;
   }

   pbm_mode = data_id == IFF_PBM;

   data_id = pack_igetl(f);         /* read id               */
   if (data_id != IFF_BMHD) {       /* check for header      */
      pack_fclose(f);
      return NULL;
   }

   len = pack_mgetl(f);             /* read header length    */
   if (len != 20) {                 /* check, if it is right */
      pack_fclose(f);
      return NULL;
   }

   w = pack_mgetw(f);               /* read screen width  */

   h = pack_mgetw(f);               /* read screen height */

   pack_igetw(f);                   /* skip initial x position  */
   pack_igetw(f);                   /* skip initial y position  */

   color_depth = pack_getc(f);      /* get image depth   */
   if (color_depth > 8) {
      pack_fclose(f);
      return NULL;
   }

   pack_getc(f);                    /* skip masking type */

   cmp_type = pack_getc(f);         /* get compression type */
   if ((cmp_type != 0) && (cmp_type != 1)) {
      pack_fclose(f);
      return NULL;
   }

   *allegro_errno = 0;

   pack_getc(f);                    /* skip unused field        */
   pack_igetw(f);                   /* skip transparent color   */
   pack_getc(f);                    /* skip x aspect ratio      */
   pack_getc(f);                    /* skip y aspect ratio      */
   pack_igetw(f);                   /* skip default page width  */
   pack_igetw(f);                   /* skip default page height */

   check_flags = 0;

   do {  /* We'll use cycle to skip possible junk      */
	 /*  chunks: ANNO, CAMG, GRAB, DEST, TEXT etc. */
      data_id = pack_igetl(f);

      switch(data_id) {

	 case IFF_CMAP:
	    memset(pal, 0, 256 * 3);
	    len = pack_mgetl(f) / 3;
	    for (i=0; i<len; i++) {
	       pal[i].r = pack_getc(f) >> 2;
	       pal[i].g = pack_getc(f) >> 2;
	       pal[i].b = pack_getc(f) >> 2;
	    }
	    check_flags |= 1;       /* flag "palette read" */
	    break;

	 case IFF_BODY:
	    pack_igetl(f);          /* skip BODY size */
	    b = create_bitmap_ex(8, w, h);
	    if (!b) {
	       pack_fclose(f);
	       return NULL;
	    }

	    memset(b->line[0], 0, w * h);

	    if (pbm_mode)
	       bpl = w;
	    else {
	       bpl = w >> 3;        /* calc bytes per line  */
	       if (w & 7)           /* for finish bits      */
		  bpl++;
	    }
	    if (bpl & 1)            /* alignment            */
	       bpl++;
	    line_buf = _AL_MALLOC_ATOMIC(bpl);
	    if (!line_buf) {
	       pack_fclose(f);
	       return NULL;
	    }

	    if (pbm_mode) {
	       for (y = 0; y < h; y++) {
		  if (cmp_type) {
		     i = 0;
		     while (i < bpl) {
			uc = pack_getc(f);
			if (uc < 128) {
			   uc++;
			   pack_fread(&line_buf[i], uc, f);
			   i += uc;
			}
			else if (uc > 128) {
			   uc = 257 - uc;
			   ch = pack_getc(f);
			   memset(&line_buf[i], ch, uc);
			   i += uc;
			}
			/* 128 (0x80) means NOP - no operation  */
		     }
		  }
		  else  /* pure theoretical situation */
		     pack_fread(line_buf, bpl, f);

		  memcpy(b->line[y], line_buf, bpl);
	       }
	    }
	    else {
	       for (y = 0; y < h; y++) {
		  for (bit_plane = 0; bit_plane < color_depth; bit_plane++) {
		     if (cmp_type) {
			i = 0;
			while (i < bpl) {
			   uc = pack_getc(f);
			   if (uc < 128) {
			      uc++;
			      pack_fread(&line_buf[i], uc, f);
			      i += uc;
			   }
			   else if (uc > 128) {
			      uc = 257 - uc;
			      ch = pack_getc(f);
			      memset(&line_buf[i], ch, uc);
			      i += uc;
			   }
			   /* 128 (0x80) means NOP - no operation  */
			}
		     }
		     else
			pack_fread(line_buf, bpl, f);

		     bit_mask = 1 << bit_plane;
		     ppl = bpl;     /* for all pixel blocks */
		     if (w & 7)     /*  may be, except the  */
			ppl--;      /*  the last            */

		     for (x = 0; x < ppl; x++) {
			if (line_buf[x] & 128)
			   b->line[y][x * 8]     |= bit_mask;
			if (line_buf[x] & 64)
			   b->line[y][x * 8 + 1] |= bit_mask;
			if (line_buf[x] & 32)
			   b->line[y][x * 8 + 2] |= bit_mask;
			if (line_buf[x] & 16)
			   b->line[y][x * 8 + 3] |= bit_mask;
			if (line_buf[x] & 8)
			   b->line[y][x * 8 + 4] |= bit_mask;
			if (line_buf[x] & 4)
			   b->line[y][x * 8 + 5] |= bit_mask;
			if (line_buf[x] & 2)
			   b->line[y][x * 8 + 6] |= bit_mask;
			if (line_buf[x] & 1)
			   b->line[y][x * 8 + 7] |= bit_mask;
		     }

		     /* last pixel block */
		     if (w & 7) {
			x = bpl - 1;

			/* no necessary to check if (w & 7) > 0 in */
			/* first condition, because (w & 7) != 0   */
			if (line_buf[x] & 128)
			   b->line[y][x * 8]     |= bit_mask;
			if ((line_buf[x] & 64) && ((w & 7) > 1))
			   b->line[y][x * 8 + 1] |= bit_mask;
			if ((line_buf[x] & 32) && ((w & 7) > 2))
			   b->line[y][x * 8 + 2] |= bit_mask;
			if ((line_buf[x] & 16) && ((w & 7) > 3))
			   b->line[y][x * 8 + 3] |= bit_mask;
			if ((line_buf[x] & 8)  && ((w & 7) > 4))
			   b->line[y][x * 8 + 4] |= bit_mask;
			if ((line_buf[x] & 4)  && ((w & 7) > 5))
			   b->line[y][x * 8 + 5] |= bit_mask;
			if ((line_buf[x] & 2)  && ((w & 7) > 6))
			   b->line[y][x * 8 + 6] |= bit_mask;
			if ((line_buf[x] & 1)  && ((w & 7) > 7))
			   b->line[y][x * 8 + 7] |= bit_mask;
		     }
		  }
	       }
	    }
	    _AL_FREE(line_buf);
	    check_flags |= 2;       /* flag "bitmap read" */
	    break;

	 default:                   /* skip useless chunks  */
	    len = pack_mgetl(f);
	    if (len & 1)
	       len++;
	    for (l=0; l < (len >> 1); l++)
	       pack_igetw(f);
      }

      /* Exit from loop if we are at the end of file, */
      /* or if we loaded both bitmap and palette      */

   } while ((check_flags != 3) && (!pack_feof(f)));

   pack_fclose(f);

   if (check_flags != 3) {
      if (check_flags & 2)
	 destroy_bitmap(b);
      return FALSE;
   }

   if (*allegro_errno) {
      destroy_bitmap(b);
      return FALSE;
   }

   if (dest_depth != 8)
      b = _fixup_loaded_bitmap(b, pal, dest_depth);

   return b;
}

