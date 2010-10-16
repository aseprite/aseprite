/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *      FLI/FLC player.
 *
 *      By Shawn Hargreaves, based on code provided by Jonathan Tarbox.
 *
 *      Major portability and reliability improvements by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"


#define FLI_MAGIC1            0xAF11      /* file header magic number */
#define FLI_MAGIC2            0xAF12      /* file magic number (Pro) */
#define FLI_FRAME_MAGIC       0xF1FA      /* frame header magic number */
#define FLI_FRAME_PREFIX      0xF100      /* FLC's prefix info */
#define FLI_FRAME_USELESS     0x00A1      /* FLC's garbage frame */



typedef struct FLI_HEADER
{
   long              size;
   unsigned short    type;
   unsigned short    frame_count;
   unsigned short    width;
   unsigned short    height;
   unsigned short    bits_a_pixel;
   unsigned short    flags;
   unsigned short    speed;
   long              next_head;
   long              frames_in_table;
   char              reserved[102];
} FLI_HEADER;

#define sizeof_FLI_HEADER (4+2+2+2+2+2+2+2+4+4+102)



typedef struct FLI_FRAME
{
   unsigned long     size;
   unsigned short    type;
   unsigned short    chunks;
   char              pad[8];
} FLI_FRAME;

#define sizeof_FLI_FRAME (4+2+2+8)



typedef struct FLI_CHUNK 
{
   unsigned long     size;
   unsigned short    type;
} FLI_CHUNK;

#define sizeof_FLI_CHUNK (4+2)



static int fli_status = FLI_NOT_OPEN;  /* current state of the FLI player */

BITMAP *fli_bitmap = NULL;             /* current frame of the FLI */
PALETTE fli_palette;                   /* current palette the FLI is using */

int fli_bmp_dirty_from = INT_MAX;      /* what part of fli_bitmap is dirty */
int fli_bmp_dirty_to = INT_MIN;
int fli_pal_dirty_from = INT_MAX;      /* what part of fli_palette is dirty */
int fli_pal_dirty_to = INT_MIN;

int fli_frame = 0;                     /* current frame number in the FLI */

volatile int fli_timer = 0;            /* for timing FLI playback */

static PACKFILE *fli_file = NULL;      /* the file we are reading */
static char *fli_filename = NULL;      /* name of the file */

static void *fli_mem_data = NULL;      /* the memory FLI we are playing */
static int fli_mem_pos = 0;            /* position in the memory FLI */

static FLI_HEADER fli_header;          /* header structure */
static FLI_FRAME frame_header;         /* frame header structure */

static unsigned char _fli_broken_data[3 * 256]; /* data substituted for broken chunks */



/* fli_timer_callback:
 *  Timer interrupt handler for syncing FLI files.
 */
static void fli_timer_callback(void)
{
   fli_timer++;
}

END_OF_STATIC_FUNCTION(fli_timer_callback);



/* fli_read:
 *  Helper function to get a block of data from the FLI, which can read 
 *  from disk or a copy of the FLI held in memory. If buf is set, that is 
 *  where it stores the data, otherwise it uses the scratch buffer. Returns 
 *  a pointer to the data, or NULL on error.
 */
static void *fli_read(void *buf, int size)
{
   int result;

   if (fli_mem_data) {
      if (buf)
	 memcpy(buf, (char *)fli_mem_data+fli_mem_pos, size);
      else
	 buf = (char *)fli_mem_data+fli_mem_pos;

      fli_mem_pos += size;
   }
   else {
      if (!buf) {
	 _grow_scratch_mem(size);
	 buf = _scratch_mem;
      }

      result = pack_fread(buf, size, fli_file);
      if (result != size)
	 return NULL;
   }

   return buf;
}



/* fli_rewind:
 *  Helper function to rewind to the beginning of the FLI file data.
 *  Pass offset from the beginning of the data in bytes.
 */
static void fli_rewind(int offset)
{
   if (fli_mem_data) {
      fli_mem_pos = offset;
   }
   else {
      pack_fclose(fli_file);
      fli_file = pack_fopen(fli_filename, F_READ);
      if (fli_file)
	 pack_fseek(fli_file, offset);
      else
	 fli_status = FLI_ERROR;
   }
}



/* fli_skip:
 *  Helper function to skip some bytes of the FLI file data.
 *  Pass number of bytes to skip.
 */
static void fli_skip(int bytes)
{
   if (fli_mem_data) {
      fli_mem_pos += bytes;
   }
   else {
      pack_fseek(fli_file, bytes);
   }
}



/* helpers for reading FLI chunk data */
#if 0
/* the "cast expression as lvalue" extension is deprecated in GCC 3.4 */
#define READ_BYTE_NC(p)    (*((unsigned char *)(p))++)
#define READ_CHAR_NC(p)    (*((signed char *)(p))++)
#else
#define READ_BYTE_NC(p)    (*(unsigned char *)(p)++)
#define READ_CHAR_NC(p)    (*(signed char *)(p)++)
#endif

#if (defined ALLEGRO_GCC) && (defined ALLEGRO_LITTLE_ENDIAN) && (!defined ALLEGRO_ARM) && (!defined ALLEGRO_PSP)

#if 0
/* the "cast expression as lvalue" extension is deprecated in GCC 3.4 */
#define READ_WORD_NC(p)    (*((uint16_t *)(p))++)
#define READ_SHORT_NC(p)   (*((int16_t *)(p))++)
#define READ_ULONG_NC(p)   (*((uint32_t *)(p))++)
#define READ_LONG_NC(p)    (*((int32_t *)(p))++)
#else
/* we use the "statement-expression" extension instead */
#define READ_WORD_NC(p)    ({ uint16_t *__p = (uint16_t *)(p); p+=2; *__p; })
#define READ_SHORT_NC(p)   ({ int16_t *__p = (int16_t *)(p); p+=2; *__p; })
#define READ_ULONG_NC(p)   ({ uint32_t *__p = (uint32_t *)(p); p+=4; *__p; })
#define READ_LONG_NC(p)    ({ int32_t *__p = (int32_t *)(p); p+=4; *__p; })
#endif

#else

static unsigned short _fli_read_word_nc(unsigned char **p)
{
   unsigned short t;
   t = (((unsigned short) *((unsigned char*) (*p)))
	| ((unsigned short) *((unsigned char*) (*p) + 1) << 8));
   *p += 2;
   return t;
}

static signed short _fli_read_short_nc(unsigned char **p)
{
   unsigned short t = _fli_read_word_nc(p);
   if (t & (unsigned short) 0x8000)
      return -(signed short) (~(t - 1) & (unsigned short) 0x7FFF);
   else
      return (signed short) t;
}

static unsigned long _fli_read_ulong_nc(unsigned char **p)
{
   unsigned long t;
   t = (((unsigned long) *((unsigned char*) (*p)))
	| ((unsigned long) *((unsigned char*) (*p) + 1) << 8)
	| ((unsigned long) *((unsigned char*) (*p) + 2) << 16)
	| ((unsigned long) *((unsigned char*) (*p) + 3) << 24));
   *p += 4;
   return t;
}

static signed long _fli_read_long_nc(unsigned char **p)
{
   unsigned long t = _fli_read_ulong_nc(p);
   if (t & (unsigned long) 0x80000000L)
      return -(signed long) (~(t - 1) & (unsigned long) 0x7FFFFFFFL);
   else
      return (signed long) t;
}

#define READ_WORD_NC(p)  _fli_read_word_nc(&(p))
#define READ_SHORT_NC(p) _fli_read_short_nc(&(p))
#define READ_ULONG_NC(p) _fli_read_ulong_nc(&(p))
#define READ_LONG_NC(p)  _fli_read_long_nc(&(p))

#endif

#define READ_BLOCK_NC(p,pos,size)                               \
{                                                               \
   memcpy((pos), (p), (size));                                  \
   (p) += (size);                                               \
}

#define READ_RLE_BYTE_NC(p,pos,size)                            \
   memset((pos), READ_BYTE_NC(p), (size))

#if (defined ALLEGRO_GCC) && (defined ALLEGRO_I386)

#define READ_RLE_WORD_NC(p,pos,size)                            \
{                                                               \
   int c;                                                       \
   uint16_t *ptr = (uint16_t*) (pos);                           \
   unsigned short v = READ_WORD_NC(p);                          \
								\
   for (c = 0; c < (size); c++)                                 \
      *ptr++ = v;                                               \
}

#else

#define READ_RLE_WORD_NC(p,pos,size)                            \
{                                                               \
   int c;                                                       \
   unsigned char *ptr = (pos);                                  \
   unsigned char v1 = READ_BYTE_NC(p);                          \
   unsigned char v2 = READ_BYTE_NC(p);                          \
								\
   for (c = 0; c < (size); c++) {                               \
      *ptr++ = v1;                                              \
      *ptr++ = v2;                                              \
   }                                                            \
}

#endif



/* support for broken chunks (copy reminder of chunk and add zeros)
 * this needs not be fast because FLI data should not be broken.
 *  p is a pointer to reminder of chunk data
 *  sz is a size of chunk after subtructing size bytes (known to be < 0)
 *  size is how much bytes we need
 *  (size + sz) is how much bytes is left in chunk */
#define FLI_KLUDGE(p,sz,size)                                   \
{                                                               \
   if (((size) + (sz)) <= 0) {                                  \
      memset(_fli_broken_data, 0, (size));                      \
   }                                                            \
   else {                                                       \
      memcpy(_fli_broken_data, (p), (size) + (sz));             \
      memset(_fli_broken_data + (size) + (sz), 0, -(sz));       \
   }                                                            \
   (p) = _fli_broken_data;                                      \
}



/* do_fli_256_color:
 *  Processes an FLI 256_COLOR chunk
 */
static void do_fli_256_color(unsigned char *p, int sz)
{
   int packets;
   int end;
   int offset;
   int length;

   offset = 0;
   if ((sz -= 2) < 0)
      return;
   packets = READ_SHORT_NC(p);

   while (packets-- > 0) {
      if ((sz -= 2) < 0)
	 return;
      offset += READ_BYTE_NC(p);
      length = READ_BYTE_NC(p);
      if (length == 0)
	 length = 256;

      end = offset + length;
      if (end > PAL_SIZE)
	 return;
      else if ((sz -= length * 3) < 0) {
	 FLI_KLUDGE(p, sz, length * 3);
      }

      fli_pal_dirty_from = MIN(fli_pal_dirty_from, offset);
      fli_pal_dirty_to = MAX(fli_pal_dirty_to, end-1);

      for(; offset < end; offset++) {
	 fli_palette[offset].r = READ_BYTE_NC(p) / 4;
	 fli_palette[offset].g = READ_BYTE_NC(p) / 4;
	 fli_palette[offset].b = READ_BYTE_NC(p) / 4;
      }
   }
}



/* do_fli_delta:
 *  Processes an FLI DELTA chunk
 */
static void do_fli_delta(unsigned char *p, int sz)
{
   int lines;
   int packets;
   int size;
   int y;
   unsigned char *curr;
   unsigned char *bitmap_end = fli_bitmap->line[fli_bitmap->h-1] + fli_bitmap->w;

   y = 0;
   if ((sz -= 2) < 0)
      return;
   lines = READ_SHORT_NC(p);

   while (lines-- > 0) {                  /* for each line... */
      if ((sz -= 2) < 0)
	 return;
      packets = READ_SHORT_NC(p);

      while (packets < 0) {
	 if (packets & 0x4000)
	    y -= packets;
	 else if (y < fli_bitmap->h)
	    fli_bitmap->line[y][fli_bitmap->w-1] = packets & 0xFF;

	 if ((sz -= 2) < 0)
	    return;
	 packets = READ_SHORT_NC(p);
      }
      if (y >= fli_bitmap->h)
	 return;

      curr = fli_bitmap->line[y];

      fli_bmp_dirty_from = MIN(fli_bmp_dirty_from, y);
      fli_bmp_dirty_to = MAX(fli_bmp_dirty_to, y);

      while (packets-- > 0) {
	 if ((sz -= 2) < 0)
	    return;
	 curr += READ_BYTE_NC(p);         /* skip bytes */
	 size = READ_CHAR_NC(p);

	 if (size > 0) {                  /* copy size words */
	    if ((curr + size * 2) > bitmap_end)
	       return;
	    else if ((sz -= size * 2) < 0) {
	       FLI_KLUDGE(p, sz, size * 2);
	    }
	    READ_BLOCK_NC(p, curr, size*2);
	    curr += size*2;
	 }
	 else if (size < 0) {             /* repeat word -size times */
	    size = -size;
	    if ((curr + size * 2) > bitmap_end)
	       return;
	    else if ((sz -= 2) < 0) {
	       FLI_KLUDGE(p, sz, 2);
	    }
	    READ_RLE_WORD_NC(p, curr, size);
	    curr += size*2;
	 }
      }

      y++;
   }
}



/* do_fli_color:
 *  Processes an FLI COLOR chunk
 */
static void do_fli_color(unsigned char *p, int sz)
{
   int packets;
   int end;
   int offset;
   int length;

   offset = 0;
   if ((sz -= 2) < 0)
      return;
   packets = READ_SHORT_NC(p);

   while (packets-- > 0) {
      if ((sz -= 2) < 0)
	 return;
      offset += READ_BYTE_NC(p);
      length = READ_BYTE_NC(p);
      if (length == 0)
	 length = 256;

      end = offset + length;
      if (end > PAL_SIZE)
	 return;
      else if ((sz -= length * 3) < 0) {
	 FLI_KLUDGE(p, sz, length * 3);
      }

      fli_pal_dirty_from = MIN(fli_pal_dirty_from, offset);
      fli_pal_dirty_to = MAX(fli_pal_dirty_to, end-1);

      for(; offset < end; offset++) {
	 fli_palette[offset].r = READ_BYTE_NC(p);
	 fli_palette[offset].g = READ_BYTE_NC(p);
	 fli_palette[offset].b = READ_BYTE_NC(p);
      }
   }
}



/* do_fli_lc:
 *  Processes an FLI LC chunk
 */
static void do_fli_lc(unsigned char *p, int sz)
{
   int lines;
   int packets;
   int size;
   int y;
   unsigned char *curr;
   unsigned char *bitmap_end = fli_bitmap->line[fli_bitmap->h-1] + fli_bitmap->w;

   if ((sz -= 4) < 0)
      return;
   y = READ_WORD_NC(p);
   lines = READ_SHORT_NC(p);

   if (y >= fli_bitmap->h)
      return;
   else if ((y + lines) > fli_bitmap->h)
      lines = fli_bitmap->h - y;

   fli_bmp_dirty_from = MIN(fli_bmp_dirty_from, y);
   fli_bmp_dirty_to = MAX(fli_bmp_dirty_to, y+lines-1);

   while (lines-- > 0) {                     /* for each line... */
      if ((sz -= 1) < 0)
	 return;
      packets = READ_BYTE_NC(p);
      curr = fli_bitmap->line[y];

      while (packets-- > 0) {
	 if ((sz -= 2) < 0)
	    return;
	 curr += READ_BYTE_NC(p);            /* skip bytes */
	 size = READ_CHAR_NC(p);

	 if (size > 0) {                     /* copy size bytes */
	    if ((curr + size) > bitmap_end)
	       return;
	    else if ((sz -= size) < 0) {
	       FLI_KLUDGE(p, sz, size);
	    }
	    READ_BLOCK_NC(p, curr, size);
	    curr += size;
	 }
	 else if (size < 0) {                /* repeat byte -size times */
	    size = -size;
	    if ((curr + size) > bitmap_end)
	       return;
	    else if ((sz -= 1) < 0) {
	       FLI_KLUDGE(p, sz, 1);
	    }
	    READ_RLE_BYTE_NC(p, curr, size);
	    curr += size;
	 }
      }

      y++;
   }
}



/* do_fli_black:
 *  Processes an FLI BLACK chunk
 */
static void do_fli_black(void)
{
   clear_bitmap(fli_bitmap);

   fli_bmp_dirty_from = 0;
   fli_bmp_dirty_to = fli_bitmap->h-1;
}



/* do_fli_brun:
 *  Processes an FLI BRUN chunk
 */
static void do_fli_brun(unsigned char *p, int sz)
{
   int packets;
   int size;
   int y;
   unsigned char *curr;
   unsigned char *bitmap_end = fli_bitmap->line[fli_bitmap->h-1] + fli_bitmap->w;

   fli_bmp_dirty_from = 0;
   fli_bmp_dirty_to = fli_bitmap->h-1;

   for (y=0; y<fli_bitmap->h; y++) {         /* for each line... */
      if ((sz -= 1) < 0)
	 return;
      packets = READ_BYTE_NC(p);
      curr = fli_bitmap->line[y];

      if (packets == 0) {                    /* FLC chunk (fills the whole line) */
	 unsigned char *line_end = curr + fli_bitmap->w;

	 while (curr < line_end) {
	    if ((sz -= 1) < 0)
	       return;
	    size = READ_CHAR_NC(p);

	    if (size < 0) {                     /* copy -size bytes */
	       size = -size;
	       if ((curr + size) > bitmap_end)
		  return;
	       else if ((sz -= size) < 0) {
		  FLI_KLUDGE(p, sz, size);
	       }
	       READ_BLOCK_NC(p, curr, size);
	       curr += size;
	    }
	    else if (size > 0) {                /* repeat byte size times */
	       if ((curr + size) > bitmap_end)
		  return;
	       else if ((sz -= 1) < 0) {
		  FLI_KLUDGE(p, sz, 1);
	       }
	       READ_RLE_BYTE_NC(p, curr, size);
	       curr += size;
	    }
	 }
      }
      else {
	 /* FLI chunk (uses packets count) */
	 while (packets-- > 0) {
	    if ((sz -= 1) < 0)
	       return;
	    size = READ_CHAR_NC(p);

	    if (size < 0) {                     /* copy -size bytes */
	       size = -size;
	       if ((curr + size) > bitmap_end)
		  return;
	       if ((sz -= size) < 0) {
		  FLI_KLUDGE(p, sz, size);
	       }
	       READ_BLOCK_NC(p, curr, size);
	       curr += size;
	    }
	    else if (size > 0) {                /* repeat byte size times */
	       if ((curr + size) > bitmap_end)
		  return;
	       if ((sz -= 1) < 0) {
		  FLI_KLUDGE(p, sz, 1);
	       }
	       READ_RLE_BYTE_NC(p, curr, size);
	       curr += size;
	    }
	 }
      }
   }
}



/* do_fli_copy:
 *  Processes an FLI COPY chunk
 */
static void do_fli_copy(unsigned char *p, int sz)
{
   int y;

   if ((sz -= (fli_bitmap->w * fli_bitmap->h)) < 0)
      return;

   for (y=0; y<fli_bitmap->h; y++)
      READ_BLOCK_NC(p, fli_bitmap->line[y], fli_bitmap->w);

   fli_bmp_dirty_from = 0;
   fli_bmp_dirty_to = fli_bitmap->h-1;
}



/* _fli_read_header:
 *  Reads FLI file header (0 -- OK).
 */
static int _fli_read_header(FLI_HEADER *header)
{
   unsigned char *p = fli_read(NULL, sizeof_FLI_HEADER);

   if (!p)
      return -1;

   header->size = READ_LONG_NC(p);
   header->type = READ_WORD_NC(p);
   header->frame_count = READ_WORD_NC(p);
   header->width = READ_WORD_NC(p);
   header->height = READ_WORD_NC(p);
   header->bits_a_pixel = READ_WORD_NC(p);
   header->flags = READ_WORD_NC(p);
   header->speed = READ_WORD_NC(p);
   header->next_head = READ_LONG_NC(p);
   header->frames_in_table = READ_LONG_NC(p);

   return ((header->size < sizeof_FLI_HEADER) ? -1 : 0);
}



/* _fli_read_frame:
 *  Reads FLI frame header (0 -- OK).
 */
static int _fli_read_frame(FLI_FRAME *frame)
{
   unsigned char *p = fli_read(NULL, sizeof_FLI_FRAME);

   if (!p)
      return -1;

   frame->size = READ_ULONG_NC(p);
   frame->type = READ_WORD_NC(p);
   frame->chunks = READ_WORD_NC(p);

   return ((frame->size < sizeof_FLI_FRAME) ? -1 : 0);
}



/* _fli_parse_chunk:
 *  Parses one FLI chunk.
 */
static int _fli_parse_chunk(FLI_CHUNK *chunk, unsigned char *p, unsigned long frame_size)
{
   if (frame_size < sizeof_FLI_CHUNK)
      return -1;

   chunk->size = READ_ULONG_NC(p);
   chunk->type = READ_WORD_NC(p);

   return (((chunk->size < sizeof_FLI_CHUNK) || (chunk->size > frame_size)) ? -1 : 0);
}



/* read_frame:
 *  Advances to the next frame in the FLI.
 */
static void read_frame(void)
{
   unsigned char *p;
   FLI_CHUNK chunk;
   int c, sz, frame_size;

   if (fli_status != FLI_OK)
      return;

   /* clear the first frame (we need it for looping, because we don't support ring frame) */
   if (fli_frame == 0) {
      clear_bitmap(fli_bitmap);
      fli_bmp_dirty_from = 0;
      fli_bmp_dirty_to = fli_bitmap->h-1;
   }

   get_another_frame:

   /* read the frame header */ 
   if (_fli_read_frame(&frame_header) != 0) {
      fli_status = FLI_ERROR;
      return;
   }

   /* skip FLC's useless frame */
   if ((frame_header.type == FLI_FRAME_PREFIX) || (frame_header.type == FLI_FRAME_USELESS)) {
      fli_skip(frame_header.size-sizeof_FLI_FRAME);

      if (++fli_frame >= fli_header.frame_count)
	 return;

      goto get_another_frame;
   }

   if (frame_header.type != FLI_FRAME_MAGIC) {
      fli_status = FLI_ERROR;
      return;
   }

   /* bytes left in this frame */
   frame_size = frame_header.size - sizeof_FLI_FRAME;

   /* return if there is no data in the frame */
   if (frame_size == 0) {
      fli_frame++;
      return;
   }

   /* read the frame data */
   p = fli_read(NULL, frame_size);
   if (!p) {
      fli_status = FLI_ERROR;
      return;
   }

   /* now to decode it */
   for (c=0; c<frame_header.chunks; c++) {
      if (_fli_parse_chunk(&chunk, p, frame_size) != 0) {
	 /* chunk is broken, but don't return an error */
	 break;
      }

      p += sizeof_FLI_CHUNK;
      sz = chunk.size - sizeof_FLI_CHUNK;
      frame_size -= chunk.size;

      if (c == frame_header.chunks-1)
	 sz += frame_size;

      switch (chunk.type) {

	 case 4: 
	    do_fli_256_color(p, sz);
	    break;

	 case 7:
	    do_fli_delta(p, sz);
	    break;

	 case 11: 
	    do_fli_color(p, sz);
	    break;

	 case 12:
	    do_fli_lc(p, sz);
	    break;

	 case 13:
	    do_fli_black();
	    break;

	 case 15:
	    do_fli_brun(p, sz);
	    break;

	 case 16:
	    do_fli_copy(p, sz);
	    break;

	 default:
	    /* should we return an error? nah... */
	    break;
      }

      p += sz;

      /* alignment */
      if (sz & 1) {
	 p++;
	 frame_size--;
      }
   }

   /* move on to the next frame */
   fli_frame++;
}



/* do_play_fli:
 *  Worker function used by play_fli() and play_memory_fli().
 *  This is complicated by the fact that it puts the timing delay between
 *  reading a frame and displaying it, rather than between one frame and
 *  the next, in order to make the playback as smooth as possible.
 */
static int do_play_fli(BITMAP *bmp, int loop, int (*callback)(void))
{
   int ret;

   ret = next_fli_frame(loop);

   while (ret == FLI_OK) {
      /* update the palette */
      if (fli_pal_dirty_from <= fli_pal_dirty_to)
	 set_palette_range(fli_palette, fli_pal_dirty_from, fli_pal_dirty_to, TRUE);

      /* update the screen */
      if (fli_bmp_dirty_from <= fli_bmp_dirty_to) {
	 vsync();
	 blit(fli_bitmap, bmp, 0, fli_bmp_dirty_from, 0, fli_bmp_dirty_from,
			fli_bitmap->w, 1+fli_bmp_dirty_to-fli_bmp_dirty_from);
      }

      reset_fli_variables();

      if (callback) {
	 ret = (*callback)();
	 if (ret != FLI_OK)
	    break;
      }

      ret = next_fli_frame(loop);

      while (fli_timer <= 0) {
	 /* wait a bit */
	 rest(0);
      }
   }

   close_fli();

   return (ret == FLI_EOF) ? FLI_OK : ret;
}



/* play_fli:
 *  Top level FLI playing function. Plays the specified file, displaying 
 *  it at the top left corner of the specified bitmap. If the callback 
 *  function is not NULL it will be called for each frame in the file, and 
 *  can return zero to continue playing or non-zero to stop the FLI. If 
 *  loop is non-zero the player will cycle through the animation (in this 
 *  case the callback function is the only way to stop the player). Returns 
 *  one of the FLI status constants, or the value returned by the callback 
 *  if this is non-zero. If you need to distinguish between the two, the 
 *  callback should return positive integers, since the FLI status values 
 *  are zero or negative.
 */
int play_fli(AL_CONST char *filename, BITMAP *bmp, int loop, int (*callback)(void))
{
   ASSERT(filename);
   ASSERT(bmp);
   
   if (open_fli(filename) != FLI_OK)
      return FLI_ERROR;

   return do_play_fli(bmp, loop, callback);
}



/* play_memory_fli:
 *  Like play_fli(), but for files which have already been loaded into 
 *  memory. Pass a pointer to the memory containing the FLI data.
 */
int play_memory_fli(void *fli_data, BITMAP *bmp, int loop, int (*callback)(void))
{
   ASSERT(fli_data);
   ASSERT(bmp);
   
   if (open_memory_fli(fli_data) != FLI_OK)
      return FLI_ERROR;

   return do_play_fli(bmp, loop, callback);
}



/* do_open_fli:
 *  Worker function used by open_fli() and open_memory_fli().
 */
static int do_open_fli(void)
{
   long speed;

   /* read the header */
   if (_fli_read_header(&fli_header) != 0) {
      close_fli();
      return FLI_ERROR;
   }

   /* check magic numbers */
   if (((fli_header.bits_a_pixel != 8) && (fli_header.bits_a_pixel != 0)) ||
       ((fli_header.type != FLI_MAGIC1) && (fli_header.type != FLI_MAGIC2))) {
      close_fli();
      return FLI_ERROR;
   }

   if (fli_header.width == 0)
      fli_header.width = 320;

   if (fli_header.height == 0)
      fli_header.height = 200;

   /* create the frame bitmap */
   fli_bitmap = create_bitmap_ex(8, fli_header.width, fli_header.height);
   if (!fli_bitmap) {
      close_fli();
      return FLI_ERROR;
   }

   reset_fli_variables();
   fli_frame = 0;
   fli_timer = 2;
   fli_status = FLI_OK;

   /* install the timer handler */
   LOCK_VARIABLE(fli_timer);
   LOCK_FUNCTION(fli_timer_callback);

   if (fli_header.type == FLI_MAGIC1)
      speed = BPS_TO_TIMER(70) * (long)fli_header.speed;
   else
      speed = MSEC_TO_TIMER((long)fli_header.speed);

   if (speed == 0)
      speed = BPS_TO_TIMER(70);

   install_int_ex(fli_timer_callback, speed);

   return fli_status;
}



/* open_fli:
 *  Opens an FLI file ready for playing.
 */
int open_fli(AL_CONST char *filename)
{
   ASSERT(filename);
   
   if (fli_status != FLI_NOT_OPEN)
      return FLI_ERROR;

   if (fli_filename) {
      _AL_FREE(fli_filename);
      fli_filename = NULL;
   }

   fli_filename = _al_ustrdup(filename);
   if (!fli_filename)
      return FLI_ERROR;

   fli_file = pack_fopen(fli_filename, F_READ);
   if (!fli_file)
      return FLI_ERROR;

   return do_open_fli();
}



/* open_memory_fli:
 *  Like open_fli(), but for files which have already been loaded into 
 *  memory. Pass a pointer to the memory containing the FLI data.
 */
int open_memory_fli(void *fli_data)
{
   ASSERT(fli_data);
   
   if (fli_status != FLI_NOT_OPEN)
      return FLI_ERROR;

   fli_mem_data = fli_data;
   fli_mem_pos = 0;

   return do_open_fli();
}



/* close_fli:
 *  Shuts down the FLI player at the end of the file.
 */
void close_fli(void)
{
   remove_int(fli_timer_callback);

   if (fli_file) {
      pack_fclose(fli_file);
      fli_file = NULL;
   }

   if (fli_filename) {
      _AL_FREE(fli_filename);
      fli_filename = NULL;
   }

   if (fli_bitmap) {
      destroy_bitmap(fli_bitmap);
      fli_bitmap = NULL;
   }

   fli_mem_data = NULL;
   fli_mem_pos = 0;

   reset_fli_variables();

   fli_status = FLI_NOT_OPEN;
}



/* next_fli_frame:
 *  Advances to the next frame of the FLI, leaving the changes in the 
 *  fli_bitmap and fli_palette. If loop is non-zero, it will cycle if 
 *  it reaches the end of the animation. Returns one of the FLI status 
 *  constants.
 */
int next_fli_frame(int loop)
{
   if (fli_status != FLI_OK)
      return fli_status;

   fli_timer--;

   /* end of file? should we loop? */
   if (fli_frame >= fli_header.frame_count) {
      if (loop) {
	 fli_rewind(sizeof_FLI_HEADER);
	 fli_frame = 0;
      }
      else {
	 fli_status = FLI_EOF;
	 return fli_status;
      }
   }

   /* read the next frame */
   read_frame();

   return fli_status;
}



/* reset_fli_variables:
 *  Clears the information about which parts of the FLI bitmap and palette
 *  are dirty, after the screen hardware has been updated.
 */
void reset_fli_variables(void)
{
   fli_bmp_dirty_from = INT_MAX;
   fli_bmp_dirty_to = INT_MIN;
   fli_pal_dirty_from = INT_MAX;
   fli_pal_dirty_to = INT_MIN;
}

