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
 *      String handling functions (UTF-8, Unicode, ASCII, etc).
 *
 *      By Shawn Hargreaves.
 *
 *      Case conversion functions improved by Ole Laursen.
 *
 *      ustrrchr() and usprintf() improvements by Sven Sandberg.
 *
 *      Peter Cech added some non-ASCII characters to uissspace().
 *
 *      size-aware (aka 'z') functions added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* this is a valid encoding in any of the supported formats */
char empty_string[] = EMPTY_STRING;



/* ascii_getc:
 *  Reads a character from an ASCII string.
 */
static int ascii_getc(AL_CONST char *s)
{
   return *((unsigned char *)s);
}



/* ascii_getx:
 *  Reads a character from an ASCII string, advancing the pointer position.
 */
static int ascii_getx(char **s)
{
   return *((unsigned char *)((*s)++));
}



/* ascii_setc:
 *  Sets a character in an ASCII string.
 */
static int ascii_setc(char *s, int c)
{
   *s = c;
   return 1;
}



/* ascii_width:
 *  Returns the width of an ASCII character.
 */
static int ascii_width(AL_CONST char *s)
{
   return 1;
}



/* ascii_cwidth:
 *  Returns the width of an ASCII character.
 */
static int ascii_cwidth(int c)
{
   return 1;
}



/* ascii_isok:
 *  Checks whether this character can be encoded in 8-bit ASCII format.
 */
static int ascii_isok(int c)
{
   return ((c >= 0) && (c <= 255));
}



/* lookup table for implementing 8-bit codepage modes */
static unsigned short _codepage_table[] =
{
   0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
   0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
   0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 
   0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E, 0x5E,
};



/* this default table reduces Latin-1 and Extended-A characters to 7 bits */
static unsigned short _codepage_extras[] =
{
   0xA1, '!',  0xA2, 'c',  0xA3, '#',  0xB5, 'u',  0xBF, '?',  0xC0, 'A', 
   0xC1, 'A',  0xC2, 'A',  0xC3, 'A',  0xC4, 'A',  0xC5, 'A',  0xC6, 'A', 
   0xC7, 'C',  0xC8, 'E',  0xC9, 'E',  0xCA, 'E',  0xCB, 'E',  0xCC, 'I', 
   0xCD, 'I',  0xCE, 'I',  0xCF, 'I',  0xD0, 'D',  0xD1, 'N',  0xD2, 'O', 
   0xD3, 'O',  0xD4, 'O',  0xD5, 'O',  0xD6, 'O',  0xD7, 'X',  0xD8, '0', 
   0xD9, 'U',  0xDA, 'U',  0xDB, 'U',  0xDC, 'U',  0xDD, 'Y',  0xDE, 'P', 
   0xDF, 'S',  0xE0, 'a',  0xE1, 'a',  0xE2, 'a',  0xE3, 'a',  0xE4, 'a', 
   0xE5, 'a',  0xE6, 'a',  0xE7, 'c',  0xE8, 'e',  0xE9, 'e',  0xEA, 'e', 
   0xEB, 'e',  0xEC, 'i',  0xED, 'i',  0xEE, 'i',  0xEF, 'i',  0xF0, 'o', 
   0xF1, 'n',  0xF2, 'o',  0xF3, 'o',  0xF4, 'o',  0xF5, 'o',  0xF6, 'o', 
   0xF8, 'o',  0xF9, 'u',  0xFA, 'u',  0xFB, 'u',  0xFC, 'u',  0xFD, 'y', 
   0xFE, 'p',  0xFF, 'y',

   0x100, 'A',  0x101, 'a',  0x102, 'A',  0x103, 'a',  0x104, 'A',
   0x105, 'a',  0x106, 'C',  0x107, 'c',  0x108, 'C',  0x109, 'c',
   0x10a, 'C',  0x10b, 'c',  0x10c, 'C',  0x10d, 'c',  0x10e, 'D',
   0x10f, 'd',  0x110, 'D',  0x111, 'd',  0x112, 'E',  0x113, 'e',
   0x114, 'E',  0x115, 'e',  0x116, 'E',  0x117, 'e',  0x118, 'E',
   0x119, 'e',  0x11a, 'E',  0x11b, 'e',  0x11c, 'G',  0x11d, 'g',
   0x11e, 'G',  0x11f, 'g',  0x120, 'G',  0x121, 'g',  0x122, 'G',
   0x123, 'g',  0x124, 'H',  0x125, 'h',  0x126, 'H',  0x127, 'h',
   0x128, 'I',  0x129, 'i',  0x12a, 'I',  0x12b, 'i',  0x12c, 'I',
   0x12d, 'i',  0x12e, 'I',  0x12f, 'i',  0x130, 'I',  0x131, 'i',
   0x134, 'J',  0x135, 'j',  0x136, 'K',  0x137, 'k',  0x138, 'K',
   0x139, 'L',  0x13a, 'l',  0x13b, 'L',  0x13c, 'l',  0x13d, 'L',
   0x13e, 'l',  0x13f, 'L',  0x140, 'l',  0x141, 'L',  0x142, 'l',
   0x143, 'N',  0x144, 'n',  0x145, 'N',  0x146, 'n',  0x147, 'N',
   0x148, 'n',  0x149, 'n',  0x14a, 'N',  0x14b, 'n',  0x14c, 'O',
   0x14d, 'o',  0x14e, 'O',  0x14f, 'o',  0x150, 'O',  0x151, 'o',
   0x154, 'R',  0x155, 'r',  0x156, 'R',  0x157, 'r',  0x158, 'R',
   0x159, 'r',  0x15a, 'S',  0x15b, 's',  0x15c, 'S',  0x15d, 's',
   0x15e, 'S',  0x15f, 's',  0x160, 'S',  0x161, 's',  0x162, 'T',
   0x163, 't',  0x164, 'T',  0x165, 't',  0x166, 'T',  0x167, 't',
   0x168, 'U',  0x169, 'u',  0x16a, 'U',  0x16b, 'u',  0x16c, 'U',
   0x16d, 'u',  0x16e, 'U',  0x16f, 'u',  0x170, 'U',  0x171, 'u',
   0x172, 'U',  0x173, 'u',  0x174, 'W',  0x175, 'w',  0x176, 'Y',
   0x177, 'y',  0x178, 'y',  0x179, 'Z',  0x17a, 'z',  0x17b, 'Z',
   0x17c, 'z',  0x17d, 'Z',  0x17e, 'z',  0
};



/* access via pointers so they can be changed by the user */
static AL_CONST unsigned short *codepage_table = _codepage_table;
static AL_CONST unsigned short *codepage_extras = _codepage_extras;



/* ascii_cp_getc:
 *  Reads a character from an ASCII codepage string.
 */
static int ascii_cp_getc(AL_CONST char *s)
{
   return codepage_table[*((unsigned char *)s)];
}



/* ascii_cp_getx:
 *  Reads from an ASCII codepage string, advancing pointer position.
 */
static int ascii_cp_getx(char **s)
{
   return codepage_table[*((unsigned char *)((*s)++))];
}



/* ascii_cp_setc:
 *  Sets a character in an ASCII codepage string.
 */
static int ascii_cp_setc(char *s, int c)
{
   int i;

   for (i=0; i<256; i++) {
      if (codepage_table[i] == c) {
	 *s = i;
	 return 1;
      }
   }

   if (codepage_extras) {
      for (i=0; codepage_extras[i]; i+=2) {
	 if (codepage_extras[i] == c) {
	    *s = codepage_extras[i+1];
	    return 1;
	 }
      } 
   }

   *s = '^';
   return 1;
}



/* ascii_cp_isok:
 *  Checks whether this character can be encoded in ASCII codepage format.
 */
static int ascii_cp_isok(int c)
{
   int i;

   for (i=0; i<256; i++) {
      if (codepage_table[i] == c)
	 return TRUE;
   }

   if (codepage_extras) {
      for (i=0; codepage_extras[i]; i+=2) {
	 if (codepage_extras[i] == c)
	    return TRUE;
      } 
   }

   return FALSE;
}



/* unicode_getc:
 *  Reads a character from a Unicode string.
 */
static int unicode_getc(AL_CONST char *s)
{
   return *((unsigned short *)s);
}



/* unicode_getx:
 *  Reads a character from a Unicode string, advancing the pointer position.
 */
static int unicode_getx(char **s)
{
   int c = *((unsigned short *)(*s));
   (*s) += sizeof(unsigned short);
   return c;
}



/* unicode_setc:
 *  Sets a character in a Unicode string.
 */
static int unicode_setc(char *s, int c)
{
   *((unsigned short *)s) = c;
   return sizeof(unsigned short);
}



/* unicode_width:
 *  Returns the width of a Unicode character.
 */
static int unicode_width(AL_CONST char *s)
{
   return sizeof(unsigned short);
}



/* unicode_cwidth:
 *  Returns the width of a Unicode character.
 */
static int unicode_cwidth(int c)
{
   return sizeof(unsigned short);
}



/* unicode_isok:
 *  Checks whether this character can be encoded in 16-bit Unicode format.
 */
static int unicode_isok(int c)
{
   return ((c >= 0) && (c <= 65535));
}



/* utf8_getc:
 *  Reads a character from a UTF-8 string.
 */
static int utf8_getc(AL_CONST char *s)
{
   int c = *((unsigned char *)(s++));
   int n, t;

   if (c & 0x80) {
      n = 1;
      while (c & (0x80>>n))
	 n++;

      c &= (1<<(8-n))-1;

      while (--n > 0) {
	 t = *((unsigned char *)(s++));

	 if ((!(t&0x80)) || (t&0x40))
	    return '^';

	 c = (c<<6) | (t&0x3F);
      }
   }

   return c;
}



/* utf8_getx:
 *  Reads a character from a UTF-8 string, advancing the pointer position.
 */
static int utf8_getx(char **s)
{
   int c = *((unsigned char *)((*s)++));
   int n, t;

   if (c & 0x80) {
      n = 1;
      while (c & (0x80>>n))
	 n++;

      c &= (1<<(8-n))-1;

      while (--n > 0) {
	 t = *((unsigned char *)((*s)++));

	 if ((!(t&0x80)) || (t&0x40)) {
	    (*s)--;
	    return '^';
	 }

	 c = (c<<6) | (t&0x3F);
      }
   }

   return c;
}



/* utf8_setc:
 *  Sets a character in a UTF-8 string.
 */
static int utf8_setc(char *s, int c)
{
   int size, bits, b, i;

   if (c < 128) {
      *s = c;
      return 1;
   }

   bits = 7;
   while (c >= (1<<bits))
      bits++;

   size = 2;
   b = 11;

   while (b < bits) {
      size++;
      b += 5;
   }

   b -= (7-size);
   s[0] = c>>b;

   for (i=0; i<size; i++)
      s[0] |= (0x80>>i);

   for (i=1; i<size; i++) {
      b -= 6;
      s[i] = 0x80 | ((c>>b)&0x3F);
   }

   return size;
}



/* utf8_width:
 *  Returns the width of a UTF-8 character.
 */
static int utf8_width(AL_CONST char *s)
{
   int c = *((unsigned char *)s);
   int n = 1;

   if (c & 0x80) {
      while (c & (0x80>>n))
	 n++;
   }

   return n;
}



/* utf8_cwidth:
 *  Returns the width of a UTF-8 character.
 */
static int utf8_cwidth(int c)
{
   int size, bits, b;

   if (c < 128)
      return 1;

   bits = 7;
   while (c >= (1<<bits))
      bits++;

   size = 2;
   b = 11;

   while (b < bits) {
      size++;
      b += 5;
   }

   return size;
}



/* utf8_isok:
 *  Checks whether this character can be encoded in UTF-8 format.
 */
static int utf8_isok(int c)
{
   return TRUE;
}



/* string format table, to allow user expansion with other encodings */
static UTYPE_INFO utypes[] =
{
   { U_ASCII,    ascii_getc,    ascii_getx,    ascii_setc,    ascii_width,   ascii_cwidth,   ascii_isok,    1    },
   { U_UTF8,     utf8_getc,     utf8_getx,     utf8_setc,     utf8_width,    utf8_cwidth,    utf8_isok,     4     },
   { U_UNICODE,  unicode_getc,  unicode_getx,  unicode_setc,  unicode_width, unicode_cwidth, unicode_isok,  2  },
   { U_ASCII_CP, ascii_cp_getc, ascii_cp_getx, ascii_cp_setc, ascii_width,   ascii_cwidth,   ascii_cp_isok, 1 },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     },
   { 0,          NULL,          NULL,          NULL,          NULL,          NULL,           NULL,          0                     }
};



/* current format information and worker routines */
static int utype = U_UTF8;

/* ugetc: */
int (*ugetc)(AL_CONST char *s) = utf8_getc;
/* ugetxc: */
int (*ugetx)(char **s) = utf8_getx;
/* ugetxc: */
int (*ugetxc)(AL_CONST char** s) = (int (*)(AL_CONST char**)) utf8_getx;
/* usetc: */
int (*usetc)(char *s, int c) = utf8_setc;
/* uwidth: */
int (*uwidth)(AL_CONST char *s) = utf8_width;
/* ucwidth: */
int (*ucwidth)(int c) = utf8_cwidth;
/* uisok: */
int (*uisok)(int c) = utf8_isok;



/* _find_utype:
 *  Helper for locating a string type description.
 */
UTYPE_INFO *_find_utype(int type)
{
   int i;

   if (type == U_CURRENT)
      type = utype;

   for (i=0; i<(int)(sizeof(utypes)/sizeof(UTYPE_INFO)); i++)
      if (utypes[i].id == type)
	 return &utypes[i];

   return NULL;
}



/* set_uformat:
 *  Selects a new text encoding format.
 */
void set_uformat(int type)
{
   UTYPE_INFO *info = _find_utype(type);

   if (info) {
      utype = info->id;
      ugetc = info->u_getc;
      ugetx = (int (*)(char **)) info->u_getx;
      ugetxc = (int (*)(AL_CONST char **)) info->u_getx;
      usetc = info->u_setc;
      uwidth = info->u_width;
      ucwidth = info->u_cwidth;
      uisok = info->u_isok;
   }
}



/* get_uformat:
 *  Returns the current text encoding format.
 */
int get_uformat(void)
{
   return utype;
}



/* register_uformat:
 *  Allows the user to hook in custom routines for supporting a new string
 *  encoding format.
 */
void register_uformat(int type, int (*ugetc)(AL_CONST char *s), int (*ugetx)(char **s), int (*usetc)(char *s, int c), int (*uwidth)(AL_CONST char *s), int (*ucwidth)(int c), int (*uisok)(int c), int uwidth_max)
{
   UTYPE_INFO *info = _find_utype(type);

   if (!info)
      info = _find_utype(0);

   if (info) {
      info->id = type;
      info->u_getc = ugetc;
      info->u_getx = ugetx;
      info->u_setc = usetc;
      info->u_width = uwidth;
      info->u_cwidth = ucwidth;
      info->u_isok = uisok;
      info->u_width_max = uwidth_max;
   }
}



/* set_ucodepage:
 *  Sets lookup table data for the codepage conversion functions.
 */
void set_ucodepage(AL_CONST unsigned short *table, AL_CONST unsigned short *extras)
{
   ASSERT(table);
   codepage_table = table;
   codepage_extras = extras;
}



/* need_uconvert:
 *  Decides whether a conversion is required to make this string be in the
 *  new type. No conversion will be needed if both types are the same, or
 *  when going from ASCII <-> UTF8 where the data is 7-bit clean.
 */
int need_uconvert(AL_CONST char *s, int type, int newtype)
{
   int c;
   ASSERT(s);
   
   if (type == U_CURRENT)
      type = utype;

   if (newtype == U_CURRENT)
      newtype = utype;

   if (type == newtype)
      return FALSE;

   if (((type == U_ASCII) || (type == U_UTF8)) && ((newtype == U_ASCII) || (newtype == U_UTF8))) {
      do {
	 c = *((unsigned char *)(s++));
	 if (!c)
	    return FALSE;
      } while (c <= 127);
   }

   return TRUE;
}



/* uconvert_size:
 *  Returns the number of bytes this string will occupy when converted to
 *  the specified type.
 */
int uconvert_size(AL_CONST char *s, int type, int newtype)
{
   UTYPE_INFO *info, *outfo;
   int size = 0;
   int c;
   ASSERT(s);

   info = _find_utype(type);
   if (!info)
      return 0;

   outfo = _find_utype(newtype);
   if (!outfo)
      return 0;

   size = 0;

   while ((c = info->u_getx((char **)&s)) != 0)
      size += outfo->u_cwidth(c);

   return size + outfo->u_cwidth(0);
}



/* do_uconvert:
 *  Converts a string from one format to another.
 */
void do_uconvert(AL_CONST char *s, int type, char *buf, int newtype, int size)
{
   UTYPE_INFO *info, *outfo;
   int pos = 0;
   int c;
   ASSERT(s);
   ASSERT(buf);
   ASSERT(size > 0);

   info = _find_utype(type);
   if (!info)
      return;

   outfo = _find_utype(newtype);
   if (!outfo)
      return;

   size -= outfo->u_cwidth(0);
   ASSERT(size >= 0);

   while ((c = info->u_getx((char**)&s)) != 0) {
      if (!outfo->u_isok(c))
	 c = '^';

      size -= outfo->u_cwidth(c);
      if (size < 0)
	 break;

      pos += outfo->u_setc(buf+pos, c);
   }

   outfo->u_setc(buf+pos, 0);
}



/* uconvert:
 *  Higher level version of do_uconvert(). This routine is intelligent
 *  enough to just return a copy of the input string if no conversion
 *  is required, and to use an internal static buffer if no user buffer
 *  is provided.
 */
char *uconvert(AL_CONST char *s, int type, char *buf, int newtype, int size)
{
   static char static_buf[1024];
   ASSERT(s);
   ASSERT(size >= 0);

   if (!need_uconvert(s, type, newtype))
      return (char *)s;

   if (!buf) {
      buf = static_buf;
      size = sizeof(static_buf);
   }

   do_uconvert(s, type, buf, newtype, size);
   return buf;
}



/* uoffset:
 *  Returns the offset in bytes from the start of the string to the 
 *  character at the specified index. If the index is negative, counts
 *  backward from the end of the string (-1 returns an offset to the
 *  last character).
 */
int uoffset(AL_CONST char *s, int index)
{
   AL_CONST char *orig = s;
   AL_CONST char *last;
   ASSERT(s);

   if (index < 0)
      index += ustrlen(s);

   while (index-- > 0) {
      last = s;
      if (!ugetxc(&s)) {
	 s = last;
	 break;
      }
   }

   return (long)s - (long)orig;
}



/* ugetat:
 *  Returns the character from the specified index within the string.
 */
int ugetat(AL_CONST char *s, int index)
{
   ASSERT(s);
   return ugetc(s + uoffset(s, index));
}



/* usetat:
 *  Modifies the character at the specified index within the string,
 *  handling adjustments for variable width data. Returns how far the
 *  rest of the string was moved.
 */
int usetat(char *s, int index, int c)
{
   int oldw, neww;
   ASSERT(s);

   s += uoffset(s, index);

   oldw = uwidth(s);
   neww = ucwidth(c);

   if (oldw != neww)
      memmove(s+neww, s+oldw, ustrsizez(s+oldw));

   usetc(s, c);

   return neww-oldw;
}



/* uinsert:
 *  Inserts a character at the specified index within a string, sliding
 *  following data along to make room. Returns how far the data was moved.
 */
int uinsert(char *s, int index, int c)
{
   int w = ucwidth(c);
   ASSERT(s);

   s += uoffset(s, index);
   memmove(s+w, s, ustrsizez(s));
   usetc(s, c);

   return w;
}



/* uremove:
 *  Removes the character at the specified index within the string, sliding
 *  following data back to make room. Returns how far the data was moved.
 */
int uremove(char *s, int index)
{
   int w;
   ASSERT(s);

   s += uoffset(s, index);
   w = uwidth(s);
   memmove(s, s+w, ustrsizez(s+w));

   return -w;
}



/* uwidth_max:
 *  Returns the largest possible size of a character in the specified
 *  encoding format, in bytes.
 */
int uwidth_max(int type)
{
   UTYPE_INFO *info;

   info = _find_utype(type);
   if (!info)
      return 0;

   return info->u_width_max;
}



/* utolower:
 *  Unicode-aware version of the ANSI tolower() function.
 */
int utolower(int c)
{
   if ((c >= 65 && c <= 90) ||
       (c >= 192 && c <= 214) ||
       (c >= 216 && c <= 222) ||
       (c >= 913 && c <= 929) ||
       (c >= 931 && c <= 939) ||
       (c >= 1040 && c <= 1071))
      return c + 32;
   if ((c >= 393 && c <= 394))
      return c + 205;
   if ((c >= 433 && c <= 434))
      return c + 217;
   if ((c >= 904 && c <= 906))
      return c + 37;
   if ((c >= 910 && c <= 911))
      return c + 63;
   if ((c >= 1025 && c <= 1036) ||
       (c >= 1038 && c <= 1039))
      return c + 80;
   if ((c >= 1329 && c <= 1366) ||
       (c >= 4256 && c <= 4293))
      return c + 48;
   if ((c >= 7944 && c <= 7951) ||
       (c >= 7960 && c <= 7965) ||
       (c >= 7976 && c <= 7983) ||
       (c >= 7992 && c <= 7999) ||
       (c >= 8008 && c <= 8013) ||
       (c >= 8040 && c <= 8047) ||
       (c >= 8072 && c <= 8079) ||
       (c >= 8088 && c <= 8095) ||
       (c >= 8104 && c <= 8111) ||
       (c >= 8120 && c <= 8121) ||
       (c >= 8152 && c <= 8153) ||
       (c >= 8168 && c <= 8169))
      return c + -8;
   if ((c >= 8122 && c <= 8123))
      return c + -74;
   if ((c >= 8136 && c <= 8139))
      return c + -86;
   if ((c >= 8154 && c <= 8155))
      return c + -100;
   if ((c >= 8170 && c <= 8171))
      return c + -112;
   if ((c >= 8184 && c <= 8185))
      return c + -128;
   if ((c >= 8186 && c <= 8187))
      return c + -126;
   if ((c >= 8544 && c <= 8559))
      return c + 16;
   if ((c >= 9398 && c <= 9423))
      return c + 26;

   switch (c) {
      case 256:
      case 258:
      case 260:
      case 262:
      case 264:
      case 266:
      case 268:
      case 270:
      case 272:
      case 274:
      case 276:
      case 278:
      case 280:
      case 282:
      case 284:
      case 286:
      case 288:
      case 290:
      case 292:
      case 294:
      case 296:
      case 298:
      case 300:
      case 302:
      case 306:
      case 308:
      case 310:
      case 313:
      case 315:
      case 317:
      case 319:
      case 321:
      case 323:
      case 325:
      case 327:
      case 330:
      case 332:
      case 334:
      case 336:
      case 338:
      case 340:
      case 342:
      case 344:
      case 346:
      case 348:
      case 350:
      case 352:
      case 354:
      case 356:
      case 358:
      case 360:
      case 362:
      case 364:
      case 366:
      case 368:
      case 370:
      case 372:
      case 374:
      case 377:
      case 379:
      case 381:
      case 386:
      case 388:
      case 391:
      case 395:
      case 401:
      case 408:
      case 416:
      case 418:
      case 420:
      case 423:
      case 428:
      case 431:
      case 435:
      case 437:
      case 440:
      case 444:
      case 453:
      case 456:
      case 459:
      case 461:
      case 463:
      case 465:
      case 467:
      case 469:
      case 471:
      case 473:
      case 475:
      case 478:
      case 480:
      case 482:
      case 484:
      case 486:
      case 488:
      case 490:
      case 492:
      case 494:
      case 498:
      case 500:
      case 506:
      case 508:
      case 510:
      case 512:
      case 514:
      case 516:
      case 518:
      case 520:
      case 522:
      case 524:
      case 526:
      case 528:
      case 530:
      case 532:
      case 534:
      case 994:
      case 996:
      case 998:
      case 1000:
      case 1002:
      case 1004:
      case 1006:
      case 1120:
      case 1122:
      case 1124:
      case 1126:
      case 1128:
      case 1130:
      case 1132:
      case 1134:
      case 1136:
      case 1138:
      case 1140:
      case 1142:
      case 1144:
      case 1146:
      case 1148:
      case 1150:
      case 1152:
      case 1168:
      case 1170:
      case 1172:
      case 1174:
      case 1176:
      case 1178:
      case 1180:
      case 1182:
      case 1184:
      case 1186:
      case 1188:
      case 1190:
      case 1192:
      case 1194:
      case 1196:
      case 1198:
      case 1200:
      case 1202:
      case 1204:
      case 1206:
      case 1208:
      case 1210:
      case 1212:
      case 1214:
      case 1217:
      case 1219:
      case 1223:
      case 1227:
      case 1232:
      case 1234:
      case 1236:
      case 1238:
      case 1240:
      case 1242:
      case 1244:
      case 1246:
      case 1248:
      case 1250:
      case 1252:
      case 1254:
      case 1256:
      case 1258:
      case 1262:
      case 1264:
      case 1266:
      case 1268:
      case 1272:
      case 7680:
      case 7682:
      case 7684:
      case 7686:
      case 7688:
      case 7690:
      case 7692:
      case 7694:
      case 7696:
      case 7698:
      case 7700:
      case 7702:
      case 7704:
      case 7706:
      case 7708:
      case 7710:
      case 7712:
      case 7714:
      case 7716:
      case 7718:
      case 7720:
      case 7722:
      case 7724:
      case 7726:
      case 7728:
      case 7730:
      case 7732:
      case 7734:
      case 7736:
      case 7738:
      case 7740:
      case 7742:
      case 7744:
      case 7746:
      case 7748:
      case 7750:
      case 7752:
      case 7754:
      case 7756:
      case 7758:
      case 7760:
      case 7762:
      case 7764:
      case 7766:
      case 7768:
      case 7770:
      case 7772:
      case 7774:
      case 7776:
      case 7778:
      case 7780:
      case 7782:
      case 7784:
      case 7786:
      case 7788:
      case 7790:
      case 7792:
      case 7794:
      case 7796:
      case 7798:
      case 7800:
      case 7802:
      case 7804:
      case 7806:
      case 7808:
      case 7810:
      case 7812:
      case 7814:
      case 7816:
      case 7818:
      case 7820:
      case 7822:
      case 7824:
      case 7826:
      case 7828:
      case 7840:
      case 7842:
      case 7844:
      case 7846:
      case 7848:
      case 7850:
      case 7852:
      case 7854:
      case 7856:
      case 7858:
      case 7860:
      case 7862:
      case 7864:
      case 7866:
      case 7868:
      case 7870:
      case 7872:
      case 7874:
      case 7876:
      case 7878:
      case 7880:
      case 7882:
      case 7884:
      case 7886:
      case 7888:
      case 7890:
      case 7892:
      case 7894:
      case 7896:
      case 7898:
      case 7900:
      case 7902:
      case 7904:
      case 7906:
      case 7908:
      case 7910:
      case 7912:
      case 7914:
      case 7916:
      case 7918:
      case 7920:
      case 7922:
      case 7924:
      case 7926:
      case 7928:
	   return c + 1;
      case 304:
	   return c + -199;
      case 376:
	   return c + -121;
      case 385:
	   return c + 210;
      case 390:
	   return c + 206;
      case 398:
	   return c + 79;
      case 399:
	   return c + 202;
      case 400:
	   return c + 203;
      case 403:
	   return c + 205;
      case 404:
	   return c + 207;
      case 406:
      case 412:
	   return c + 211;
      case 407:
	   return c + 209;
      case 413:
	   return c + 213;
      case 415:
	   return c + 214;
      case 422:
      case 425:
      case 430:
	   return c + 218;
      case 439:
	   return c + 219;
      case 452:
      case 455:
      case 458:
      case 497:
	   return c + 2;
      case 902:
	   return c + 38;
      case 908:
	   return c + 64;
      case 8025:
      case 8027:
      case 8029:
      case 8031:
	   return c + -8;
      case 8124:
      case 8140:
      case 8188:
	   return c + -9;
      case 8172:
	   return c + -7;
      default:
	   return c;
   }
}



/* utoupper:
 *  Unicode-aware version of the ANSI toupper() function.
 */
int utoupper(int c)
{
   if ((c >= 97 && c <= 122) ||
       (c >= 224 && c <= 246) ||
       (c >= 248 && c <= 254) ||
       (c >= 945 && c <= 961) ||
       (c >= 963 && c <= 971) ||
       (c >= 1072 && c <= 1103))
      return c + -32;
   if ((c >= 598 && c <= 599))
      return c + -205;
   if ((c >= 650 && c <= 651))
      return c + -217;
   if ((c >= 941 && c <= 943))
      return c + -37;
   if ((c >= 973 && c <= 974))
      return c + -63;
   if ((c >= 1105 && c <= 1116) ||
       (c >= 1118 && c <= 1119))
      return c + -80;
   if ((c >= 1377 && c <= 1414))
      return c + -48;
   if ((c >= 7936 && c <= 7943) ||
       (c >= 7952 && c <= 7957) ||
       (c >= 7968 && c <= 7975) ||
       (c >= 7984 && c <= 7991) ||
       (c >= 8000 && c <= 8005) ||
       (c >= 8032 && c <= 8039) ||
       (c >= 8064 && c <= 8071) ||
       (c >= 8080 && c <= 8087) ||
       (c >= 8096 && c <= 8103) ||
       (c >= 8112 && c <= 8113) ||
       (c >= 8144 && c <= 8145) ||
       (c >= 8160 && c <= 8161))
      return c + 8;
   if ((c >= 8048 && c <= 8049))
      return c + 74;
   if ((c >= 8050 && c <= 8053))
      return c + 86;
   if ((c >= 8054 && c <= 8055))
      return c + 100;
   if ((c >= 8056 && c <= 8057))
      return c + 128;
   if ((c >= 8058 && c <= 8059))
      return c + 112;
   if ((c >= 8060 && c <= 8061))
      return c + 126;
   if ((c >= 8560 && c <= 8575))
      return c + -16;
   if ((c >= 9424 && c <= 9449))
      return c + -26;

   switch (c) {
      case 255:
	   return c + 121;
      case 257:
      case 259:
      case 261:
      case 263:
      case 265:
      case 267:
      case 269:
      case 271:
      case 273:
      case 275:
      case 277:
      case 279:
      case 281:
      case 283:
      case 285:
      case 287:
      case 289:
      case 291:
      case 293:
      case 295:
      case 297:
      case 299:
      case 301:
      case 303:
      case 307:
      case 309:
      case 311:
      case 314:
      case 316:
      case 318:
      case 320:
      case 322:
      case 324:
      case 326:
      case 328:
      case 331:
      case 333:
      case 335:
      case 337:
      case 339:
      case 341:
      case 343:
      case 345:
      case 347:
      case 349:
      case 351:
      case 353:
      case 355:
      case 357:
      case 359:
      case 361:
      case 363:
      case 365:
      case 367:
      case 369:
      case 371:
      case 373:
      case 375:
      case 378:
      case 380:
      case 382:
      case 387:
      case 389:
      case 392:
      case 396:
      case 402:
      case 409:
      case 417:
      case 419:
      case 421:
      case 424:
      case 429:
      case 432:
      case 436:
      case 438:
      case 441:
      case 445:
      case 453:
      case 456:
      case 459:
      case 462:
      case 464:
      case 466:
      case 468:
      case 470:
      case 472:
      case 474:
      case 476:
      case 479:
      case 481:
      case 483:
      case 485:
      case 487:
      case 489:
      case 491:
      case 493:
      case 495:
      case 498:
      case 501:
      case 507:
      case 509:
      case 511:
      case 513:
      case 515:
      case 517:
      case 519:
      case 521:
      case 523:
      case 525:
      case 527:
      case 529:
      case 531:
      case 533:
      case 535:
      case 995:
      case 997:
      case 999:
      case 1001:
      case 1003:
      case 1005:
      case 1007:
      case 1121:
      case 1123:
      case 1125:
      case 1127:
      case 1129:
      case 1131:
      case 1133:
      case 1135:
      case 1137:
      case 1139:
      case 1141:
      case 1143:
      case 1145:
      case 1147:
      case 1149:
      case 1151:
      case 1153:
      case 1169:
      case 1171:
      case 1173:
      case 1175:
      case 1177:
      case 1179:
      case 1181:
      case 1183:
      case 1185:
      case 1187:
      case 1189:
      case 1191:
      case 1193:
      case 1195:
      case 1197:
      case 1199:
      case 1201:
      case 1203:
      case 1205:
      case 1207:
      case 1209:
      case 1211:
      case 1213:
      case 1215:
      case 1218:
      case 1220:
      case 1224:
      case 1228:
      case 1233:
      case 1235:
      case 1237:
      case 1239:
      case 1241:
      case 1243:
      case 1245:
      case 1247:
      case 1249:
      case 1251:
      case 1253:
      case 1255:
      case 1257:
      case 1259:
      case 1263:
      case 1265:
      case 1267:
      case 1269:
      case 1273:
      case 7681:
      case 7683:
      case 7685:
      case 7687:
      case 7689:
      case 7691:
      case 7693:
      case 7695:
      case 7697:
      case 7699:
      case 7701:
      case 7703:
      case 7705:
      case 7707:
      case 7709:
      case 7711:
      case 7713:
      case 7715:
      case 7717:
      case 7719:
      case 7721:
      case 7723:
      case 7725:
      case 7727:
      case 7729:
      case 7731:
      case 7733:
      case 7735:
      case 7737:
      case 7739:
      case 7741:
      case 7743:
      case 7745:
      case 7747:
      case 7749:
      case 7751:
      case 7753:
      case 7755:
      case 7757:
      case 7759:
      case 7761:
      case 7763:
      case 7765:
      case 7767:
      case 7769:
      case 7771:
      case 7773:
      case 7775:
      case 7777:
      case 7779:
      case 7781:
      case 7783:
      case 7785:
      case 7787:
      case 7789:
      case 7791:
      case 7793:
      case 7795:
      case 7797:
      case 7799:
      case 7801:
      case 7803:
      case 7805:
      case 7807:
      case 7809:
      case 7811:
      case 7813:
      case 7815:
      case 7817:
      case 7819:
      case 7821:
      case 7823:
      case 7825:
      case 7827:
      case 7829:
      case 7841:
      case 7843:
      case 7845:
      case 7847:
      case 7849:
      case 7851:
      case 7853:
      case 7855:
      case 7857:
      case 7859:
      case 7861:
      case 7863:
      case 7865:
      case 7867:
      case 7869:
      case 7871:
      case 7873:
      case 7875:
      case 7877:
      case 7879:
      case 7881:
      case 7883:
      case 7885:
      case 7887:
      case 7889:
      case 7891:
      case 7893:
      case 7895:
      case 7897:
      case 7899:
      case 7901:
      case 7903:
      case 7905:
      case 7907:
      case 7909:
      case 7911:
      case 7913:
      case 7915:
      case 7917:
      case 7919:
      case 7921:
      case 7923:
      case 7925:
      case 7927:
      case 7929:
	   return c + -1;
      case 305:
	   return c + -232;
      case 383:
	   return c + -300;
      case 454:
      case 457:
      case 460:
      case 499:
	   return c + -2;
      case 477:
      case 1010:
	   return c + -79;
      case 595:
	   return c + -210;
      case 596:
	   return c + -206;
      case 601:
	   return c + -202;
      case 603:
	   return c + -203;
      case 608:
	   return c + -205;
      case 611:
	   return c + -207;
      case 616:
	   return c + -209;
      case 617:
      case 623:
	   return c + -211;
      case 626:
	   return c + -213;
      case 629:
	   return c + -214;
      case 640:
      case 643:
      case 648:
	   return c + -218;
      case 658:
	   return c + -219;
      case 837:
	   return c + 84;
      case 940:
	   return c + -38;
      case 962:
	   return c + -31;
      case 972:
	   return c + -64;
      case 976:
	   return c + -62;
      case 977:
	   return c + -57;
      case 981:
	   return c + -47;
      case 982:
	   return c + -54;
      case 1008:
	   return c + -86;
      case 1009:
	   return c + -80;
      case 7835:
	   return c + -59;
      case 8017:
      case 8019:
      case 8021:
      case 8023:
	   return c + 8;
      case 8115:
      case 8131:
      case 8179:
	   return c + 9;
      case 8126:
	   return c + -7205;
      case 8165:
	   return c + 7;
      default:
	   return c;
   }
}



/* uisspace:
 *  Unicode-aware version of the ANSI isspace() function.
 */
int uisspace(int c)
{
   return ((c == ' ') || (c == '\t') || (c == '\r') || 
	   (c == '\n') || (c == '\f') || (c == '\v') ||
	   (c == 0x1680) || ((c >= 0x2000) && (c <= 0x200A)) ||
	   (c == 0x2028) || (c == 0x202f) || (c == 0x3000));
}



/* uisdigit:
 *  Unicode-aware version of the ANSI isdigit() function.
 */
int uisdigit(int c)
{
   return ((c >= '0') && (c <= '9'));
}



/* ustrdup:
 *  Returns a newly allocated copy of the src string, which must later be
 *  freed by the caller.  This function is for external use by Allegro
 *  clients only.  Internal code should use _al_ustrdup().
 */
char *_ustrdup(AL_CONST char *src, AL_METHOD(void *, malloc_func, (size_t)))
{
   char *s;
   int size;
   ASSERT(src);
   ASSERT(malloc_func);

   size = ustrsizez(src);

   s = malloc_func(size);
   if (s)
      ustrzcpy(s, size, src);
   else
      *allegro_errno = ENOMEM;

   return s;
}



/* _al_ustrdup:
 *  Returns a newly allocated copy of the src string, which must later be
 *  freed by the caller using _AL_FREE().  This function is for internal use
 *  by Allegro clients only.  External code should use ustrdup().
 */
char *_al_ustrdup(AL_CONST char *src)
{
   int size;
   char *newstring;
   ASSERT(src);

   size = ustrsizez(src);
   newstring = _AL_MALLOC(size);

   if (newstring)
      ustrzcpy(newstring, size, src);

   return newstring;
}



/* ustrsize:
 *  Returns the size of the specified string in bytes, not including the
 *  trailing zero.
 */
int ustrsize(AL_CONST char *s)
{
   AL_CONST char *orig = s;
   AL_CONST char *last;
   ASSERT(s);

   do {
      last = s;
   } while (ugetxc(&s) != 0);

   return (long)last - (long)orig;
}



/* ustrsizez:
 *  Returns the size of the specified string in bytes, including the
 *  trailing zero.
 */
int ustrsizez(AL_CONST char *s)
{
   AL_CONST char *orig = s;
   ASSERT(s);

   do {
   } while (ugetxc(&s) != 0);

   return (long)s - (long)orig;
}



/* ustrzcpy:
 *  Enhanced Unicode-aware version of the ANSI strcpy() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strcpy() is defined as:
 *   #define ustrcpy(dest, src) ustrzcpy(dest, INT_MAX, src)
 */
char *ustrzcpy(char *dest, int size, AL_CONST char *src)
{
   int pos = 0;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size > 0);

   size -= ucwidth(0);
   ASSERT(size >= 0);

   while ((c = ugetxc(&src)) != 0) {
      size -= ucwidth(c);
      if (size < 0)
         break;

      pos += usetc(dest+pos, c);
   }

   usetc(dest+pos, 0);

   return dest;
}



/* ustrzcat:
 *  Enhanced Unicode-aware version of the ANSI strcat() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strcat() is defined as:
 *   #define ustrcat(dest, src) ustrzcat(dest, INT_MAX, src)
 */
char *ustrzcat(char *dest, int size, AL_CONST char *src)
{
   int pos;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size > 0);

   pos = ustrsize(dest);
   size -= pos + ucwidth(0);
   ASSERT(size >= 0);

   while ((c = ugetxc(&src)) != 0) {
      size -= ucwidth(c);
      if (size < 0)
         break;

      pos += usetc(dest+pos, c);
   }

   usetc(dest+pos, 0);

   return dest;
}



/* ustrlen:
 *  Unicode-aware version of the ANSI strlen() function.
 */
int ustrlen(AL_CONST char *s)
{
   int c = 0;
   ASSERT(s);

   while (ugetxc(&s))
      c++;

   return c;
}



/* ustrcmp:
 *  Unicode-aware version of the ANSI strcmp() function.
 */
int ustrcmp(AL_CONST char *s1, AL_CONST char *s2)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   for (;;) {
      c1 = ugetxc(&s1);
      c2 = ugetxc(&s2);

      if (c1 != c2)
	 return c1 - c2;

      if (!c1)
	 return 0;
   }
}



/* ustrzncpy:
 *  Enhanced Unicode-aware version of the ANSI strncpy() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strncpy() is defined as:
 *   #define ustrncpy(dest, src, n) ustrzncpy(dest, INT_MAX, src, n)
 */
char *ustrzncpy(char *dest, int size, AL_CONST char *src, int n)
{
   int pos = 0, len = 0;
   int ansi_oddness = FALSE;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size > 0);
   ASSERT(n >= 0);

   /* detect raw ustrncpy() call */
   if (size == INT_MAX)
      ansi_oddness = TRUE;

   size -= ucwidth(0);
   ASSERT(size >= 0);

   /* copy at most n characters */
   while (((c = ugetxc(&src)) != 0) && (len < n)) {
      size -= ucwidth(c);
      if (size<0)
         break;

      pos += usetc(dest+pos, c);
      len++;
   }

   /* pad with NULL characters */
   while (len < n) {
      size -= ucwidth(0);
      if (size < 0)
         break;

      pos += usetc(dest+pos, 0);
      len++;
   }

   /* ANSI C doesn't append the terminating NULL character */
   if (!ansi_oddness)
      usetc(dest+pos, 0);

   return dest;
}



/* ustrzncat:
 *  Enhanced Unicode-aware version of the ANSI strncat() function
 *  that can handle the size (in bytes) of the destination string.
 *  The raw Unicode-aware version of ANSI strncat() is defined as:
 *   #define ustrncat(dest, src, n) ustrzncat(dest, INT_MAX, src, n)
 */
char *ustrzncat(char *dest, int size, AL_CONST char *src, int n)
{
   int pos, len = 0;
   int c;
   ASSERT(dest);
   ASSERT(src);
   ASSERT(size >= 0);
   ASSERT(n >= 0);

   pos = ustrsize(dest);
   size -= pos + ucwidth(0);

   while (((c = ugetxc(&src)) != 0) && (len < n)) {
      size -= ucwidth(c);
      if (size<0)
         break;

      pos += usetc(dest+pos, c);
      len++;
   }

   usetc(dest+pos, 0);

   return dest;
}



/* ustrncmp:
 *  Unicode-aware version of the ANSI strncmp() function.
 */
int ustrncmp(AL_CONST char *s1, AL_CONST char *s2, int n)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   if (n <= 0)
      return 0;

   for (;;) {
      c1 = ugetxc(&s1);
      c2 = ugetxc(&s2);

      if (c1 != c2)
	 return c1 - c2;

      if ((!c1) || (--n <= 0))
	 return 0;
   }
}



/* ustricmp:
 *  Unicode-aware version of the DJGPP stricmp() function.
 */
int ustricmp(AL_CONST char *s1, AL_CONST char *s2)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   for (;;) {
      c1 = utolower(ugetxc(&s1));
      c2 = utolower(ugetxc(&s2));

      if (c1 != c2)
	 return c1 - c2;

      if (!c1)
	 return 0;
   }
}



/* ustrnicmp:
 *  Unicode-aware version of the DJGPP strnicmp() function.
 */
int ustrnicmp(AL_CONST char *s1, AL_CONST char *s2, int n)
{
   int c1, c2;
   ASSERT(s1);
   ASSERT(s2);

   if (n <= 0)
      return 0;

   for (;;) {
      c1 = utolower(ugetxc(&s1));
      c2 = utolower(ugetxc(&s2));

      if (c1 != c2)
         return c1 - c2;

      if ((!c1) || (--n <= 0))
         return 0;
   }
}



/* ustrlwr:
 *  Unicode-aware version of the ANSI strlwr() function.
 */
char *ustrlwr(char *s)
{
   int pos = 0;
   int c, lc;
   ASSERT(s);

   while ((c = ugetc(s+pos)) != 0) {
      lc = utolower(c);

      if (lc != c)
	 usetat(s+pos, 0, lc);

      pos += uwidth(s+pos);
   }

   return s;
}



/* ustrupr:
 *  Unicode-aware version of the ANSI strupr() function.
 */
char *ustrupr(char *s)
{
   int pos = 0;
   int c, uc;
   ASSERT(s);

   while ((c = ugetc(s+pos)) != 0) {
      uc = utoupper(c);

      if (uc != c)
	 usetat(s+pos, 0, uc);

      pos += uwidth(s+pos);
   }

   return s;
}



/* ustrchr:
 *  Unicode-aware version of the ANSI strchr() function.
 */
char *ustrchr(AL_CONST char *s, int c)
{
   int d;
   ASSERT(s);

   while ((d = ugetc(s)) != 0) {
      if (c == d)
	 return (char *)s;

      s += uwidth(s);
   }

   if (!c)
      return (char *)s;

   return NULL;
}



/* ustrrchr:
 *  Unicode-aware version of the ANSI strrchr() function.
 */
char *ustrrchr(AL_CONST char *s, int c)
{
   AL_CONST char *last_match = NULL;
   int c1, pos = 0;
   ASSERT(s);

   for (c1=ugetc(s); c1; c1=ugetc(s+pos)) {
      if (c1 == c)
	 last_match = s + pos;

      pos += ucwidth(c1);
   }

   return (char *)last_match;
}



/* ustrstr:
 *  Unicode-aware version of the ANSI strstr() function.
 */
char *ustrstr(AL_CONST char *s1, AL_CONST char *s2)
{
   int len;
   ASSERT(s1);
   ASSERT(s2);

   len = ustrlen(s2);
   while (ugetc(s1)) {
      if (ustrncmp(s1, s2, len) == 0)
	 return (char *)s1;

      s1 += uwidth(s1);
   }

   return NULL;
}



/* ustrpbrk:
 *  Unicode-aware version of the ANSI strpbrk() function.
 */
char *ustrpbrk(AL_CONST char *s, AL_CONST char *set)
{
   AL_CONST char *setp;
   int c, d;
   ASSERT(s);
   ASSERT(set);

   while ((c = ugetc(s)) != 0) {
      setp = set;

      while ((d = ugetxc(&setp)) != 0) {
	 if (c == d)
	    return (char *)s;
      }

      s += uwidth(s);
   }

   return NULL;
}



/* ustrtok:
 *  Unicode-aware version of the ANSI strtok() function.
 */
char *ustrtok(char *s, AL_CONST char *set)
{
   static char *last = NULL;

   return ustrtok_r(s, set, &last);
}



/* ustrtok_r:
 *  Unicode-aware version of the strtok_r() function.
 */
char *ustrtok_r(char *s, AL_CONST char *set, char **last)
{
   char *prev_str, *tok;
   AL_CONST char *setp;
   int c, sc;

   ASSERT(last);

   if (!s) {
      s = *last;

      if (!s)
	 return NULL;
   }

   skip_leading_delimiters:

   prev_str = s;
   c = ugetx(&s);

   setp = set;

   while ((sc = ugetxc(&setp)) != 0) {
      if (c == sc)
	 goto skip_leading_delimiters;
   }

   if (!c) {
      *last = NULL;
      return NULL;
   }

   tok = prev_str;

   for (;;) {
      prev_str = s;
      c = ugetx(&s);

      setp = set;

      do {
	 sc = ugetxc(&setp);
	 if (sc == c) {
	    if (!c) {
	       *last = NULL;
	       return tok;
	    }
	    else {
	       s += usetat(prev_str, 0, 0);
	       *last = s;
	       return tok;
	    }
	 }
      } while (sc);
   }
}



/* uatof:
 *  Unicode-aware version of the ANSI atof() function. No need to bother
 *  implementing this ourselves, since all floating point numbers are
 *  valid ASCII in any case.
 */
double uatof(AL_CONST char *s)
{
   char tmp[64];
   ASSERT(s);
   return atof(uconvert_toascii(s, tmp));
}



/* ustrtol:
 *  Unicode-aware version of the ANSI strtol() function. Note the
 *  nicely bodged implementation :-)
 */
long ustrtol(AL_CONST char *s, char **endp, int base)
{
   char tmp[64];
   char *myendp;
   long ret;
   char *t;
   ASSERT(s);

   t = uconvert_toascii(s, tmp);

   ret = strtol(t, &myendp, base);

   if (endp)
      *endp = (char *)s + uoffset(s, (long)myendp - (long)t);

   return ret;
}



/* ustrtod:
 *  Unicode-aware version of the ANSI strtod() function. Note the
 *  nicely bodged implementation :-)
 */
double ustrtod(AL_CONST char *s, char **endp)
{
   char tmp[64];
   char *myendp;
   double ret;
   char *t;
   ASSERT(s);

   t = uconvert_toascii(s, tmp);

   ret = strtod(t, &myendp);

   if (endp)
      *endp = (char *)s + uoffset(s, (long)myendp - (long)t);

   return ret;
}



/* ustrerror:
 *  Fetch the error description from the OS and convert to Unicode.
 */
AL_CONST char *ustrerror(int err)
{
   AL_CONST char *s = strerror(err);
   static char tmp[1024];
   return uconvert_ascii(s, tmp);
}



/*******************************************************************/
/***             Unicode-aware sprintf() functions               ***/
/*******************************************************************/


/* information about the current format conversion mode */
typedef struct SPRINT_INFO
{
   int flags;
   int field_width;
   int precision;
   int num_special;
} SPRINT_INFO;



#define SPRINT_FLAG_LEFT_JUSTIFY             1
#define SPRINT_FLAG_FORCE_PLUS_SIGN          2
#define SPRINT_FLAG_FORCE_SPACE              4
#define SPRINT_FLAG_ALTERNATE_CONVERSION     8
#define SPRINT_FLAG_PAD_ZERO                 16
#define SPRINT_FLAG_SHORT_INT                32
#define SPRINT_FLAG_LONG_INT                 64
#define SPRINT_FLAG_LONG_DOUBLE              128
#define SPRINT_FLAG_LONG_LONG                256



/* decoded string argument type */
typedef struct STRING_ARG
{
   char *data;
   int size;  /* in bytes without the terminating '\0' */
   struct STRING_ARG *next;
} STRING_ARG;



/* LONGEST:
 *  64-bit integers on platforms that support it, 32-bit otherwise.
 */
#ifdef LONG_LONG
   #define LONGEST LONG_LONG
#else
   #define LONGEST long
#endif



/* va_int:
 *  Helper for reading an integer from the varargs list.
 */
#ifdef LONG_LONG

   #define va_int(args, flags)               \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, signed int)            \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_LONG) ?   \
	 va_arg(args, signed LONG_LONG)      \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, signed long int)       \
      :                                      \
	 va_arg(args, signed int)))          \
   )

#else

   #define va_int(args, flags)               \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, signed int)            \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, signed long int)       \
      :                                      \
	 va_arg(args, signed int))           \
   )

#endif



/* va_uint:
 *  Helper for reading an unsigned integer from the varargs list.
 */
#ifdef LONG_LONG

   #define va_uint(args, flags)              \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, unsigned int)          \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_LONG) ?   \
	 va_arg(args, unsigned LONG_LONG)    \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, unsigned long int)     \
      :                                      \
	 va_arg(args, unsigned int)))        \
   )

#else

   #define va_uint(args, flags)              \
   (                                         \
      ((flags) & SPRINT_FLAG_SHORT_INT) ?    \
	 va_arg(args, unsigned int)          \
      :                                      \
      (((flags) & SPRINT_FLAG_LONG_INT) ?    \
	 va_arg(args, unsigned long int)     \
      :                                      \
	 va_arg(args, unsigned int))         \
   )

#endif



/* sprint_char:
 *  Helper for formatting (!) a character.
 */
static int sprint_char(STRING_ARG *string_arg, SPRINT_INFO *info, long val)
{
   int pos = 0;

   /* 1 character max for... a character */
   string_arg->data = _AL_MALLOC((MAX(1, info->field_width) * uwidth_max(U_CURRENT)
                                                   + ucwidth(0)) * sizeof(char));

   pos += usetc(string_arg->data, val);

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return 1;
}



/* sprint_i:
 *  Worker function for formatting integers.
 */
static int sprint_i(STRING_ARG *string_arg, unsigned LONGEST val, int precision)
{
   char tmp[24];  /* for 64-bit integers */
   int i = 0, pos = string_arg->size;
   int len;

   do {
      tmp[i++] = val % 10;
      val /= 10;
   } while (val);

   for (len=i; len<precision; len++)
      pos += usetc(string_arg->data+pos, '0');

   while (i > 0)
      pos += usetc(string_arg->data+pos, tmp[--i] + '0');

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len;
}



/* sprint_plus_sign:
 *  Helper to add a plus sign or space in front of a number.
 */
#define sprint_plus_sign(len)                              \
{                                                          \
   if (info->flags & SPRINT_FLAG_FORCE_PLUS_SIGN) {        \
      pos += usetc(string_arg->data+pos, '+');             \
      len++;                                               \
   }                                                       \
   else if (info->flags & SPRINT_FLAG_FORCE_SPACE) {       \
      pos += usetc(string_arg->data+pos, ' ');             \
      len++;                                               \
   }                                                       \
}



/* sprint_int:
 *  Helper for formatting a signed integer.
 */
static int sprint_int(STRING_ARG *string_arg, SPRINT_INFO *info, LONGEST val)
{
   int pos = 0, len = 0;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_CURRENT)
                                                    + ucwidth(0)) * sizeof(char));

   if (val < 0) {
      val = -val;
      pos += usetc(string_arg->data+pos, '-');
      len++;
   }
   else
      sprint_plus_sign(len);

   info->num_special = len;

   string_arg->size = pos;

   return sprint_i(string_arg, val, info->precision) +  info->num_special;
}



/* sprint_unsigned:
 *  Helper for formatting an unsigned integer.
 */
static int sprint_unsigned(STRING_ARG *string_arg, SPRINT_INFO *info, unsigned LONGEST val)
{
   int pos = 0;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_CURRENT)
                                                    + ucwidth(0)) * sizeof(char));

   sprint_plus_sign(info->num_special);

   string_arg->size = pos;

   return sprint_i(string_arg, val, info->precision) + info->num_special;
}



/* sprint_hex:
 *  Helper for formatting a hex integer.
 */
static int sprint_hex(STRING_ARG *string_arg, SPRINT_INFO *info, int caps, unsigned LONGEST val)
{
   static char hex_digit_caps[] = "0123456789ABCDEF";
   static char hex_digit[] = "0123456789abcdef";

   char tmp[24];  /* for 64-bit integers */
   char *table;
   int pos = 0, i = 0;
   int len;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_CURRENT)
                                                    + ucwidth(0)) * sizeof(char));

   sprint_plus_sign(info->num_special);

   if (info->flags & SPRINT_FLAG_ALTERNATE_CONVERSION) {
      pos += usetc(string_arg->data+pos, '0');
      pos += usetc(string_arg->data+pos, 'x');
      info->num_special += 2;
   }

   do {
      tmp[i++] = val & 15;
      val >>= 4;
   } while (val);

   for (len=i; len<info->precision; len++)
      pos += usetc(string_arg->data+pos, '0');

   if (caps)
      table = hex_digit_caps;
   else
      table = hex_digit;

   while (i > 0)
      pos += usetc(string_arg->data+pos, table[(int)tmp[--i]]);

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len + info->num_special;
}



/* sprint_octal:
 *  Helper for formatting an octal integer.
 */
static int sprint_octal(STRING_ARG *string_arg, SPRINT_INFO *info, unsigned LONGEST val)
{
   char tmp[24];  /* for 64-bit integers */
   int pos = 0, i = 0;
   int len;

   /* 24 characters max for a 64-bit integer */
   string_arg->data = _AL_MALLOC((MAX(24, info->field_width) * uwidth_max(U_CURRENT)
                                                    + ucwidth(0)) * sizeof(char));

   sprint_plus_sign(info->num_special);

   if (info->flags & SPRINT_FLAG_ALTERNATE_CONVERSION) {
      pos += usetc(string_arg->data+pos, '0');
      info->num_special++;
   }

   do {
      tmp[i++] = val & 7;
      val >>= 3;
   } while (val);

   for (len=i; len<info->precision; len++)
      pos += usetc(string_arg->data+pos, '0');

   while (i > 0)
      pos += usetc(string_arg->data+pos, tmp[--i] + '0');

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len + info->num_special;
}



/* sprint_float:
 *  Helper for formatting a float (piggyback on the libc implementation).
 */
static int sprint_float(STRING_ARG *string_arg, SPRINT_INFO *info, double val, int conversion)
{
   char format[256], tmp[256];
   int len = 0, size;

   format[len++] = '%';

   if (info->flags & SPRINT_FLAG_LEFT_JUSTIFY)
      format[len++] = '-';

   if (info->flags & SPRINT_FLAG_FORCE_PLUS_SIGN)
      format[len++] = '+';

   if (info->flags & SPRINT_FLAG_FORCE_SPACE)
      format[len++] = ' ';

   if (info->flags & SPRINT_FLAG_ALTERNATE_CONVERSION)
      format[len++] = '#';

   if (info->flags & SPRINT_FLAG_PAD_ZERO)
      format[len++] = '0';

   if (info->field_width > 0)
      len += sprintf(format+len, "%d", info->field_width);

   if (info->precision >= 0)
      len += sprintf(format+len, ".%d", info->precision);

   format[len++] = conversion;
   format[len] = 0;

   len = sprintf(tmp, format, val);
   size = len * uwidth_max(U_CURRENT) + ucwidth(0);

   string_arg->data = _AL_MALLOC(size * sizeof(char));

   do_uconvert(tmp, U_ASCII, string_arg->data, U_CURRENT, size);

   info->field_width = 0;

   string_arg->size = ustrsize(string_arg->data);

   return len;
}



/* sprint_string:
 *  Helper for formatting a string.
 */
static int sprint_string(STRING_ARG *string_arg, SPRINT_INFO *info, AL_CONST char *s)
{
   int pos = 0, len = 0;
   int c;

   string_arg->data = _AL_MALLOC((MAX(ustrlen(s), info->field_width) * uwidth_max(U_CURRENT)
                                                            + ucwidth(0)) * sizeof(char));

   while ((c = ugetxc(&s)) != 0) {
      if ((info->precision >= 0) && (len >= info->precision))
	 break;

      pos += usetc(string_arg->data+pos, c);
      len++;
   }

   string_arg->size = pos;
   usetc(string_arg->data+pos, 0);

   return len;
}



/* decode_format_string:
 *  Worker function for decoding the format string (with those pretty '%' characters)
 */
static int decode_format_string(char *buf, STRING_ARG *string_arg, AL_CONST char *format, va_list args)
{
   SPRINT_INFO info;
   int *pstr_pos;
   int done, slen, c, i, pos;
   int shift, shiftbytes, shiftfiller;
   int len = 0;

   while ((c = ugetxc(&format)) != 0) {

      if (c == '%') {
	 if ((c = ugetc(format)) == '%') {
	    /* percent sign escape */
	    format += uwidth(format);
            buf += usetc(buf, '%');
            buf += usetc(buf, '%');
            len++;
	 }
	 else {
	    /* format specifier */
	    #define NEXT_C()                 \
	    {                                \
	       format += uwidth(format);     \
	       c = ugetc(format);            \
	    }

	    /* set default conversion flags */
	    info.flags = 0;
	    info.field_width = 0;
	    info.precision = -1;
	    info.num_special = 0;

	    /* check for conversion flags */
	    done = FALSE;

	    do {
	       switch (c) {

		  case '-':
		     info.flags |= SPRINT_FLAG_LEFT_JUSTIFY;
		     NEXT_C();
		     break;

		  case '+':
		     info.flags |= SPRINT_FLAG_FORCE_PLUS_SIGN;
		     NEXT_C();
		     break;

		  case ' ':
		     info.flags |= SPRINT_FLAG_FORCE_SPACE;
		     NEXT_C();
		     break;

		  case '#':
		     info.flags |= SPRINT_FLAG_ALTERNATE_CONVERSION;
		     NEXT_C();
		     break;

		  case '0':
		     info.flags |= SPRINT_FLAG_PAD_ZERO;
		     NEXT_C();
		     break;

		  default:
		     done = TRUE;
		     break;
	       }

	    } while (!done);

	    /* check for a field width specifier */
	    if (c == '*') {
	       NEXT_C();
	       info.field_width = va_arg(args, int);
	       if (info.field_width < 0) {
		  info.flags |= SPRINT_FLAG_LEFT_JUSTIFY;
		  info.field_width = -info.field_width;
	       }
	    }
	    else if ((c >= '0') && (c <= '9')) {
	       info.field_width = 0;
	       do {
		  info.field_width *= 10;
		  info.field_width += c - '0';
		  NEXT_C();
	       } while ((c >= '0') && (c <= '9'));
	    }

	    /* check for a precision specifier */
	    if (c == '.')
	       NEXT_C();

	    if (c == '*') {
	       NEXT_C();
	       info.precision = va_arg(args, int);
	       if (info.precision < 0)
		  info.precision = 0;
	    }
	    else if ((c >= '0') && (c <= '9')) {
	       info.precision = 0;
	       do {
		  info.precision *= 10;
		  info.precision += c - '0';
		  NEXT_C();
	       } while ((c >= '0') && (c <= '9'));
	    }

	    /* check for size qualifiers */
	    done = FALSE;

	    do {
	       switch (c) {

		  case 'h':
		     info.flags |= SPRINT_FLAG_SHORT_INT;
		     NEXT_C();
		     break;

		  case 'l':
		     if (info.flags & SPRINT_FLAG_LONG_INT)
			info.flags |= SPRINT_FLAG_LONG_LONG;
		     else
			info.flags |= SPRINT_FLAG_LONG_INT;
		     NEXT_C();
		     break;

		  case 'L':
		     info.flags |= (SPRINT_FLAG_LONG_DOUBLE | SPRINT_FLAG_LONG_LONG);
		     NEXT_C();
		     break;

		  default:
		     done = TRUE;
		     break;
	       }

	    } while (!done);

	    /* format the data */
	    switch (c) {

	       case 'c':
		  /* character */
		  slen = sprint_char(string_arg, &info, va_arg(args, int));
		  NEXT_C();
		  break;

	       case 'd':
	       case 'i':
		  /* signed integer */
		  slen = sprint_int(string_arg, &info, va_int(args, info.flags));
		  NEXT_C();
		  break;

	       case 'D':
		  /* signed long integer */
		  slen = sprint_int(string_arg, &info, va_int(args, info.flags | SPRINT_FLAG_LONG_INT));
		  NEXT_C();
		  break;

	       case 'e':
	       case 'E':
	       case 'f':
	       case 'g':
	       case 'G':
		  /* double */
		  if (info.flags & SPRINT_FLAG_LONG_DOUBLE)
		     slen = sprint_float(string_arg, &info, va_arg(args, long double), c);
		  else
		     slen = sprint_float(string_arg, &info, va_arg(args, double), c);
		  NEXT_C();
		  break;

	       case 'n':
		  /* store current string position */
		  pstr_pos = va_arg(args, int *);
		  *pstr_pos = len;
		  slen = -1;
		  NEXT_C();
		  break;

	       case 'o':
		  /* unsigned octal integer */
		  slen = sprint_octal(string_arg, &info, va_uint(args, info.flags));
		  NEXT_C();
		  break;

	       case 'p':
		  /* pointer */
		  slen = sprint_hex(string_arg, &info, FALSE, (unsigned long)(va_arg(args, void *)));
		  NEXT_C();
		  break;

	       case 's':
		  /* string */
		  slen = sprint_string(string_arg, &info, va_arg(args, char *));
		  NEXT_C();
		  break;

	       case 'u':
		  /* unsigned integer */
		  slen = sprint_unsigned(string_arg, &info, va_uint(args, info.flags));
		  NEXT_C();
		  break;

	       case 'U':
		  /* unsigned long integer */
		  slen = sprint_unsigned(string_arg, &info, va_uint(args, info.flags | SPRINT_FLAG_LONG_INT));
		  NEXT_C();
		  break;

	       case 'x':
	       case 'X':
		  /* unsigned hex integer */
		  slen = sprint_hex(string_arg, &info, (c == 'X'), va_uint(args, info.flags));
		  NEXT_C();
		  break;

	       default:
		  /* weird shit... */
		  slen = -1;
		  break;
	    }

            if (slen >= 0) {
               if (slen < info.field_width) {
                  if (info.flags & SPRINT_FLAG_LEFT_JUSTIFY) {
                     /* left align the result */
                     pos = string_arg->size;
                     while (slen < info.field_width) {
                        pos += usetc(string_arg->data+pos, ' ');
                        slen++;
                     }

                     string_arg->size = pos;
                     usetc(string_arg->data+pos, 0);
                  }
                  else {
                     /* right align the result */
                     shift = info.field_width - slen;

                     if (shift > 0) {
                        pos = 0;

                        if (info.flags & SPRINT_FLAG_PAD_ZERO) {
                           shiftfiller = '0';

                           for (i=0; i<info.num_special; i++)
                              pos += uwidth(string_arg->data+pos);
                        }
                        else
                           shiftfiller = ' ';

                        shiftbytes = shift * ucwidth(shiftfiller);
                        memmove(string_arg->data+pos+shiftbytes, string_arg->data+pos, string_arg->size-pos+ucwidth(0));

                        string_arg->size += shiftbytes;
                        slen += shift;

                        for (i=0; i<shift; i++)
                           pos += usetc(string_arg->data+pos, shiftfiller);
                     }
                  }
               }

               buf += usetc(buf, '%');
               buf += usetc(buf, 's');
               len += slen;

               /* allocate next item */
               string_arg->next = _AL_MALLOC(sizeof(STRING_ARG));
               string_arg = string_arg->next;
               string_arg->next = NULL;
            }
	 }
      }
      else {
	 /* normal character */
	 buf += usetc(buf, c);
	 len++;
      }
   }

   usetc(buf, 0);

   return len;
}



/* uvszprintf:
 *  Enhanced Unicode-aware version of the ANSI vsprintf() function
 *  than can handle the size (in bytes) of the destination buffer.
 *  The raw Unicode-aware version of ANSI vsprintf() is defined as:
 *   #define uvsprintf(buf, format, args) uvszprintf(buf, INT_MAX, format, args)
 */
int uvszprintf(char *buf, int size, AL_CONST char *format, va_list args)
{
   char *decoded_format, *df;
   STRING_ARG *string_args, *iter_arg;
   int c, len;
   ASSERT(buf);
   ASSERT(size >= 0);
   ASSERT(format);

   /* decoding can only lower the length of the format string */
   df = decoded_format = _AL_MALLOC_ATOMIC(ustrsizez(format) * sizeof(char));

   /* allocate first item */
   string_args = _AL_MALLOC(sizeof(STRING_ARG));
   string_args->next = NULL;

   /* 1st pass: decode */
   len = decode_format_string(decoded_format, string_args, format, args);

   size -= ucwidth(0);
   iter_arg = string_args;

   /* 2nd pass: concatenate */
   while ((c = ugetx(&decoded_format)) != 0) {

      if (c == '%') {
	 if ((c = ugetx(&decoded_format)) == '%') {
	    /* percent sign escape */
            size -= ucwidth('%');
            if (size<0)
               break;
	    buf += usetc(buf, '%');
	 }
	 else if (c == 's') {
            /* string argument */
            ustrzcpy(buf, size+ucwidth(0), iter_arg->data);
            buf += iter_arg->size;
            size -= iter_arg->size;
            if (size<0) {
               buf += size;
               break;
            }
            iter_arg = iter_arg->next;
         }
      }
      else {
	 /* normal character */
         size -= ucwidth(c);
         if (size<0)
            break;
	 buf += usetc(buf, c);
      }
   }

   usetc(buf, 0);

   /* free allocated resources */
   while (string_args->next) {
      _AL_FREE(string_args->data);
      iter_arg = string_args;
      string_args = string_args->next;
      _AL_FREE(iter_arg);
   }
   _AL_FREE(string_args);
   _AL_FREE(df);  /* alias for decoded_format */

   return len;
}



/* usprintf:
 *  Unicode-aware version of the ANSI sprintf() function.
 */
int usprintf(char *buf, AL_CONST char *format, ...)
{
   int ret;
   va_list ap;
   ASSERT(buf);
   ASSERT(format);

   va_start(ap, format);
   ret = uvszprintf(buf, INT_MAX, format, ap);
   va_end(ap);

   return ret;
}



/* uszprintf:
 *  Enhanced Unicode-aware version of the ANSI sprintf() function
 *  that can handle the size (in bytes) of the destination buffer.
 */
int uszprintf(char *buf, int size, AL_CONST char *format, ...)
{
   int ret;
   va_list ap;
   ASSERT(buf);
   ASSERT(size >= 0);
   ASSERT(format);

   va_start(ap, format);
   ret = uvszprintf(buf, size, format, ap);
   va_end(ap);

   return ret;
}

