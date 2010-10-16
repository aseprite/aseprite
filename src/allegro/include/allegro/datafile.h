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
 *      Datafile access routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DATAFILE_H
#define ALLEGRO_DATAFILE_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct BITMAP;
struct PACKFILE;
struct RGB;

#define DAT_ID(a,b,c,d)    AL_ID(a,b,c,d)

#define DAT_MAGIC          DAT_ID('A','L','L','.')
#define DAT_FILE           DAT_ID('F','I','L','E')
#define DAT_DATA           DAT_ID('D','A','T','A')
#define DAT_FONT           DAT_ID('F','O','N','T')
#define DAT_SAMPLE         DAT_ID('S','A','M','P')
#define DAT_MIDI           DAT_ID('M','I','D','I')
#define DAT_PATCH          DAT_ID('P','A','T',' ')
#define DAT_FLI            DAT_ID('F','L','I','C')
#define DAT_BITMAP         DAT_ID('B','M','P',' ')
#define DAT_RLE_SPRITE     DAT_ID('R','L','E',' ')
#define DAT_C_SPRITE       DAT_ID('C','M','P',' ')
#define DAT_XC_SPRITE      DAT_ID('X','C','M','P')
#define DAT_PALETTE        DAT_ID('P','A','L',' ')
#define DAT_PROPERTY       DAT_ID('p','r','o','p')
#define DAT_NAME           DAT_ID('N','A','M','E')
#define DAT_END            -1


typedef struct DATAFILE_PROPERTY
{
   char *dat;                          /* pointer to the data */
   int type;                           /* property type */
} DATAFILE_PROPERTY;


typedef struct DATAFILE
{
   void *dat;                          /* pointer to the data */
   int type;                           /* object type */
   long size;                          /* size of the object */
   DATAFILE_PROPERTY *prop;            /* object properties */
} DATAFILE;


typedef struct DATAFILE_INDEX
{
   char *filename;                     /* datafile name (path) */
   long *offset;                       /* list of offsets */
} DATAFILE_INDEX;


AL_FUNC(DATAFILE *, load_datafile, (AL_CONST char *filename));
AL_FUNC(DATAFILE *, load_datafile_callback, (AL_CONST char *filename, AL_METHOD(void, callback, (DATAFILE *))));
AL_FUNC(DATAFILE_INDEX *, create_datafile_index, (AL_CONST char *filename));
AL_FUNC(void, unload_datafile, (DATAFILE *dat));
AL_FUNC(void, destroy_datafile_index, (DATAFILE_INDEX *index));

AL_FUNC(DATAFILE *, load_datafile_object, (AL_CONST char *filename, AL_CONST char *objectname));
AL_FUNC(DATAFILE *, load_datafile_object_indexed, (AL_CONST DATAFILE_INDEX *index, int item));
AL_FUNC(void, unload_datafile_object, (DATAFILE *dat));

AL_FUNC(DATAFILE *, find_datafile_object, (AL_CONST DATAFILE *dat, AL_CONST char *objectname));
AL_FUNC(AL_CONST char *, get_datafile_property, (AL_CONST DATAFILE *dat, int type));
AL_FUNC(void, register_datafile_object, (int id_, AL_METHOD(void *, load, (struct PACKFILE *f, long size)), AL_METHOD(void, destroy, (void *data))));

AL_FUNC(void, fixup_datafile, (DATAFILE *data));

AL_FUNC(struct BITMAP *, load_bitmap, (AL_CONST char *filename, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_bmp, (AL_CONST char *filename, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_bmp_pf, (PACKFILE *f, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_lbm, (AL_CONST char *filename, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_pcx, (AL_CONST char *filename, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_pcx_pf, (PACKFILE *f, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_tga, (AL_CONST char *filename, struct RGB *pal));
AL_FUNC(struct BITMAP *, load_tga_pf, (PACKFILE *f, struct RGB *pal));

AL_FUNC(int, save_bitmap, (AL_CONST char *filename, struct BITMAP *bmp, AL_CONST struct RGB *pal));
AL_FUNC(int, save_bmp, (AL_CONST char *filename, struct BITMAP *bmp, AL_CONST struct RGB *pal));
AL_FUNC(int, save_bmp_pf, (PACKFILE *f, struct BITMAP *bmp, AL_CONST struct RGB *pal));
AL_FUNC(int, save_pcx, (AL_CONST char *filename, struct BITMAP *bmp, AL_CONST struct RGB *pal));
AL_FUNC(int, save_pcx_pf, (PACKFILE *f, struct BITMAP *bmp, AL_CONST struct RGB *pal));
AL_FUNC(int, save_tga, (AL_CONST char *filename, struct BITMAP *bmp, AL_CONST struct RGB *pal));
AL_FUNC(int, save_tga_pf, (PACKFILE *f, struct BITMAP *bmp, AL_CONST struct RGB *pal));

AL_FUNC(void, register_bitmap_file_type, (AL_CONST char *ext, AL_METHOD(struct BITMAP *, load, (AL_CONST char *filename, struct RGB *pal)), AL_METHOD(int, save, (AL_CONST char *filename, struct BITMAP *bmp, AL_CONST struct RGB *pal))));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_DATAFILE_H */
