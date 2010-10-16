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
 *      Helper routines to make file.c work on PSP platform.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_PSP
#error Something is wrong with the makefile
#endif


int _al_file_isok(AL_CONST char *filename)
{
   return TRUE;
}



uint64_t _al_file_size_ex(AL_CONST char *filename)
{
    struct stat s;
   char tmp[1024];

   if (stat(uconvert(filename, U_CURRENT, tmp, U_UTF8, sizeof(tmp)), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_size;
}



time_t _al_file_time(AL_CONST char *filename)
{
    struct stat s;
   char tmp[1024];

   if (stat(uconvert(filename, U_CURRENT, tmp, U_UTF8, sizeof(tmp)), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_mtime;
}



void _al_getdcwd(int drive, char *buf, int size)
{
}



int al_findfirst(AL_CONST char *pattern, struct al_ffblk *info, int attrib)
{
   return 0;
}



int al_findnext(struct al_ffblk *info)
{
   return 0;
}



void al_findclose(struct al_ffblk *info)
{
}



void _al_detect_filename_encoding(void)
{
}
