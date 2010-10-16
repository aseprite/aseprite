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
 *      LZSS compression routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Original code by Haruhiko Okumura.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"


/*
   This compression algorithm is based on the ideas of Lempel and Ziv,
   with the modifications suggested by Storer and Szymanski. The algorithm 
   is based on the use of a ring buffer, which initially contains zeros. 
   We read several characters from the file into the buffer, and then 
   search the buffer for the longest string that matches the characters 
   just read, and output the length and position of the match in the buffer.

   With a buffer size of 4096 bytes, the position can be encoded in 12
   bits. If we represent the match length in four bits, the <position,
   length> pair is two bytes long. If the longest match is no more than
   two characters, then we send just one character without encoding, and
   restart the process with the next letter. We must send one extra bit
   each time to tell the decoder whether we are sending a <position,
   length> pair or an unencoded character, and these flags are stored as
   an eight bit mask every eight items.

   This implementation uses binary trees to speed up the search for the
   longest match.

   Original code by Haruhiko Okumura, 4/6/1989.
   12-2-404 Green Heights, 580 Nagasawa, Yokosuka 239, Japan.

   Modified for use in the Allegro filesystem by Shawn Hargreaves.

   Use, distribute, and modify this code freely.
*/


#define N            4096           /* 4k buffers for LZ compression */
#define F            18             /* upper limit for LZ match length */
#define THRESHOLD    2              /* LZ encode string into pos and length
				       if match size is greater than this */


struct LZSS_PACK_DATA               /* stuff for doing LZ compression */
{
   int state;                       /* where have we got to in the pack? */
   int i, c, len, r, s;
   int last_match_length, code_buf_ptr;
   unsigned char mask;
   char code_buf[17];
   int match_position;
   int match_length;
   int lson[N+1];                   /* left children, */
   int rson[N+257];                 /* right children, */
   int dad[N+1];                    /* and parents, = binary search trees */
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
};


struct LZSS_UNPACK_DATA             /* for reading LZ files */
{
   int state;                       /* where have we got to? */
   int i, j, k, r, c;
   int flags;
   unsigned char text_buf[N+F-1];   /* ring buffer, with F-1 extra bytes
				       for string comparison */
};



/*** Compression (writing) ***/

/* create_lzss_pack_data:
 *  Creates a PACK_DATA structure.
 */
LZSS_PACK_DATA *create_lzss_pack_data(void)
{
   LZSS_PACK_DATA *dat;
   int c;

   if ((dat = _AL_MALLOC_ATOMIC(sizeof(LZSS_PACK_DATA))) == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c < N - F; c++)
      dat->text_buf[c] = 0;

   dat->state = 0;

   return dat;
}



/* free_lzss_pack_data:
 *  Frees an LZSS_PACK_DATA structure.
 */
void free_lzss_pack_data(LZSS_PACK_DATA *dat)
{
   ASSERT(dat);

   _AL_FREE(dat);
}



/* lzss_inittree:
 *  For i = 0 to N-1, rson[i] and lson[i] will be the right and left 
 *  children of node i. These nodes need not be initialized. Also, dad[i] 
 *  is the parent of node i. These are initialized to N, which stands for 
 *  'not used.' For i = 0 to 255, rson[N+i+1] is the root of the tree for 
 *  strings that begin with character i. These are initialized to N. Note 
 *  there are 256 trees.
 */
static void lzss_inittree(LZSS_PACK_DATA *dat)
{
   int i;

   for (i=N+1; i<=N+256; i++)
      dat->rson[i] = N;

   for (i=0; i<N; i++)
      dat->dad[i] = N;
}



/* lzss_insertnode:
 *  Inserts a string of length F, text_buf[r..r+F-1], into one of the trees 
 *  (text_buf[r]'th tree) and returns the longest-match position and length 
 *  via match_position and match_length. If match_length = F, then removes 
 *  the old node in favor of the new one, because the old one will be 
 *  deleted sooner. Note r plays double role, as tree node and position in 
 *  the buffer. 
 */
static void lzss_insertnode(int r, LZSS_PACK_DATA *dat)
{
   int i, p, cmp;
   unsigned char *key;
   unsigned char *text_buf = dat->text_buf;

   cmp = 1;
   key = &text_buf[r];
   p = N + 1 + key[0];
   dat->rson[r] = dat->lson[r] = N;
   dat->match_length = 0;

   for (;;) {

      if (cmp >= 0) {
	 if (dat->rson[p] != N)
	    p = dat->rson[p];
	 else {
	    dat->rson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }
      else {
	 if (dat->lson[p] != N)
	    p = dat->lson[p];
	 else {
	    dat->lson[p] = r;
	    dat->dad[r] = p;
	    return;
	 }
      }

      for (i = 1; i < F; i++)
	 if ((cmp = key[i] - text_buf[p + i]) != 0)
	    break;

      if (i > dat->match_length) {
	 dat->match_position = p;
	 if ((dat->match_length = i) >= F)
	    break;
      }
   }

   dat->dad[r] = dat->dad[p];
   dat->lson[r] = dat->lson[p];
   dat->rson[r] = dat->rson[p];
   dat->dad[dat->lson[p]] = r;
   dat->dad[dat->rson[p]] = r;
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = r;
   else
      dat->lson[dat->dad[p]] = r;
   dat->dad[p] = N;                 /* remove p */
}



/* lzss_deletenode:
 *  Removes a node from a tree.
 */
static void lzss_deletenode(int p, LZSS_PACK_DATA *dat)
{
   int q;

   if (dat->dad[p] == N)
      return;     /* not in tree */

   if (dat->rson[p] == N)
      q = dat->lson[p];
   else
      if (dat->lson[p] == N)
	 q = dat->rson[p];
      else {
	 q = dat->lson[p];
	 if (dat->rson[q] != N) {
	    do {
	       q = dat->rson[q];
	    } while (dat->rson[q] != N);
	    dat->rson[dat->dad[q]] = dat->lson[q];
	    dat->dad[dat->lson[q]] = dat->dad[q];
	    dat->lson[q] = dat->lson[p];
	    dat->dad[dat->lson[p]] = q;
	 }
	 dat->rson[q] = dat->rson[p];
	 dat->dad[dat->rson[p]] = q;
      }

   dat->dad[q] = dat->dad[p];
   if (dat->rson[dat->dad[p]] == p)
      dat->rson[dat->dad[p]] = q;
   else
      dat->lson[dat->dad[p]] = q;

   dat->dad[p] = N;
}



/* lzss_write:
 *  Packs size bytes from buf, using the pack information contained in dat.
 *  Returns 0 on success.
 */
int lzss_write(PACKFILE *file, LZSS_PACK_DATA *dat, int size, unsigned char *buf, int last)
{
   int i = dat->i;
   int c = dat->c;
   int len = dat->len;
   int r = dat->r;
   int s = dat->s;
   int last_match_length = dat->last_match_length;
   int code_buf_ptr = dat->code_buf_ptr;
   unsigned char mask = dat->mask;
   int ret = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   dat->code_buf[0] = 0;
      /* code_buf[1..16] saves eight units of code, and code_buf[0] works
	 as eight flags, "1" representing that the unit is an unencoded
	 letter (1 byte), "0" a position-and-length pair (2 bytes).
	 Thus, eight units require at most 16 bytes of code. */

   code_buf_ptr = mask = 1;

   s = 0;
   r = N - F;
   lzss_inittree(dat);

   for (len=0; (len < F) && (size > 0); len++) {
      dat->text_buf[r+len] = *(buf++);
      if (--size == 0) {
	 if (!last) {
	    dat->state = 1;
	    goto getout;
	 }
      }
      pos1:
	 ; 
   }

   if (len == 0)
      goto getout;

   for (i=1; i <= F; i++)
      lzss_insertnode(r-i,dat);
	    /* Insert the F strings, each of which begins with one or
	       more 'space' characters. Note the order in which these
	       strings are inserted. This way, degenerate trees will be
	       less likely to occur. */

   lzss_insertnode(r,dat);
	    /* Finally, insert the whole string just read. match_length
	       and match_position are set. */

   do {
      if (dat->match_length > len)
	 dat->match_length = len;  /* match_length may be long near the end */

      if (dat->match_length <= THRESHOLD) {
	 dat->match_length = 1;  /* not long enough match: send one byte */
	 dat->code_buf[0] |= mask;    /* 'send one byte' flag */
	 dat->code_buf[code_buf_ptr++] = dat->text_buf[r]; /* send uncoded */
      }
      else {
	 /* send position and length pair. Note match_length > THRESHOLD */
	 dat->code_buf[code_buf_ptr++] = (unsigned char) dat->match_position;
	 dat->code_buf[code_buf_ptr++] = (unsigned char)
				     (((dat->match_position >> 4) & 0xF0) |
				      (dat->match_length - (THRESHOLD + 1)));
      }

      if ((mask <<= 1) == 0) {                  /* shift mask left one bit */

	 if ((file->is_normal_packfile) && (file->normal.passpos) &&
	     (file->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
	 {
	    dat->code_buf[0] ^= *file->normal.passpos;
	    file->normal.passpos++;
	    if (!*file->normal.passpos)
	       file->normal.passpos = file->normal.passdata;
	 }

	 for (i=0; i<code_buf_ptr; i++)         /* send at most 8 units of */
	    pack_putc(dat->code_buf[i], file);  /* code together */

	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
	 dat->code_buf[0] = 0;
	 code_buf_ptr = mask = 1;
      }

      last_match_length = dat->match_length;

      for (i=0; (i < last_match_length) && (size > 0); i++) {
	 c = *(buf++);
	 if (--size == 0) {
	    if (!last) {
	       dat->state = 2;
	       goto getout;
	    }
	 }
	 pos2:
	 lzss_deletenode(s,dat);    /* delete old strings and */
	 dat->text_buf[s] = c;      /* read new bytes */
	 if (s < F-1)
	    dat->text_buf[s+N] = c; /* if the position is near the end of
				       buffer, extend the buffer to make
				       string comparison easier */
	 s = (s+1) & (N-1);
	 r = (r+1) & (N-1);         /* since this is a ring buffer,
				       increment the position modulo N */

	 lzss_insertnode(r,dat);    /* register the string in
				       text_buf[r..r+F-1] */
      }

      while (i++ < last_match_length) {   /* after the end of text, */
	 lzss_deletenode(s,dat);          /* no need to read, but */
	 s = (s+1) & (N-1);               /* buffer may not be empty */
	 r = (r+1) & (N-1);
	 if (--len)
	    lzss_insertnode(r,dat); 
      }

   } while (len > 0);   /* until length of string to be processed is zero */

   if (code_buf_ptr > 1) {         /* send remaining code */

      if ((file->is_normal_packfile) && (file->normal.passpos) &&
	  (file->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
      {
	 dat->code_buf[0] ^= *file->normal.passpos;
	 file->normal.passpos++;
	 if (!*file->normal.passpos)
	    file->normal.passpos = file->normal.passdata;
      }

      for (i=0; i<code_buf_ptr; i++) {
	 pack_putc(dat->code_buf[i], file);
	 if (pack_ferror(file)) {
	    ret = EOF;
	    goto getout;
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->c = c;
   dat->len = len;
   dat->r = r;
   dat->s = s;
   dat->last_match_length = last_match_length;
   dat->code_buf_ptr = code_buf_ptr;
   dat->mask = mask;

   return ret;
}



/*** Decompression (reading) ***/

/* create_unpack_data:
 *  Creates an LZSS_UNPACK_DATA structure.
 */
LZSS_UNPACK_DATA *create_lzss_unpack_data(void)
{
   LZSS_UNPACK_DATA *dat;
   int c;

   if ((dat = _AL_MALLOC_ATOMIC(sizeof(LZSS_UNPACK_DATA))) == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   for (c=0; c < N - F; c++)
      dat->text_buf[c] = 0;

   dat->state = 0;

   return dat;
}



/* free_lzss_unpack_data:
 *  Frees an LZSS_UNPACK_DATA structure.
 */
void free_lzss_unpack_data(LZSS_UNPACK_DATA *dat)
{
   ASSERT(dat);

   _AL_FREE(dat);
}



/* lzss_read:
 *  Unpacks from dat into buf, until either EOF is reached or s bytes have
 *  been extracted. Returns the number of bytes added to the buffer
 */
int lzss_read(PACKFILE *file, LZSS_UNPACK_DATA *dat, int s, unsigned char *buf)
{
   int i = dat->i;
   int j = dat->j;
   int k = dat->k;
   int r = dat->r;
   int c = dat->c;
   unsigned int flags = dat->flags;
   int size = 0;

   if (dat->state==2)
      goto pos2;
   else
      if (dat->state==1)
	 goto pos1;

   r = N-F;
   flags = 0;

   for (;;) {
      if (((flags >>= 1) & 256) == 0) {
	 if ((c = pack_getc(file)) == EOF)
	    break;
  
	 if ((file->is_normal_packfile) && (file->normal.passpos) &&
	     (file->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
	 {
	    c ^= *file->normal.passpos;
	    file->normal.passpos++;
	    if (!*file->normal.passpos)
	       file->normal.passpos = file->normal.passdata;
	 }

	 flags = c | 0xFF00;        /* uses higher byte to count eight */
      }

      if (flags & 1) {
	 if ((c = pack_getc(file)) == EOF)
	    break;
	 dat->text_buf[r++] = c;
	 r &= (N - 1);
	 *(buf++) = c;
	 if (++size >= s) {
	    dat->state = 1;
	    goto getout;
	 }
	 pos1:
	    ; 
      }
      else {
	 if ((i = pack_getc(file)) == EOF)
	    break;
	 if ((j = pack_getc(file)) == EOF)
	    break;
	 i |= ((j & 0xF0) << 4);
	 j = (j & 0x0F) + THRESHOLD;
	 for (k=0; k <= j; k++) {
	    c = dat->text_buf[(i + k) & (N - 1)];
	    dat->text_buf[r++] = c;
	    r &= (N - 1);
	    *(buf++) = c;
	    if (++size >= s) {
	       dat->state = 2;
	       goto getout;
	    }
	    pos2:
	       ; 
	 }
      }
   }

   dat->state = 0;

   getout:

   dat->i = i;
   dat->j = j;
   dat->k = k;
   dat->r = r;
   dat->c = c;
   dat->flags = flags;

   return size;
}



/* _al_lzss_incomplete_state:
 *  Return non-zero if the previous lzss_read() call was in the middle of
 *  unpacking a sequence of bytes into the supplied buffer, but had to suspend
 *  because the buffer wasn't big enough.
 */
int _al_lzss_incomplete_state(AL_CONST LZSS_UNPACK_DATA *dat)
{
   return dat->state == 2;
}
