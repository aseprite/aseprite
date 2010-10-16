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
 *      Helper routines to make file.c work on DOS platforms.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <string.h>
   #include <time.h>
   #include <dos.h>
   #include <sys/stat.h>
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive.
 */
int _al_file_isok(AL_CONST char *filename)
{
   char ch = utolower(ugetc(filename));
   __dpmi_regs r;

   if (((ch == 'a') || (ch == 'b')) && (ugetat(filename, 1) == ':')) {
      r.x.ax = 0x440E;
      r.x.bx = 1;
      __dpmi_int(0x21, &r);

      if ((r.h.al != 0) && (r.h.al != (ch - 'a' + 1))) {
	 *allegro_errno = EACCES;
	 return FALSE;
      }
   }

   return TRUE;
}



/* _al_file_size_ex:
 *  Measures the size of the specified file.
 */
uint64_t _al_file_size_ex(AL_CONST char *filename)
{
   struct stat s;
   char tmp[1024];

   if (stat(uconvert_toascii(filename, tmp), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_size;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
time_t _al_file_time(AL_CONST char *filename)
{
   struct stat s;
   char tmp[1024];

   if (stat(uconvert_toascii(filename, tmp), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_mtime;
}


/* _al_detect_filename_encoding:
 *
 *  Platform specific function to detect the filename encoding. This is called
 *  after setting a system driver, and even if this driver is SYSTEM_NONE.
 */
void _al_detect_filename_encoding(void)
{
   set_filename_encoding(U_ASCII); 
}



/* structure for use by the directory scanning routines */
struct FF_DATA
{
   struct ffblk data;
   int attrib;
};



/* fill_ffblk:
 *  Helper function to fill in an al_ffblk structure.
 */
static void fill_ffblk(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;
   struct tm t;

   info->attrib = ff_data->data.ff_attrib;

   memset(&t, 0, sizeof(struct tm));
   t.tm_sec  = (ff_data->data.ff_ftime & 0x1F) * 2;
   t.tm_min  = (ff_data->data.ff_ftime >> 5) & 0x3F;
   t.tm_hour = (ff_data->data.ff_ftime >> 11) & 0x1F;
   t.tm_mday = (ff_data->data.ff_fdate & 0x1F);
   t.tm_mon  = ((ff_data->data.ff_fdate >> 5) & 0x0F) - 1;
   t.tm_year = (ff_data->data.ff_fdate >> 9) + 80;
   t.tm_isdst = -1;

   info->time = mktime(&t);

   info->size = ff_data->data.ff_fsize;

   do_uconvert(ff_data->data.ff_name, U_ASCII, info->name, U_CURRENT, sizeof(info->name));
}



/* al_findfirst:
 *  Initiates a directory search.
 */
int al_findfirst(AL_CONST char *pattern, struct al_ffblk *info, int attrib)
{
   struct FF_DATA *ff_data;
   char tmp[1024];
   int ret;

   /* allocate ff_data structure */
   ff_data = _AL_MALLOC(sizeof(struct FF_DATA));

   if (!ff_data) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   /* attach it to the info structure */
   info->ff_data = (void *) ff_data;

   /* Win2k appears to set bit7 for files which otherwise
    * would not have any attribute flag set.
    */
   ff_data->attrib = attrib | 0x80;

   /* start the search */
   errno = *allegro_errno = 0;

   /* We pass all the flags to findfirst() in order to work around the DOS limitations
    * for FA_RDONLY and FA_ARCH which are normally ignored.
    *
    * The return value of findfirst() and findnext() is not meaningful because the
    * functions return an errno code under DJGPP (ENOENT or ENMFILE) whereas they
    * return a DOS error code under Watcom (02h, 03h or 12h).
    * However the functions of both compilers set errno accordingly.
    */
   ret = findfirst(uconvert_toascii(pattern, tmp), &ff_data->data, 0xFF);

   if (ret != 0) {
#ifdef ALLEGRO_DJGPP
      /* the DJGPP libc may set errno to ENMFILE, we convert it to ENOENT */
      *allegro_errno = (errno == ENMFILE ? ENOENT : errno);
#else
      *allegro_errno = errno;
#endif
      _AL_FREE(ff_data);
      info->ff_data = NULL;
      return -1;
   }

   if (ff_data->data.ff_attrib & ~ff_data->attrib) {
      if (al_findnext(info) != 0) {
         al_findclose(info);
         return -1;
      }
      else
         return 0;
   }

   fill_ffblk(info);
   return 0;
}



/* al_findnext:
 *  Retrieves the next file from a directory search.
 */
int al_findnext(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   do {
      if (findnext(&ff_data->data) != 0) {
#ifdef ALLEGRO_DJGPP
         /* the DJGPP libc may set errno to ENMFILE, we convert it to ENOENT */
         *allegro_errno = (errno == ENMFILE ? ENOENT : errno);
#else
         *allegro_errno = errno;
#endif
         return -1;
      }

   } while (ff_data->data.ff_attrib & ~ff_data->attrib);

   fill_ffblk(info);
   return 0;
}



/* al_findclose:
 *  Cleans up after a directory search.
 */
void al_findclose(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   if (ff_data) {
      _AL_FREE(ff_data);
      info->ff_data = NULL;
   }
}



/* _al_drive_exists:
 *  Checks whether the specified drive is valid.
 */
int _al_drive_exists(int drive)
{
   unsigned int old_drive;
   int ret = FALSE;
   __dpmi_regs r;

   /* get actual drive */
   r.h.ah = 0x19;
   __dpmi_int(0x21, &r);
   old_drive = r.h.al;

   /* see if the drive is assigned as a valid drive */
   r.h.ah = 0x0E;
   r.h.dl = drive;
   __dpmi_int(0x21, &r);

   r.h.ah = 0x19;
   __dpmi_int(0x21, &r);

   if (r.h.al == drive) {
      /* ok, now check if it is a logical drive */
      r.x.ax = 0x440E;
      r.h.bl = drive+1;
      __dpmi_int(0x21, &r);

      if ((r.x.flags & 1) ||        /* call failed */
          (r.h.al == 0) ||          /* has no logical drives */
          (r.h.al == (drive+1)))    /* not a logical drive */
         ret = TRUE;
   }

   /* now we set the old drive */
   r.h.ah = 0x0E;
   r.h.dl = old_drive;
   __dpmi_int(0x21, &r);

   return ret;
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive(void)
{
   unsigned int d;

   _dos_getdrive(&d);

   return d-1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   unsigned int old_drive, tmp_drive;
   char filename[32], tmp[1024];
   int pos; 

   pos = usetc(filename, drive+'a');
   pos += usetc(filename+pos, ':');
   pos += usetc(filename+pos, '\\');
   usetc(filename+pos, 0);

   if (!_al_file_isok(filename)) {
      *buf = 0;
      return;
   }

   _dos_getdrive(&old_drive);
   _dos_setdrive(drive+1, &tmp_drive);
   _dos_getdrive(&tmp_drive);

   if (tmp_drive == (unsigned int)drive+1) {
      if (getcwd(tmp, sizeof(tmp)))
	 do_uconvert(tmp, U_ASCII, buf, U_CURRENT, size);
      else
	 usetc(buf, 0);
   }
   else
      usetc(buf, 0);

   _dos_setdrive(old_drive, &tmp_drive); 
}



/* _al_ffblk_get_size:
 *  Returns the size out of an _al_ffblk structure.
 */
uint64_t al_ffblk_get_size(struct al_ffblk *info)
{
   struct FF_DATA *ff_data;
   ASSERT(info);
   ff_data = (struct FF_DATA *) info->ff_data;

   return ff_data->data.ff_fsize;
}
