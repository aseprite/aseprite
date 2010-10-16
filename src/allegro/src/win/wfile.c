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
 *      Helper routines to make file.c work on Windows platforms.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <time.h>
   #include <sys/stat.h>
#endif

#include "allegro.h"
#include "winalleg.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif



/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive. This really only applies to the DOS library, so we don't bother
 *  with it.
 */
int _al_file_isok(AL_CONST char *filename)
{
   return TRUE;
}



/* _al_detect_filename_encoding:
 *  Platform specific function to detect the filename encoding. This is called
 *  after setting a system driver, and even if this driver is SYSTEM_NONE.
 */
void _al_detect_filename_encoding(void)
{
#ifdef ALLEGRO_DMC
   /* DMC's C library does not support _wfinddata_t */
   set_filename_encoding(U_ASCII);
#else
   /* Windows NT 4.0, 2000, XP, etc support unicode filenames */
   set_filename_encoding(GetVersion() & 0x80000000 ? U_ASCII : U_UNICODE);
#endif
}



/* _al_file_size_ex:
 *  Measures the size of the specified file.
 */
uint64_t _al_file_size_ex(AL_CONST char *filename)
{
   struct _stat s;
   char tmp[1024];

   if (get_filename_encoding() != U_UNICODE) {
      if (_stat(uconvert(filename, U_CURRENT, tmp, U_ASCII, sizeof(tmp)), &s) != 0) {
         *allegro_errno = errno;
         return 0;
      }
   }
   else {
      if (_wstat((wchar_t*)uconvert(filename, U_CURRENT, tmp, U_UNICODE,
                 sizeof(tmp)), &s) != 0) {
         *allegro_errno = errno;
         return 0;
      }
   }

   return s.st_size;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
time_t _al_file_time(AL_CONST char *filename)
{
   struct _stat s;
   char tmp[1024];

   if (get_filename_encoding() != U_UNICODE) {
      if (_stat(uconvert(filename, U_CURRENT, tmp, U_ASCII, sizeof(tmp)), &s) != 0) {
         *allegro_errno = errno;
         return 0;
      }
   }
   else {
      if (_wstat((wchar_t*)uconvert(filename, U_CURRENT, tmp, U_UNICODE, sizeof(tmp)), &s) != 0) {
         *allegro_errno = errno;
         return 0;
      }
   }

   return s.st_mtime;
}



/* structure for use by the directory scanning routines */
struct FF_DATA
{
   union {
      struct _finddata_t a;
      struct _wfinddata_t w;
   } data;
   long handle;
   int attrib;
};



/* fill_ffblk:
 *  Helper function to fill in an al_ffblk structure.
 */
static void fill_ffblk(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   if (get_filename_encoding() != U_UNICODE) {
      info->attrib = ff_data->data.a.attrib;
      info->time = ff_data->data.a.time_write;
      info->size = ff_data->data.a.size;

      do_uconvert(ff_data->data.a.name, U_ASCII, info->name, U_CURRENT,
                  sizeof(info->name));
   }
   else {
      info->attrib = ff_data->data.w.attrib;
      info->time = ff_data->data.w.time_write;
      info->size = ff_data->data.w.size;

      do_uconvert((const char*)ff_data->data.w.name, U_UNICODE, info->name,
                  U_CURRENT, sizeof(info->name));
   }
}



/* al_findfirst:
 *  Initiates a directory search.
 */
int al_findfirst(AL_CONST char *pattern, struct al_ffblk *info, int attrib)
{
   struct FF_DATA *ff_data;
   char tmp[1024];

   /* allocate ff_data structure */
   ff_data = _AL_MALLOC(sizeof(struct FF_DATA));

   if (!ff_data) {
      *allegro_errno = ENOMEM;
      return -1;
   }

   /* attach it to the info structure */
   info->ff_data = (void *) ff_data;

   /* Windows defines specific flags for NTFS permissions:
    *   FA_TEMPORARY            0x0100
    *   FA_SPARSE_FILE          0x0200 
    *   FA_REPARSE_POINT        0x0400
    *   FA_COMPRESSED           0x0800
    *   FA_OFFLINE              0x1000
    *   FA_NOT_CONTENT_INDEXED  0x2000
    *   FA_ENCRYPTED            0x4000
    * so we must set them in the mask by default; moreover,
    * in order to avoid problems with flags added in the
    * future, we simply set all bits past the first byte.
    */
   ff_data->attrib = attrib | 0xFFFFFF00;

   /* start the search */
   errno = *allegro_errno = 0;

   if (get_filename_encoding() != U_UNICODE) {
      ff_data->handle = _findfirst(uconvert(pattern, U_CURRENT, tmp,
                                            U_ASCII, sizeof(tmp)),
                                            &ff_data->data.a);

      if (ff_data->handle < 0) {
         *allegro_errno = errno;
         _AL_FREE(ff_data);
         info->ff_data = NULL;
         return -1;
      }

      if (ff_data->data.a.attrib & ~ff_data->attrib) {
         if (al_findnext(info) != 0) {
            al_findclose(info);
            return -1;
         }
         else
            return 0;
      }
   }
   else {
      ff_data->handle = _wfindfirst((wchar_t*)uconvert(pattern, U_CURRENT, tmp,
                                                       U_UNICODE, sizeof(tmp)),
                                                       &ff_data->data.w);

      if (ff_data->handle < 0) {
         *allegro_errno = errno;
         _AL_FREE(ff_data);
         info->ff_data = NULL;
         return -1;
      }

      if (ff_data->data.w.attrib & ~ff_data->attrib) {
         if (al_findnext(info) != 0) {
            al_findclose(info);
            return -1;
         }
         else
            return 0;
      }
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

   if (get_filename_encoding() != U_UNICODE) {
      do {
         if (_findnext(ff_data->handle, &ff_data->data.a) != 0) {
            *allegro_errno = errno;
            return -1;
         }
      } while (ff_data->data.a.attrib & ~ff_data->attrib);
   }
   else {
      do {
         if (_wfindnext(ff_data->handle, &ff_data->data.w) != 0) {
            *allegro_errno = errno;
            return -1;
         }
      } while (ff_data->data.w.attrib & ~ff_data->attrib);
   }

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
      _findclose(ff_data->handle);
      _AL_FREE(ff_data);
      info->ff_data = NULL;
   }
}



/* _al_drive_exists:
 *  Checks whether the specified drive is valid.
 */
int _al_drive_exists(int drive)
{
   return GetLogicalDrives() & (1 << drive);
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive(void)
{
   return _getdrive() - 1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   char tmp[1024];

   if (get_filename_encoding() != U_UNICODE) {
      if (_getdcwd(drive+1, tmp, sizeof(tmp)))
         do_uconvert(tmp, U_ASCII, buf, U_CURRENT, size);
      else
         usetc(buf, 0);
   }
   else {
      if (_wgetdcwd(drive+1, (wchar_t*)tmp, sizeof(tmp)/sizeof(wchar_t)))
         do_uconvert(tmp, U_UNICODE, buf, U_CURRENT, size);
      else
         usetc(buf, 0);
   }
}



/* _al_ffblk_get_size:
 *  Returns the size out of an _al_ffblk structure.
 */
uint64_t al_ffblk_get_size(struct al_ffblk *info)
{
   struct FF_DATA *ff_data;

   ASSERT(info);
   ff_data = (struct FF_DATA *) info->ff_data;

   if (get_filename_encoding() != U_UNICODE) {
      return ff_data->data.a.size;
   }
   else {
      return ff_data->data.w.size;
   }
}



/* _al_win_open:
 *  Open a file with open() or _wopen() depending on whether Unicode filenames
 *  are supported by this version of Windows and compiler.
 */
int _al_win_open(const char *filename, int mode, int perm)
{
   if (get_filename_encoding() != U_UNICODE) {
      return open(filename, mode, perm);
   }
   else {
      return _wopen((wchar_t*)filename, mode, perm);
   }
}



/* _al_win_unlink:
 *  Remove a file with unlink() or _wunlink() depending on whether Unicode
 *  filenames are supported by this version of Windows and compiler.
 */
int _al_win_unlink(const char *pathname)
{
   if (get_filename_encoding() != U_UNICODE) {
      return unlink(pathname);
   }
   else {
      return _wunlink((wchar_t*)pathname);
   }
}

