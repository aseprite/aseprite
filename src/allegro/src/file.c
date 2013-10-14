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
 *      File I/O.
 *
 *      By Shawn Hargreaves.
 *
 *      _pack_fdopen() and related modifications by Annie Testes.
 *
 *      Evert Glebbeek added the support for relative filenames:
 *      make_absolute_filename(), make_relative_filename() and
 *      is_relative_filename().
 *
 *      Peter Wang added support for packfile vtables.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_MPW
   #include <sys/stat.h>
#endif

#ifdef ALLEGRO_UNIX
   #include <pwd.h>                 /* for tilde expansion */
#endif

#ifdef ALLEGRO_WINDOWS
   #include "winalleg.h" /* for GetTempPath */
#endif


/* permissions to use when opening files */
#ifndef ALLEGRO_MPW

/* some OSes have no concept of "group" and "other" */
#ifndef S_IRGRP
   #define S_IRGRP   0
   #define S_IWGRP   0
#endif
#ifndef S_IROTH
   #define S_IROTH   0
   #define S_IWOTH   0
#endif

#define OPEN_PERMS   (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#endif /* !ALLEGRO_MPW */



static char the_password[256] = EMPTY_STRING;

int _packfile_filesize = 0;
int _packfile_datasize = 0;

int _packfile_type = 0;

static PACKFILE_VTABLE normal_vtable;

static PACKFILE *pack_fopen_special_file(AL_CONST char *filename, AL_CONST char *mode);

static int filename_encoding = U_ASCII;


#define FA_DAT_FLAGS  (FA_RDONLY | FA_ARCH)


typedef struct RESOURCE_PATH
{
   int priority;
   char path[1024];
   struct RESOURCE_PATH *next;
} RESOURCE_PATH;

static RESOURCE_PATH *resource_path_list = NULL;


static void destroy_resource_path_list(void);



/***************************************************
 ****************** Path handling ******************
 ***************************************************/


/* fix_filename_case:
 *  Converts filename to upper case.
 */
char *fix_filename_case(char *filename)
{
   ASSERT(filename);

   if (!ALLEGRO_LFN)
      ustrupr(filename);

   return filename;
}



/* fix_filename_slashes:
 *  Converts '/' or '\' to system specific path separator.
 */
char *fix_filename_slashes(char *filename)
{
   int pos, c;
   ASSERT(filename);

   for (pos=0; ugetc(filename+pos); pos+=uwidth(filename+pos)) {
      c = ugetc(filename+pos);
      if ((c == '/') || (c == '\\'))
         usetat(filename+pos, 0, OTHER_PATH_SEPARATOR);
   }

   return filename;
}



/* Canonicalize_filename:
 *  Returns the canonical form of the specified filename, i.e. the
 *  minimal absolute filename describing the same file.
 */
char *canonicalize_filename(char *dest, AL_CONST char *filename, int size)
{
   int saved_errno = errno;
   char buf[1024], buf2[1024];
   char *p;
   int pos = 0;
   int drive = -1;
   int c1, i;
   ASSERT(dest);
   ASSERT(filename);
   ASSERT(size >= 0);

   #if (DEVICE_SEPARATOR != 0) && (DEVICE_SEPARATOR != '\0')

      /* check whether we have a drive letter */
      c1 = utolower(ugetc(filename));
      if ((c1 >= 'a') && (c1 <= 'z')) {
         int c2 = ugetat(filename, 1);
         if (c2 == DEVICE_SEPARATOR) {
            drive = c1 - 'a';
            filename += uwidth(filename);
            filename += uwidth(filename);
         }
      }

      /* if not, use the current drive */
      if (drive < 0)
         drive = _al_getdrive();

      pos += usetc(buf+pos, drive+'a');
      pos += usetc(buf+pos, DEVICE_SEPARATOR);

   #endif

   #ifdef ALLEGRO_UNIX

      /* if the filename starts with ~ then it's relative to a home directory */
      if ((ugetc(filename) == '~')) {
         AL_CONST char *tail = filename + uwidth(filename); /* could be the username */
         char *home = NULL;                /* their home directory */

         if (ugetc(tail) == '/' || !ugetc(tail)) {
            /* easy */
            home = getenv("HOME");
            if (home)
               home = _al_strdup(home);
         }
         else {
            /* harder */
            char *username = (char *)tail, *ascii_username, *ch;
            int userlen;
            struct passwd *pwd;

            /* find the end of the username */
            tail = ustrchr(username, '/');
            if (!tail)
               tail = ustrchr(username, '\0');

            /* this ought to be the ASCII length, but I can't see a Unicode
             * function to return the difference in characters between two
             * pointers. This code is safe on the assumption that ASCII is
             * the most efficient encoding, but wasteful of memory */
            userlen = tail - username + ucwidth('\0');
            ascii_username = _AL_MALLOC_ATOMIC(userlen);

            if (ascii_username) {
               /* convert the username to ASCII, find the password entry,
                * and copy their home directory. */
               do_uconvert(username, U_CURRENT, ascii_username, U_ASCII, userlen);

               if ((ch = strchr(ascii_username, '/')))
                  *ch = '\0';

               setpwent();

               while (((pwd = getpwent()) != NULL) &&
                      (strcmp(pwd->pw_name, ascii_username) != 0))
                  ;

               _AL_FREE(ascii_username);

               if (pwd)
                  home = _al_strdup(pwd->pw_dir);

               endpwent();
            }
         }

         /* If we got a home directory, prepend it to the filename. Otherwise
          * we leave the filename alone, like bash but not tcsh; bash is better
          * anyway. :)
          */
         if (home) {
            do_uconvert(home, U_ASCII, buf+pos, U_CURRENT, sizeof(buf)-pos);
            _AL_FREE(home);
            pos = ustrsize(buf);
            filename = tail;
            goto no_relativisation;
         }
      }

   #endif   /* Unix */

   /* if the filename is relative, make it absolute */
   if ((ugetc(filename) != '/') && (ugetc(filename) != OTHER_PATH_SEPARATOR) && (ugetc(filename) != '#')) {
      _al_getdcwd(drive, buf2, sizeof(buf2) - ucwidth(OTHER_PATH_SEPARATOR));
      put_backslash(buf2);

      p = buf2;
      if ((utolower(p[0]) >= 'a') && (utolower(p[0]) <= 'z') && (p[1] == DEVICE_SEPARATOR))
         p += 2;

      ustrzcpy(buf+pos, sizeof(buf)-pos, p);
      pos = ustrsize(buf);
   }

 #ifdef ALLEGRO_UNIX
   no_relativisation:
 #endif

   /* add our filename, and clean it up a bit */
   ustrzcpy(buf+pos, sizeof(buf)-pos, filename);

   fix_filename_case(buf);
   fix_filename_slashes(buf);

   /* remove duplicate slashes */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL)
      uremove(p, 0);

   /* remove /./ patterns */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL) {
      uremove(p, 0);
      uremove(p, 0);
   }

   /* collapse /../ patterns */
   pos = usetc(buf2, OTHER_PATH_SEPARATOR);
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, '.');
   pos += usetc(buf2+pos, OTHER_PATH_SEPARATOR);
   usetc(buf2+pos, 0);

   while ((p = ustrstr(buf, buf2)) != NULL) {
      for (i=0; buf+uoffset(buf, i) < p; i++)
         ;

      while (--i > 0) {
         c1 = ugetat(buf, i);

         if (c1 == OTHER_PATH_SEPARATOR)
            break;

         if (c1 == DEVICE_SEPARATOR) {
            i++;
            break;
         }
      }

      if (i < 0)
         i = 0;

      p += ustrsize(buf2);
      memmove(buf+uoffset(buf, i+1), p, ustrsizez(p));
   }

   /* all done! */
   ustrzcpy(dest, size, buf);

   errno = saved_errno;

   return dest;
}



/* make_absolute_filename:
 *  Makes the absolute filename corresponding to the specified relative
 *  filename using the specified base (PATH is absolute and represents
 *  the base, FILENAME is the relative filename), stores it in DEST
 *  whose size in bytes is SIZE and returns a pointer to it.
 *  It does not append '/' to the path.
 */
char *make_absolute_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char tmp[1024];
   ASSERT(dest);
   ASSERT(path);
   ASSERT(filename);
   ASSERT(size >= 0);

   replace_filename(tmp, path, filename, sizeof(tmp));

   canonicalize_filename(dest, tmp, size);

   return dest;
}



/* make_relative_filename:
 *  Makes the relative filename corresponding to the specified absolute
 *  filename using the specified base (PATH is absolute and represents
 *  the base, FILENAME is the absolute filename), stores it in DEST
 *  whose size in bytes is SIZE and returns a pointer to it, or returns
 *  NULL if it cannot do so.
 *  It does not append '/' to the path.
 */
char *make_relative_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char *my_path, *my_filename;
   char *reduced_path = NULL, *reduced_filename = NULL;
   char *p1, *p2;
   int c, c1, c2, pos;
   ASSERT(dest);
   ASSERT(path);
   ASSERT(filename);
   ASSERT(size >= 0);

   /* The first check under DOS/Windows would be for the drive: since the
    * paths are absolute, they will always contain a drive letter. Do this
    * check under Unix too where the first character should always be '/'
    * in order not to screw up existing DOS/Windows paths.
    */
   if (ugetc(path) != ugetc(filename))
      return NULL;

   my_path = _al_ustrdup(path);
   if (!my_path)
      return NULL;

   my_filename = _al_ustrdup(filename);
   if (!my_filename) {
      _AL_FREE(my_path);
      return NULL;
   }

   /* Strip the filenames to keep only the directories. */
   usetc(get_filename(my_path), 0);
   usetc(get_filename(my_filename), 0);

   /* Both paths are on the same device. There are three cases:
    *  - the filename is a "child" of the path in the directory tree,
    *  - the filename is a "brother" of the path,
    *  - the filename is only a "cousin" of the path.
    * In the two former cases, we will only need to keep a suffix of the
    * filename. In the latter case, we will need to back-paddle through
    * the directory tree.
    */
   p1 = my_path;
   p2 = my_filename;
   while (((c1=ugetx(&p1)) == (c2=ugetx(&p2))) && c1 && c2) {
      if ((c1 == '/') || (c1 == OTHER_PATH_SEPARATOR)) {
         reduced_path = p1;
         reduced_filename = p2;
      }
   }

   if (!c1) {
      /* If the path is exhausted, we are in one of the two former cases. */

      if (!c2) {
         /* If the filename is also exhausted, we are in the second case.
          * Prepend './' to the reduced filename.
          */
         pos = usetc(dest, '.');
         pos += usetc(dest+pos, OTHER_PATH_SEPARATOR);
         usetc(dest+pos, 0);
      }
      else {
         /* Otherwise we are in the first case. Nothing to do. */
         usetc(dest, 0);
      }
   }
   else {
      /* Bail out if previously something went wrong (eg. user supplied
       * paths are not canonical and we can't understand them). */
      if (!reduced_path) {
         _AL_FREE(my_path);
         _AL_FREE(my_filename);
         return NULL;
      }
      /* Otherwise, we are in the latter case and need to count the number
       * of remaining directories in the reduced path and prepend the same
       * number of '../' to the reduced filename.
       */
      pos = 0;
      while ((c=ugetx(&reduced_path))) {
         if ((c == '/') || (c == OTHER_PATH_SEPARATOR)) {
            pos += usetc(dest+pos, '.');
            pos += usetc(dest+pos, '.');
            pos += usetc(dest+pos, OTHER_PATH_SEPARATOR);
         }
      }

      usetc(dest+pos, 0);
   }

   /* Bail out if previously something went wrong (eg. user supplied
    * paths are not canonical and we can't understand them). */
   if (!reduced_filename) {
      _AL_FREE(my_path);
      _AL_FREE(my_filename);
      return NULL;
   }

   ustrzcat(dest, size, reduced_filename);
   ustrzcat(dest, size, get_filename(filename));

   _AL_FREE(my_path);
   _AL_FREE(my_filename);

   /* Harmonize path separators. */
   return fix_filename_slashes(dest);
}



/* is_relative_filename:
 *  Checks whether the specified filename is relative.
 */
int is_relative_filename(AL_CONST char *filename)
{
   ASSERT(filename);

   /* All filenames that start with a '.' are relative. */
   if (ugetc(filename) == '.')
      return TRUE;

   /* Filenames that contain a device separator (DOS/Windows)
    * or start with a '/' (Unix) are considered absolute.
    */
#if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
   if (ustrchr(filename, DEVICE_SEPARATOR))
      return FALSE;
#endif

   if ((ugetc(filename) == '/') || (ugetc(filename) == OTHER_PATH_SEPARATOR))
      return FALSE;

   return TRUE;
}



/* replace_filename:
 *  Replaces filename in path with different one.
 *  It does not append '/' to the path.
 */
char *replace_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char tmp[1024];
   int pos, c;
   ASSERT(dest);
   ASSERT(path);
   ASSERT(filename);
   ASSERT(size >= 0);

   pos = ustrlen(path);

   while (pos>0) {
      c = ugetat(path, pos-1);
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR))
         break;
      pos--;
   }

   ustrzncpy(tmp, sizeof(tmp), path, pos);
   ustrzcat(tmp, sizeof(tmp), filename);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* replace_extension:
 *  Replaces extension in filename with different one.
 *  It appends '.' if it is not present in the filename.
 */
char *replace_extension(char *dest, AL_CONST char *filename, AL_CONST char *ext, int size)
{
   char tmp[1024], tmp2[16];
   int pos, end, c;
   ASSERT(dest);
   ASSERT(filename);
   ASSERT(ext);
   ASSERT(size >= 0);

   pos = end = ustrlen(filename);

   while (pos>0) {
      c = ugetat(filename, pos-1);
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR))
         break;
      pos--;
   }

   if (ugetat(filename, pos-1) == '.')
      end = pos-1;

   ustrzncpy(tmp, sizeof(tmp), filename, end);
   ustrzcat(tmp, sizeof(tmp), uconvert_ascii(".", tmp2));
   ustrzcat(tmp, sizeof(tmp), ext);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* append_filename:
 *  Append filename to path, adding separator if necessary.
 */
char *append_filename(char *dest, AL_CONST char *path, AL_CONST char *filename, int size)
{
   char tmp[1024];
   int pos, c;
   ASSERT(dest);
   ASSERT(path);
   ASSERT(filename);
   ASSERT(size >= 0);

   ustrzcpy(tmp, sizeof(tmp), path);
   pos = ustrlen(tmp);

   if ((pos > 0) && (uoffset(tmp, pos) < ((int)sizeof(tmp) - ucwidth(OTHER_PATH_SEPARATOR) - ucwidth(0)))) {
      c = ugetat(tmp, pos-1);

      if ((c != '/') && (c != OTHER_PATH_SEPARATOR) && (c != DEVICE_SEPARATOR)) {
         pos = uoffset(tmp, pos);
         pos += usetc(tmp+pos, OTHER_PATH_SEPARATOR);
         usetc(tmp+pos, 0);
      }
   }

   ustrzcat(tmp, sizeof(tmp), filename);

   ustrzcpy(dest, size, tmp);

   return dest;
}



/* get_filename:
 *  When passed a completely specified file path, this returns a pointer
 *  to the filename portion. Both '\' and '/' are recognized as directory
 *  separators.
 */
char *get_filename(AL_CONST char *path)
{
   int c;
   const char *ptr, *ret;
   ASSERT(path);

   ptr = path;
   ret = ptr;
   for (;;) {
      c = ugetxc(&ptr);
      if (!c) break;
      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR))
         ret = (char*)ptr;
   }
   return (char*)ret;
}



/* get_extension:
 *  When passed a complete filename (with or without path information)
 *  this returns a pointer to the file extension.
 */
char *get_extension(AL_CONST char *filename)
{
   int pos, c;
   ASSERT(filename);

   pos = ustrlen(filename);

   while (pos>0) {
      c = ugetat(filename, pos-1);
      if ((c == '.') || (c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR))
         break;
      pos--;
   }

   if ((pos>0) && (ugetat(filename, pos-1) == '.'))
      return (char *)filename + uoffset(filename, pos);

   return (char *)filename + ustrsize(filename);
}



/* put_backslash:
 *  If the last character of the filename is not a \, /, or #, or a device
 *  separator (eg. : under DOS), this routine will concatenate a \ or / on
 *  to it (depending on platform).
 */
void put_backslash(char *filename)
{
   int c;
   ASSERT(filename);

   if (ugetc(filename)) {
      c = ugetat(filename, -1);

      if ((c == '/') || (c == OTHER_PATH_SEPARATOR) || (c == DEVICE_SEPARATOR) || (c == '#'))
         return;
   }

   filename += ustrsize(filename);
   filename += usetc(filename, OTHER_PATH_SEPARATOR);
   usetc(filename, 0);
}



/***************************************************
 ******************* Filesystem ********************
 ***************************************************/


/* set_filename_encoding:
 *  Sets the encoding to use for filenames. By default, UTF8 is assumed.
 */
void set_filename_encoding(int encoding)
{
    filename_encoding = encoding;
}



/* get_filename_encoding:
 *  Returns the encoding currently assumed for filenames.
 */
int get_filename_encoding(void)
{
    return filename_encoding ;
}



/* file_exists:
 *  Checks whether a file matching the given name and attributes exists,
 *  returning non zero if it does. The file attribute may contain any of
 *  the FA_* constants from dir.h. If aret is not null, it will be set
 *  to the attributes of the matching file. If an error occurs the system
 *  error code will be stored in errno.
 */
int file_exists(AL_CONST char *filename, int attrib, int *aret)
{
   struct al_ffblk info;
   ASSERT(filename);

   if (ustrchr(filename, '#')) {
      PACKFILE *f = pack_fopen_special_file(filename, F_READ);
      if (f) {
         pack_fclose(f);
         if (aret)
            *aret = FA_DAT_FLAGS;
         return ((attrib & FA_DAT_FLAGS) == FA_DAT_FLAGS) ? TRUE : FALSE;
      }
   }

   if (!_al_file_isok(filename))
      return FALSE;

   if (al_findfirst(filename, &info, attrib) != 0) {
      /* no entry is not an error for file_exists() */
      if (*allegro_errno == ENOENT)
         *allegro_errno = 0;

      return FALSE;
   }

   al_findclose(&info);

   if (aret)
      *aret = info.attrib;

   return TRUE;
}



/* exists:
 *  Shortcut version of file_exists().
 */
int exists(AL_CONST char *filename)
{
   ASSERT(filename);
   return file_exists(filename, FA_ARCH | FA_RDONLY, NULL);
}



/* file_size_ex:
 *  Returns the size of a file, in bytes.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
uint64_t file_size_ex(AL_CONST char *filename)
{
   ASSERT(filename);
   if (ustrchr(filename, '#')) {
      PACKFILE *f = pack_fopen_special_file(filename, F_READ);
      if (f) {
         long ret;
         ASSERT(f->is_normal_packfile);
         ret = f->normal.todo;
         pack_fclose(f);
         return ret;
      }
   }

   if (!_al_file_isok(filename))
      return 0;

   return _al_file_size_ex(filename);
}



/* For binary compatibility with 4.2.0. */
long file_size(AL_CONST char *filename)
{
   return file_size_ex(filename);
}



/* For binary compatibility with 4.2.0.
 * This is an internal symbol and only required because _al_file_size was
 * exposed in the Windows DLL.
 */
long _al_file_size(AL_CONST char *filename)
{
   return _al_file_size_ex(filename);
}



/* file_time:
 *  Returns a file time-stamp.
 *  If the file does not exist or an error occurs, it will return zero
 *  and store the system error code in errno.
 */
time_t file_time(AL_CONST char *filename)
{
   ASSERT(filename);

   if (!_al_file_isok(filename))
      return 0;

   return _al_file_time(filename);
}



/* delete_file:
 *  Removes a file from the disk.
 */
int delete_file(AL_CONST char *filename)
{
   char tmp[1024];
   ASSERT(filename);

   if (!_al_file_isok(filename))
      return -1;

   if (_al_unlink(uconvert_tofilename(filename, tmp)) != 0) {
      *allegro_errno = errno;
      return -1;
   }

   return 0;
}



/* for_each_file:
 *  Finds all the files on the disk which match the given wildcard
 *  specification and file attributes, and executes callback() once for
 *  each. callback() will be passed three arguments, the first a string
 *  which contains the completed filename, the second being the attributes
 *  of the file, and the third an int which is simply a copy of param (you
 *  can use this for whatever you like). If an error occurs an error code
 *  will be stored in errno, and callback() can cause for_each_file() to
 *  abort by setting errno itself. Returns the number of successful calls
 *  made to callback(). The file attribute may contain any of the FA_*
 *  flags from dir.h.
 */
int for_each_file(AL_CONST char *name, int attrib, void (*callback)(AL_CONST char *filename, int attrib, int param), int param)
{
   char buf[1024];
   struct al_ffblk info;
   int c = 0;
   ASSERT(name);

   if (!_al_file_isok(name))
      return 0;

   if (al_findfirst(name, &info, attrib) != 0) {
      /* no entry is not an error for for_each_file() */
      if (*allegro_errno == ENOENT)
         *allegro_errno = 0;

      return 0;
   }

   *allegro_errno = 0;

   do {
      replace_filename(buf, name, info.name, sizeof(buf));
      (*callback)(buf, info.attrib, param);

      if (*allegro_errno) /* evil, evil, evil! */
         break;

      c++;
   } while (al_findnext(&info) == 0);

   al_findclose(&info);

   /* no entry is not an error for for_each_file() */
   if (*allegro_errno == ENOENT)
      *allegro_errno = 0;

   return c;
}



/* for_each_file_ex:
 *  Finds all the files on disk which match the given wildcard specification
 *  and file attributes, and executes callback() once for each. callback()
 *  will be passed three arguments: the first is a string which contains the
 *  completed filename, the second is the actual attributes of the file, and
 *  the third is a void pointer which is simply a copy of param (you can use
 *  this for whatever you like). It must return 0 to let the enumeration
 *  proceed, or any non-zero value to stop it. If an error occurs, the error
 *  code will be stored in errno but the enumeration won't stop. Returns the
 *  number of successful calls made to callback(), that is the number of
 *  times callback() was called and returned 0. The file attribute masks may
 *  contain any of the FA_* flags from dir.h.
 */
int for_each_file_ex(AL_CONST char *name, int in_attrib, int out_attrib, int (*callback)(AL_CONST char *filename, int attrib, void *param), void *param)
{
   char buf[1024];
   struct al_ffblk info;
   int ret, c = 0;
   ASSERT(name);

   if (!_al_file_isok(name))
      return 0;

   if (al_findfirst(name, &info, ~out_attrib) != 0) {
      /* no entry is not an error for for_each_file_ex() */
      if (*allegro_errno == ENOENT)
         *allegro_errno = 0;

      return 0;
   }

   do {
      if ((~info.attrib & in_attrib) == 0) {
         replace_filename(buf, name, info.name, sizeof(buf));
         ret = (*callback)(buf, info.attrib, param);

         if (ret != 0)
            break;

         c++;
      }
   } while (al_findnext(&info) == 0);

   al_findclose(&info);

   /* no entry is not an error for for_each_file_ex() */
   if (*allegro_errno == ENOENT)
      *allegro_errno = 0;

   return c;
}



/* find_resource:
 *  Tries lots of different places that a resource file might live.
 */
static int find_resource(char *dest, AL_CONST char *path, AL_CONST char *name, AL_CONST char *datafile, AL_CONST char *objectname, AL_CONST char *subdir, int size)
{
   char _name[128], _objectname[128], hash[8];
   char tmp[16];
   int i;

   /* convert from name.ext to name_ext (datafile object name format) */
   ustrzcpy(_name, sizeof(_name), name);

   for (i=0; i<ustrlen(_name); i++) {
      if (ugetat(_name, i) == '.')
         usetat(_name, i, '_');
   }

   if (objectname) {
      ustrzcpy(_objectname, sizeof(_objectname), objectname);

      for (i=0; i<ustrlen(_objectname); i++) {
         if (ugetat(_objectname, i) == '.')
            usetat(_objectname, i, '_');
      }
   }
   else
      usetc(_objectname, 0);

   usetc(hash+usetc(hash, '#'), 0);

   /* try path/name */
   if (ugetc(name)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, name);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path#_name */
   if ((ustrchr(path, '#')) && (ugetc(name))) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, _name);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path/name#_objectname */
   if ((ustricmp(get_extension(name), uconvert_ascii("dat", tmp)) == 0) && (objectname)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, name);
      ustrzcat(dest, size, hash);
      ustrzcat(dest, size, _objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path/datafile#_name */
   if ((datafile) && (ugetc(name))) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, datafile);
      ustrzcat(dest, size, hash);
      ustrzcat(dest, size, _name);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path/datafile#_objectname */
   if ((datafile) && (objectname)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, datafile);
      ustrzcat(dest, size, hash);
      ustrzcat(dest, size, _objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path/objectname */
   if (objectname) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path#_objectname */
   if ((ustrchr(path, '#')) && (objectname)) {
      ustrzcpy(dest, size, path);
      ustrzcat(dest, size, _objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   /* try path/subdir/objectname */
   if ((subdir) && (objectname)) {
      ustrzcpy(dest, size - ucwidth(OTHER_PATH_SEPARATOR), path);
      ustrzcat(dest, size - ucwidth(OTHER_PATH_SEPARATOR), subdir);
      put_backslash(dest);
      ustrzcat(dest, size, objectname);

      if (file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
         return 0;
   }

   return -1;
}



/* set_allegro_resource_path:
 *  Set an additional path to be searched by find_allegro_resource(). If
 *  multiple paths are set, they will be searched from lowest to highest
 *  priority. Call with path set to NULL to remove a path. Returns 1 on success
 *  and 0 on failure.
 */
int set_allegro_resource_path(int priority, AL_CONST char *path)
{
   RESOURCE_PATH *node = resource_path_list;
   RESOURCE_PATH *prior_node = NULL;
   RESOURCE_PATH *new_node = NULL;

   while (node && node->priority > priority) {
      prior_node = node;
      node = node->next;
   }

   if (path) {
      if (node && priority == node->priority)
         new_node = node;
      else {
         new_node = _AL_MALLOC(sizeof(RESOURCE_PATH));
         if (!new_node)
            return 0;

         new_node->priority = priority;

         if (prior_node) {
            prior_node->next = new_node;
            new_node->next = node;
         }
         else {
            new_node->next = resource_path_list;
            resource_path_list = new_node;
         }

         if (!resource_path_list->next)
            _add_exit_func(destroy_resource_path_list,
                           "destroy_resource_path_list");
      }

      ustrzcpy(new_node->path,
               sizeof(new_node->path) - ucwidth(OTHER_PATH_SEPARATOR),
               path);
      fix_filename_slashes(new_node->path);
      put_backslash(new_node->path);
   }
   else {
       if (node && node->priority == priority) {
          if (prior_node)
             prior_node->next = node->next;
          else
             resource_path_list = node->next;

          _AL_FREE(node);

          if (!resource_path_list)
             _remove_exit_func(destroy_resource_path_list);
       }
       else
          return 0;
   }

   return 1;
}



static void destroy_resource_path_list(void)
{
   RESOURCE_PATH *node = resource_path_list;

   if (node)
      _remove_exit_func(destroy_resource_path_list);

   while (node) {
      resource_path_list = node->next;
      _AL_FREE(node);
      node = resource_path_list;
   }
}



/* find_allegro_resource:
 *  Searches for a support file, eg. allegro.cfg or language.dat. Passed
 *  a resource string describing what you are looking for, along with
 *  extra optional information such as the default extension, what datafile
 *  to look inside, what the datafile object name is likely to be, any
 *  special environment variable to check, and any subdirectory that you
 *  would like to check as well as the default location, this function
 *  looks in a hell of a lot of different places :-) Returns zero on
 *  success, and stores a full path to the file (at most size bytes) in
 *  the dest parameter.
 */
int find_allegro_resource(char *dest, AL_CONST char *resource, AL_CONST char *ext, AL_CONST char *datafile, AL_CONST char *objectname, AL_CONST char *envvar, AL_CONST char *subdir, int size)
{
   int (*sys_find_resource)(char *, AL_CONST char *, int);
   char rname[128], path[1024], tmp[128];
   char *s;
   int i, c;
   RESOURCE_PATH *rp_list_node = resource_path_list;

   ASSERT(dest);

   /* if the resource is a path with no filename, look in that location */
   if ((resource) && (ugetc(resource)) && (!ugetc(get_filename(resource))))
      return find_resource(dest, resource, empty_string, datafile, objectname, subdir, size);

   /* if we have a path+filename, just use it directly */
   if ((resource) && (ustrpbrk(resource, uconvert_ascii("\\/#", tmp)))) {
      if (file_exists(resource, FA_RDONLY | FA_ARCH, NULL)) {
         ustrzcpy(dest, size, resource);

         /* if the resource is a datafile, try looking inside it */
         if ((ustricmp(get_extension(dest), uconvert_ascii("dat", tmp)) == 0) && (objectname)) {
            ustrzcat(dest, size, uconvert_ascii("#", tmp));

            for (i=0; i<ustrlen(objectname); i++) {
               c = ugetat(objectname, i);
               if (c == '.')
                  c = '_';
               if (ustrsizez(dest)+ucwidth(c) <= size)
                  uinsert(dest, ustrlen(dest), c);
            }

            if (!file_exists(dest, FA_RDONLY | FA_ARCH, NULL))
               return -1;
         }

         return 0;
      }
      else
         return -1;
   }

   /* clean up the resource name, adding any default extension */
   if (resource) {
      ustrzcpy(rname, sizeof(rname), resource);

      if (ext) {
         s = get_extension(rname);
         if (!ugetc(s))
            ustrzcat(rname, sizeof(rname), ext);
      }
   }
   else
      usetc(rname, 0);

   /* try resource path list */
   while (rp_list_node) {
      if (find_resource(dest, rp_list_node->path, rname, datafile, objectname,
                        subdir, size) == 0)
         return 0;
      rp_list_node = rp_list_node->next;
   }

   /* try looking in the same directory as the program */
   get_executable_name(path, sizeof(path));
   usetc(get_filename(path), 0);

   if (find_resource(dest, path, rname, datafile, objectname, subdir, size) == 0)
      return 0;

   /* try the ALLEGRO environment variable */
   s = getenv("ALLEGRO");

   if (s) {
      do_uconvert(s, U_ASCII, path, U_CURRENT, sizeof(path)-ucwidth(OTHER_PATH_SEPARATOR));
      put_backslash(path);

      if (find_resource(dest, path, rname, datafile, objectname, subdir, size) == 0)
         return 0;
   }

   /* try any extra environment variable that the parameters say to use */
   if (envvar) {
      s = getenv(uconvert_tofilename(envvar, tmp));

      if (s) {
         do_uconvert(s, U_ASCII, path, U_CURRENT, sizeof(path)-ucwidth(OTHER_PATH_SEPARATOR));
         put_backslash(path);

         if (find_resource(dest, path, rname, datafile, objectname, subdir, size) == 0)
            return 0;
      }
   }

   /* ask the system driver */
   if (system_driver)
      sys_find_resource = system_driver->find_resource;
   else
      sys_find_resource = NULL;

   if (sys_find_resource) {
      if ((ugetc(rname)) && (sys_find_resource(dest, (char *)rname, size) == 0))
         return 0;

      if ((datafile) && ((ugetc(rname)) || (objectname)) && (sys_find_resource(path, (char *)datafile, sizeof(path)) == 0)) {
         if (!ugetc(rname))
            ustrzcpy(rname, sizeof(rname), objectname);

         for (i=0; i<ustrlen(rname); i++) {
            if (ugetat(rname, i) == '.')
               usetat(rname, i, '_');
         }

         ustrzcat(path, sizeof(path), uconvert_ascii("#", tmp));
         ustrzcat(path, sizeof(path), rname);

         if (file_exists(path, FA_RDONLY | FA_ARCH, NULL)) {
            ustrzcpy(dest, size, path);
            return 0;
         }
      }
   }

   /* argh, all that work, and still no biscuit */
   return -1;
}



/***************************************************
 ******************** Packfiles ********************
 ***************************************************/


/* pack_fopen_exe_file:
 *  Helper to handle opening files that have been appended to the end of
 *  the program executable.
 */
static PACKFILE *pack_fopen_exe_file(void)
{
   PACKFILE *f;
   char exe_name[1024];
   long size;

   /* open the file */
   get_executable_name(exe_name, sizeof(exe_name));

   if (!ugetc(get_filename(exe_name))) {
      *allegro_errno = ENOENT;
      return NULL;
   }

   f = pack_fopen(exe_name, F_READ);
   if (!f)
      return NULL;

   ASSERT(f->is_normal_packfile);

   /* seek to the end and check for the magic number */
   pack_fseek(f, f->normal.todo-8);

   if (pack_mgetl(f) != F_EXE_MAGIC) {
      pack_fclose(f);
      *allegro_errno = ENOTDIR;
      return NULL;
   }

   size = pack_mgetl(f);

   /* rewind */
   pack_fclose(f);
   f = pack_fopen(exe_name, F_READ);
   if (!f)
      return NULL;

   /* seek to the start of the appended data */
   pack_fseek(f, f->normal.todo-size);

   f = pack_fopen_chunk(f, FALSE);

   if (f)
      f->normal.flags |= PACKFILE_FLAG_EXEDAT;

   return f;
}



/* pack_fopen_datafile_object:
 *  Recursive helper to handle opening member objects from datafiles,
 *  given a fake filename in the form 'object_name[/nestedobject]'.
 */
static PACKFILE *pack_fopen_datafile_object(PACKFILE *f, AL_CONST char *objname)
{
   char buf[512];   /* text is read into buf as UTF-8 */
   char tmp[512*4]; /* this should be enough even when expanding to UCS-4 */
   char name[512];
   int use_next = FALSE;
   int recurse = FALSE;
   int type, size, pos, c;

   /* split up the object name */
   pos = 0;

   while ((c = ugetxc(&objname)) != 0) {
      if ((c == '#') || (c == '/') || (c == OTHER_PATH_SEPARATOR)) {
         recurse = TRUE;
         break;
      }
      pos += usetc(name+pos, c);
   }

   usetc(name+pos, 0);

   pack_mgetl(f);

   /* search for the requested object */
   while (!pack_feof(f)) {
      type = pack_mgetl(f);

      if (type == DAT_PROPERTY) {
         type = pack_mgetl(f);
         size = pack_mgetl(f);
         if (type == DAT_NAME) {
            /* examine name property */
            pack_fread(buf, size, f);
            buf[size] = 0;
            if (ustricmp(uconvert(buf, U_UTF8, tmp, U_CURRENT, sizeof tmp), name) == 0)
               use_next = TRUE;
         }
         else {
            /* skip property */
            pack_fseek(f, size);
         }
      }
      else {
         if (use_next) {
            /* found it! */
            if (recurse) {
               if (type == DAT_FILE)
                  return pack_fopen_datafile_object(pack_fopen_chunk(f, FALSE), objname);
               else
                  break;
            }
            else {
               _packfile_type = type;
               return pack_fopen_chunk(f, FALSE);
            }
         }
         else {
            /* skip unwanted object */
            size = pack_mgetl(f);
            pack_fseek(f, size+4);
         }
      }
   }

   /* oh dear, the object isn't there... */
   pack_fclose(f);
   *allegro_errno = ENOENT;
   return NULL;
}



/* pack_fopen_special_file:
 *  Helper to handle opening psuedo-files, ie. datafile objects and data
 *  that has been appended to the end of the executable.
 */
static PACKFILE *pack_fopen_special_file(AL_CONST char *filename, AL_CONST char *mode)
{
   char fname[1024], objname[512], tmp[16];
   PACKFILE *f;
   char *p;
   int c;

   /* special files are read-only */
   while ((c = *(mode++)) != 0) {
      if ((c == 'w') || (c == 'W')) {
         *allegro_errno = EROFS;
         return NULL;
      }
   }

   if (ustrcmp(filename, uconvert_ascii("#", tmp)) == 0) {
      /* read appended executable data */
      return pack_fopen_exe_file();
   }
   else {
      if (ugetc(filename) == '#') {
         /* read object from an appended datafile */
         ustrzcpy(fname,  sizeof(fname), uconvert_ascii("#", tmp));
         ustrzcpy(objname, sizeof(objname), filename+uwidth(filename));
      }
      else {
         /* read object from a regular datafile */
         ustrzcpy(fname,  sizeof(fname), filename);
         p = ustrrchr(fname, '#');
         usetat(p, 0, 0);
         ustrzcpy(objname, sizeof(objname), p+uwidth(p));
      }

      /* open the file */
      f = pack_fopen(fname, F_READ_PACKED);
      if (!f)
         return NULL;

      if (pack_mgetl(f) != DAT_MAGIC) {
         pack_fclose(f);
         *allegro_errno = ENOTDIR;
         return NULL;
      }

      /* find the required object */
      return pack_fopen_datafile_object(f, objname);
   }
}



/* packfile_password:
 *  Sets the password to be used by all future read/write operations.
 *  This only affects "normal" PACKFILEs, i.e. ones not using user-supplied
 *  packfile vtables.
 */
void packfile_password(AL_CONST char *password)
{
   int i = 0;
   int c;

   if (password) {
      while ((c = ugetxc(&password)) != 0) {
         the_password[i++] = c;
         if (i >= (int)sizeof(the_password)-1)
            break;
      }
   }

   the_password[i] = 0;
}



/* encrypt_id:
 *  Helper for encrypting magic numbers, using the current password.
 */
static int32_t encrypt_id(long x, int new_format)
{
   int32_t mask = 0;
   int i, pos;

   if (the_password[0]) {
      for (i=0; the_password[i]; i++)
         mask ^= ((int32_t)the_password[i] << ((i&3) * 8));

      for (i=0, pos=0; i<4; i++) {
         mask ^= (int32_t)the_password[pos++] << (24-i*8);
         if (!the_password[pos])
            pos = 0;
      }

      if (new_format)
         mask ^= 42;
   }

   return x ^ mask;
}



/* clone_password:
 *  Sets up a local password string for use by this packfile.
 */
static int clone_password(PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->is_normal_packfile);

   if (the_password[0]) {
      if ((f->normal.passdata = _AL_MALLOC_ATOMIC(strlen(the_password)+1)) == NULL) {
         *allegro_errno = ENOMEM;
         return FALSE;
      }
      _al_sane_strncpy(f->normal.passdata, the_password, strlen(the_password)+1);
      f->normal.passpos = f->normal.passdata;
   }
   else {
      f->normal.passpos = NULL;
      f->normal.passdata = NULL;
   }

   return TRUE;
}



/* create_packfile:
 *  Helper function for creating a PACKFILE structure.
 */
static PACKFILE *create_packfile(int is_normal_packfile)
{
   PACKFILE *f;

   if (is_normal_packfile)
      f = _AL_MALLOC(sizeof(PACKFILE));
   else
      f = _AL_MALLOC(sizeof(PACKFILE) - sizeof(struct _al_normal_packfile_details));

   if (f == NULL) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   if (!is_normal_packfile) {
      f->vtable = NULL;
      f->userdata = NULL;
      f->is_normal_packfile = FALSE;
   }
   else {
      f->vtable = &normal_vtable;
      f->userdata = f;
      f->is_normal_packfile = TRUE;

      f->normal.buf_pos = f->normal.buf;
      f->normal.flags = 0;
      f->normal.buf_size = 0;
      f->normal.filename = NULL;
      f->normal.passdata = NULL;
      f->normal.passpos = NULL;
      f->normal.parent = NULL;
      f->normal.pack_data = NULL;
      f->normal.unpack_data = NULL;
      f->normal.todo = 0;
   }

   return f;
}



/* free_packfile:
 *  Helper function for freeing the PACKFILE struct.
 */
static void free_packfile(PACKFILE *f)
{
   if (f) {
      /* These are no longer the responsibility of this function, but
       * these assertions help catch instances of old code which still
       * rely on the old behaviour.
       */
      if (f->is_normal_packfile) {
         ASSERT(!f->normal.pack_data);
         ASSERT(!f->normal.unpack_data);
         ASSERT(!f->normal.passdata);
         ASSERT(!f->normal.passpos);
      }

      _AL_FREE(f);
   }
}



/* _pack_fdopen:
 *  Converts the given file descriptor into a PACKFILE. The mode can have
 *  the same values as for pack_fopen() and must be compatible with the
 *  mode of the file descriptor. Unlike the libc fdopen(), pack_fdopen()
 *  is unable to convert an already partially read or written file (i.e.
 *  the file offset must be 0).
 *  On success, it returns a pointer to a file structure, and on error it
 *  returns NULL and stores an error code in errno. An attempt to read
 *  a normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *_pack_fdopen(int fd, AL_CONST char *mode)
{
   PACKFILE *f, *f2;
   long header = FALSE;
   int c;

   if ((f = create_packfile(TRUE)) == NULL)
      return NULL;

   ASSERT(f->is_normal_packfile);

   while ((c = *(mode++)) != 0) {
      switch (c) {
         case 'r': case 'R': f->normal.flags &= ~PACKFILE_FLAG_WRITE; break;
         case 'w': case 'W': f->normal.flags |= PACKFILE_FLAG_WRITE; break;
         case 'p': case 'P': f->normal.flags |= PACKFILE_FLAG_PACK; break;
         case '!': f->normal.flags &= ~PACKFILE_FLAG_PACK; header = TRUE; break;
      }
   }

   if (f->normal.flags & PACKFILE_FLAG_WRITE) {
      if (f->normal.flags & PACKFILE_FLAG_PACK) {
         /* write a packed file */
         f->normal.pack_data = create_lzss_pack_data();
         ASSERT(!f->normal.unpack_data);

         if (!f->normal.pack_data) {
            free_packfile(f);
            return NULL;
         }

         if ((f->normal.parent = _pack_fdopen(fd, F_WRITE)) == NULL) {
            free_lzss_pack_data(f->normal.pack_data);
            f->normal.pack_data = NULL;
            free_packfile(f);
            return NULL;
         }

         pack_mputl(encrypt_id(F_PACK_MAGIC, TRUE), f->normal.parent);

         f->normal.todo = 4;
      }
      else {
         /* write a 'real' file */
         if (!clone_password(f)) {
            free_packfile(f);
            return NULL;
         }

         f->normal.hndl = fd;
         f->normal.todo = 0;

         errno = 0;

         if (header)
            pack_mputl(encrypt_id(F_NOPACK_MAGIC, TRUE), f);
      }
   }
   else {
      if (f->normal.flags & PACKFILE_FLAG_PACK) {
         /* read a packed file */
         f->normal.unpack_data = create_lzss_unpack_data();
         ASSERT(!f->normal.pack_data);

         if (!f->normal.unpack_data) {
            free_packfile(f);
            return NULL;
         }

         if ((f->normal.parent = _pack_fdopen(fd, F_READ)) == NULL) {
            free_lzss_unpack_data(f->normal.unpack_data);
            f->normal.unpack_data = NULL;
            free_packfile(f);
            return NULL;
         }

         header = pack_mgetl(f->normal.parent);

         if ((f->normal.parent->normal.passpos) &&
             ((header == encrypt_id(F_PACK_MAGIC, FALSE)) ||
              (header == encrypt_id(F_NOPACK_MAGIC, FALSE))))
         {
            /* duplicate the file descriptor */
            int fd2 = dup(fd);

            if (fd2<0) {
               pack_fclose(f->normal.parent);
               free_packfile(f);
               *allegro_errno = errno;
               return NULL;
            }

            /* close the parent file (logically, not physically) */
            pack_fclose(f->normal.parent);

            /* backward compatibility mode */
            if (!clone_password(f)) {
               free_packfile(f);
               return NULL;
            }

            f->normal.flags |= PACKFILE_FLAG_OLD_CRYPT;

            /* re-open the parent file */
            lseek(fd2, 0, SEEK_SET);

            if ((f->normal.parent = _pack_fdopen(fd2, F_READ)) == NULL) {
               free_packfile(f);
               return NULL;
            }

            f->normal.parent->normal.flags |= PACKFILE_FLAG_OLD_CRYPT;

            pack_mgetl(f->normal.parent);

            if (header == encrypt_id(F_PACK_MAGIC, FALSE))
               header = encrypt_id(F_PACK_MAGIC, TRUE);
            else
               header = encrypt_id(F_NOPACK_MAGIC, TRUE);
         }

         if (header == encrypt_id(F_PACK_MAGIC, TRUE)) {
            f->normal.todo = LONG_MAX;
         }
         else if (header == encrypt_id(F_NOPACK_MAGIC, TRUE)) {
            f2 = f->normal.parent;
            free_lzss_unpack_data(f->normal.unpack_data);
            f->normal.unpack_data = NULL;
            free_packfile(f);
            return f2;
         }
         else {
            pack_fclose(f->normal.parent);
            free_lzss_unpack_data(f->normal.unpack_data);
            f->normal.unpack_data = NULL;
            free_packfile(f);
            *allegro_errno = EDOM;
            return NULL;
         }
      }
      else {
         /* read a 'real' file */
         f->normal.todo = lseek(fd, 0, SEEK_END);  /* size of the file */
         if (f->normal.todo < 0) {
            *allegro_errno = errno;
            free_packfile(f);
            return NULL;
         }

         lseek(fd, 0, SEEK_SET);

         if (!clone_password(f)) {
            free_packfile(f);
            return NULL;
         }

         f->normal.hndl = fd;
      }
   }

   return f;
}



/* pack_fopen:
 *  Opens a file according to mode, which may contain any of the flags:
 *  'r': open file for reading.
 *  'w': open file for writing, overwriting any existing data.
 *  'p': open file in 'packed' mode. Data will be compressed as it is
 *       written to the file, and automatically uncompressed during read
 *       operations. Files created in this mode will produce garbage if
 *       they are read without this flag being set.
 *  '!': open file for writing in normal, unpacked mode, but add the value
 *       F_NOPACK_MAGIC to the start of the file, so that it can be opened
 *       in packed mode and Allegro will automatically detect that the
 *       data does not need to be decompressed.
 *
 *  Instead of these flags, one of the constants F_READ, F_WRITE,
 *  F_READ_PACKED, F_WRITE_PACKED or F_WRITE_NOPACK may be used as the second
 *  argument to fopen().
 *
 *  On success, fopen() returns a pointer to a file structure, and on error
 *  it returns NULL and stores an error code in errno. An attempt to read a
 *  normal file in packed mode will cause errno to be set to EDOM.
 */
PACKFILE *pack_fopen(AL_CONST char *filename, AL_CONST char *mode)
{
   char tmp[1024];
   int fd;
   ASSERT(filename);

   _packfile_type = 0;

   if (ustrchr(filename, '#')) {
      PACKFILE *special = pack_fopen_special_file(filename, mode);
      if (special)
         return special;
   }

   if (!_al_file_isok(filename))
      return NULL;

#ifndef ALLEGRO_MPW
   if (strpbrk(mode, "wW"))  /* write mode? */
      fd = _al_open(uconvert_tofilename(filename, tmp), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, OPEN_PERMS);
   else
      fd = _al_open(uconvert_tofilename(filename, tmp), O_RDONLY | O_BINARY, OPEN_PERMS);
#else
   if (strpbrk(mode, "wW"))  /* write mode? */
      fd = _al_open(uconvert_tofilename(filename, tmp), O_WRONLY | O_BINARY | O_CREAT | O_TRUNC);
   else
      fd = _al_open(uconvert_tofilename(filename, tmp), O_RDONLY | O_BINARY);
#endif

   if (fd < 0) {
      *allegro_errno = errno;
      return NULL;
   }

   return _pack_fdopen(fd, mode);
}



/* pack_fopen_vtable:
 *  Creates a new packfile structure that uses the functions specified in
 *  the vtable instead of the standard functions.  On success, it returns a
 *  pointer to a file structure, and on error it returns NULL and
 *  stores an error code in errno.
 *
 *  The vtable and userdata must remain available for the lifetime of the
 *  created packfile.
 *
 *  Opening chunks using pack_fopen_chunk() on top of the returned packfile
 *  is not possible at this time.
 *
 *  packfile_password() does not have any effect on packfiles opened
 *  with pack_fopen_vtable().
 */
PACKFILE *pack_fopen_vtable(AL_CONST PACKFILE_VTABLE *vtable, void *userdata)
{
   PACKFILE *f;
   ASSERT(vtable);
   ASSERT(vtable->pf_fclose);
   ASSERT(vtable->pf_getc);
   ASSERT(vtable->pf_ungetc);
   ASSERT(vtable->pf_fread);
   ASSERT(vtable->pf_putc);
   ASSERT(vtable->pf_fwrite);
   ASSERT(vtable->pf_fseek);
   ASSERT(vtable->pf_feof);
   ASSERT(vtable->pf_ferror);

   if ((f = create_packfile(FALSE)) == NULL)
      return NULL;

   f->vtable = vtable;
   f->userdata = userdata;
   ASSERT(!f->is_normal_packfile);

   return f;
}



/* pack_fclose:
 *  Closes a file after it has been read or written.
 *  Returns zero on success. On error it returns an error code which is
 *  also stored in errno. This function can fail only when writing to
 *  files: if the file was opened in read mode it will always succeed.
 */
int pack_fclose(PACKFILE *f)
{
   int ret;

   if (!f)
      return 0;

   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_fclose);

   ret = f->vtable->pf_fclose(f->userdata);
   if (ret != 0)
      *allegro_errno = errno;

   free_packfile(f);

   return ret;
}



/* pack_fopen_chunk:
 *  Opens a sub-chunk of the specified file, for reading or writing depending
 *  on the type of the file. The returned file pointer describes the sub
 *  chunk, and replaces the original file, which will no longer be valid.
 *  When writing to a chunk file, data is sent to the original file, but
 *  is prefixed with two length counts (32 bit, big-endian). For uncompressed
 *  chunks these will both be set to the length of the data in the chunk.
 *  For compressed chunks, created by setting the pack flag, the first will
 *  contain the raw size of the chunk, and the second will be the negative
 *  size of the uncompressed data. When reading chunks, the pack flag is
 *  ignored, and the compression type is detected from the sign of the
 *  second size value. The file structure used to read chunks checks the
 *  chunk size, and will return EOF if you try to read past the end of
 *  the chunk. If you don't read all of the chunk data, when you call
 *  pack_fclose_chunk(), the parent file will advance past the unused data.
 *  When you have finished reading or writing a chunk, you should call
 *  pack_fclose_chunk() to return to your original file.
 */
PACKFILE *pack_fopen_chunk(PACKFILE *f, int pack)
{
   PACKFILE *chunk;
   char tmp[1024];
   char *name;
   ASSERT(f);

   /* unsupported */
   if (!f->is_normal_packfile) {
      *allegro_errno = EINVAL;
      return NULL;
   }

   if (f->normal.flags & PACKFILE_FLAG_WRITE) {

      /* write a sub-chunk */
      int tmp_fd = -1;
      char *tmp_dir = NULL;
      char *tmp_name = NULL;
      #ifndef ALLEGRO_HAVE_MKSTEMP
      char* tmpnam_string;
      #endif

      #ifdef ALLEGRO_WINDOWS
         int size;
         int new_size = 64;

         /* Get the path of the temporary directory */
         do {
            size = new_size;
            tmp_dir = _AL_REALLOC(tmp_dir, size);
            new_size = GetTempPathA(size, tmp_dir);
         } while ( (size < new_size) && (new_size > 0) );

         /* Check if we retrieved the path OK */
         if (new_size == 0)
            sprintf(tmp_dir, "%s", "");
      #else
         /* Get the path of the temporary directory */

         /* Try various possible locations to store the temporary file */
         if (getenv("TEMP")) {
            tmp_dir = _al_strdup(getenv("TEMP"));
         }
         else if (getenv("TMP")) {
            tmp_dir = _al_strdup(getenv("TMP"));
         }
         else if (file_exists("/tmp", FA_DIREC, NULL)) {
            tmp_dir = _al_strdup("/tmp");
         }
         else if (getenv("HOME")) {
            tmp_dir = _al_strdup(getenv("HOME"));
         }
         else {
            /* Give up - try current directory */
            tmp_dir = _al_strdup(".");
         }

      #endif

      /* the file is open in read/write mode, even if the pack file
       * seems to be in write only mode
       */
      #ifdef ALLEGRO_HAVE_MKSTEMP

         tmp_name = _AL_MALLOC_ATOMIC(strlen(tmp_dir) + 16);
         sprintf(tmp_name, "%s/XXXXXX", tmp_dir);
         tmp_fd = mkstemp(tmp_name);

      #else

         /* note: since the filename creation and the opening are not
          * an atomic operation, this is not secure
          */
         tmpnam_string = tmpnam(NULL);
         tmp_name = _AL_MALLOC_ATOMIC(strlen(tmp_dir) + strlen(tmpnam_string) + 2);
         sprintf(tmp_name, "%s/%s", tmp_dir, tmpnam_string);

         if (tmp_name) {
#ifndef ALLEGRO_MPW
            tmp_fd = open(tmp_name, O_RDWR | O_BINARY | O_CREAT | O_EXCL, OPEN_PERMS);
#else
            tmp_fd = _al_open(tmp_name, O_RDWR | O_BINARY | O_CREAT | O_EXCL);
#endif
         }

      #endif

      if (tmp_fd < 0) {
         _AL_FREE(tmp_dir);
         _AL_FREE(tmp_name);

         return NULL;
      }

      name = uconvert_ascii(tmp_name, tmp);
      chunk = _pack_fdopen(tmp_fd, (pack ? F_WRITE_PACKED : F_WRITE_NOPACK));

      if (chunk) {
         chunk->normal.filename = _al_ustrdup(name);

         if (pack)
            chunk->normal.parent->normal.parent = f;
         else
            chunk->normal.parent = f;

         chunk->normal.flags |= PACKFILE_FLAG_CHUNK;
      }

      _AL_FREE(tmp_dir);
      _AL_FREE(tmp_name);
   }
   else {
      /* read a sub-chunk */
      _packfile_filesize = pack_mgetl(f);
      _packfile_datasize = pack_mgetl(f);

      if ((chunk = create_packfile(TRUE)) == NULL)
         return NULL;

      chunk->normal.flags = PACKFILE_FLAG_CHUNK;
      chunk->normal.parent = f;

      if (f->normal.flags & PACKFILE_FLAG_OLD_CRYPT) {
         /* backward compatibility mode */
         if (f->normal.passdata) {
            if ((chunk->normal.passdata = _AL_MALLOC_ATOMIC(strlen(f->normal.passdata)+1)) == NULL) {
               *allegro_errno = ENOMEM;
               _AL_FREE(chunk);
               return NULL;
            }
            _al_sane_strncpy(chunk->normal.passdata, f->normal.passdata, strlen(f->normal.passdata)+1);
            chunk->normal.passpos = chunk->normal.passdata + (long)f->normal.passpos - (long)f->normal.passdata;
            f->normal.passpos = f->normal.passdata;
         }
         chunk->normal.flags |= PACKFILE_FLAG_OLD_CRYPT;
      }

      if (_packfile_datasize < 0) {
         /* read a packed chunk */
         chunk->normal.unpack_data = create_lzss_unpack_data();
         ASSERT(!chunk->normal.pack_data);

         if (!chunk->normal.unpack_data) {
            free_packfile(chunk);
            return NULL;
         }

         _packfile_datasize = -_packfile_datasize;
         chunk->normal.todo = _packfile_datasize;
         chunk->normal.flags |= PACKFILE_FLAG_PACK;
      }
      else {
         /* read an uncompressed chunk */
         chunk->normal.todo = _packfile_datasize;
      }
   }

   return chunk;
}



/* pack_fclose_chunk:
 *  Call after reading or writing a sub-chunk. This closes the chunk file,
 *  and returns a pointer to the original file structure (the one you
 *  passed to pack_fopen_chunk()), to allow you to read or write data
 *  after the chunk. If an error occurs, returns NULL and sets errno.
 */
PACKFILE *pack_fclose_chunk(PACKFILE *f)
{
   PACKFILE *parent;
   PACKFILE *tmp;
   char *name;
   int header, c;
   ASSERT(f);

   /* unsupported */
   if (!f->is_normal_packfile) {
      *allegro_errno = EINVAL;
      return NULL;
   }

   parent = f->normal.parent;
   name = f->normal.filename;

   if (f->normal.flags & PACKFILE_FLAG_WRITE) {
      /* finish writing a chunk */
      int hndl;

      /* duplicate the file descriptor to create a readable pack file,
       * the file descriptor must have been opened in read/write mode
       */
      if (f->normal.flags & PACKFILE_FLAG_PACK)
         hndl = dup(f->normal.parent->normal.hndl);
      else
         hndl = dup(f->normal.hndl);

      if (hndl<0) {
         *allegro_errno = errno;
         return NULL;
      }

      _packfile_datasize = f->normal.todo + f->normal.buf_size - 4;

      if (f->normal.flags & PACKFILE_FLAG_PACK) {
         parent = parent->normal.parent;
         f->normal.parent->normal.parent = NULL;
      }
      else
         f->normal.parent = NULL;

      /* close the writeable temp file, it isn't physically closed
       * because the descriptor has been duplicated
       */
      f->normal.flags &= ~PACKFILE_FLAG_CHUNK;
      pack_fclose(f);

      lseek(hndl, 0, SEEK_SET);

      /* create a readable pack file */
      tmp = _pack_fdopen(hndl, F_READ);
      if (!tmp)
         return NULL;

      _packfile_filesize = tmp->normal.todo - 4;

      header = pack_mgetl(tmp);

      pack_mputl(_packfile_filesize, parent);

      if (header == encrypt_id(F_PACK_MAGIC, TRUE))
         pack_mputl(-_packfile_datasize, parent);
      else
         pack_mputl(_packfile_datasize, parent);

      while ((c = pack_getc(tmp)) != EOF)
         pack_putc(c, parent);

      pack_fclose(tmp);

      delete_file(name);
      _AL_FREE(name);
   }
   else {
      /* finish reading a chunk */
      while (f->normal.todo > 0)
         pack_getc(f);

      if (f->normal.unpack_data) {
         free_lzss_unpack_data(f->normal.unpack_data);
         f->normal.unpack_data = NULL;
      }

      if ((f->normal.passpos) && (f->normal.flags & PACKFILE_FLAG_OLD_CRYPT))
         parent->normal.passpos = parent->normal.passdata + (long)f->normal.passpos - (long)f->normal.passdata;

      free_packfile(f);
   }

   return parent;
}



/* pack_fseek:
 *  Like the stdio fseek() function, but only supports forward seeks
 *  relative to the current file position.
 */
int pack_fseek(PACKFILE *f, int offset)
{
   ASSERT(f);
   ASSERT(offset >= 0);

   return f->vtable->pf_fseek(f->userdata, offset);
}



/* pack_getc:
 *  Returns the next character from the stream f, or EOF if the end of the
 *  file has been reached.
 */
int pack_getc(PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_getc);

   return f->vtable->pf_getc(f->userdata);
}



/* pack_putc:
 *  Puts a character in the stream f.
 */
int pack_putc(int c, PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_putc);

   return f->vtable->pf_putc(c, f->userdata);
}



/* pack_feof:
 *  pack_feof() returns nonzero as soon as you reach the end of the file. It
 *  does not wait for you to attempt to read beyond the end of the file,
 *  contrary to the ISO C feof() function.
 */
int pack_feof(PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_feof);

   return f->vtable->pf_feof(f->userdata);
}



/* pack_ferror:
 *  Returns nonzero if the error indicator for the stream is set, indicating
 *  that an error has occurred during a previous operation on the stream.
 */
int pack_ferror(PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_ferror);

   return f->vtable->pf_ferror(f->userdata);
}



/* pack_igetw:
 *  Reads a 16 bit word from a file, using intel byte ordering.
 */
int pack_igetw(PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
         return ((b2 << 8) | b1);

   return EOF;
}



/* pack_igetl:
 *  Reads a 32 bit long from a file, using intel byte ordering.
 */
long pack_igetl(PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
         if ((b3 = pack_getc(f)) != EOF)
            if ((b4 = pack_getc(f)) != EOF)
               return (((long)b4 << 24) | ((long)b3 << 16) |
                       ((long)b2 << 8) | (long)b1);

   return EOF;
}



/* pack_iputw:
 *  Writes a 16 bit int to a file, using intel byte ordering.
 */
int pack_iputw(int w, PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b2,f)==b2)
      if (pack_putc(b1,f)==b1)
         return w;

   return EOF;
}



/* pack_iputl:
 *  Writes a 32 bit long to a file, using intel byte ordering.
 */
long pack_iputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b4,f)==b4)
      if (pack_putc(b3,f)==b3)
         if (pack_putc(b2,f)==b2)
            if (pack_putc(b1,f)==b1)
               return l;

   return EOF;
}



/* pack_mgetw:
 *  Reads a 16 bit int from a file, using motorola byte-ordering.
 */
int pack_mgetw(PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
         return ((b1 << 8) | b2);

   return EOF;
}



/* pack_mgetl:
 *  Reads a 32 bit long from a file, using motorola byte-ordering.
 */
long pack_mgetl(PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = pack_getc(f)) != EOF)
      if ((b2 = pack_getc(f)) != EOF)
         if ((b3 = pack_getc(f)) != EOF)
            if ((b4 = pack_getc(f)) != EOF)
               return (((long)b1 << 24) | ((long)b2 << 16) |
                       ((long)b3 << 8) | (long)b4);

   return EOF;
}



/* pack_mputw:
 *  Writes a 16 bit int to a file, using motorola byte-ordering.
 */
int pack_mputw(int w, PACKFILE *f)
{
   int b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
         return w;

   return EOF;
}



/* pack_mputl:
 *  Writes a 32 bit long to a file, using motorola byte-ordering.
 */
long pack_mputl(long l, PACKFILE *f)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   b1 = (int)((l & 0xFF000000L) >> 24);
   b2 = (int)((l & 0x00FF0000L) >> 16);
   b3 = (int)((l & 0x0000FF00L) >> 8);
   b4 = (int)l & 0x00FF;

   if (pack_putc(b1,f)==b1)
      if (pack_putc(b2,f)==b2)
         if (pack_putc(b3,f)==b3)
            if (pack_putc(b4,f)==b4)
               return l;

   return EOF;
}



/* pack_fread:
 *  Reads n bytes from f and stores them at memory location p. Returns the
 *  number of items read, which will be less than n if EOF is reached or an
 *  error occurs. Error codes are stored in errno.
 */
long pack_fread(void *p, long n, PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_fread);
   ASSERT(p);
   ASSERT(n >= 0);

   return f->vtable->pf_fread(p, n, f->userdata);
}



/* pack_fwrite:
 *  Writes n bytes to the file f from memory location p. Returns the number
 *  of items written, which will be less than n if an error occurs. Error
 *  codes are stored in errno.
 */
long pack_fwrite(AL_CONST void *p, long n, PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_fwrite);
   ASSERT(p);
   ASSERT(n >= 0);

   return f->vtable->pf_fwrite(p, n, f->userdata);
}



/* pack_ungetc:
 *  Puts a character back in the file's input buffer. It only works
 *  for characters just fetched by pack_getc and, like ungetc, only a
 *  single push back is guaranteed.
 */
int pack_ungetc(int c, PACKFILE *f)
{
   ASSERT(f);
   ASSERT(f->vtable);
   ASSERT(f->vtable->pf_ungetc);

   return f->vtable->pf_ungetc(c, f->userdata);
}



/* pack_fgets:
 *  Reads a line from a text file, storing it at location p. Stops when a
 *  linefeed is encountered, or max bytes have been read. Returns a pointer
 *  to where it stored the text, or NULL on error. The end of line is
 *  handled by detecting optional '\r' characters optionally followed
 *  by '\n' characters. This supports CR-LF (DOS/Windows), LF (Unix), and
 *  CR (Mac) formats.
 */
char *pack_fgets(char *p, int max, PACKFILE *f)
{
   char *pmax, *orig_p = p;
   int c;
   ASSERT(f);

   *allegro_errno = 0;

   pmax = p+max - ucwidth(0);

   if ((c = pack_getc(f)) == EOF) {
      if (ucwidth(0) <= max)
         usetc(p,0);
      return NULL;
   }

   do {

      if (c == '\r' || c == '\n') {
         /* Technically we should check there's space in the buffer, and if so,
          * add a \n.  But pack_fgets has never done this. */
         if (c == '\r') {
            /* eat the following \n, if any */
            c = pack_getc(f);
            if ((c != '\n') && (c != EOF))
               pack_ungetc(c, f);
         }
         break;
      }

      /* is there room in the buffer? */
      if (ucwidth(c) > pmax - p) {
         pack_ungetc(c, f);
         c = '\0';
         break;
      }

      /* write the character */
      p += usetc(p, c);
   }
   while ((c = pack_getc(f)) != EOF);

   /* terminate the string */
   usetc(p, 0);

   if (c == '\0' || *allegro_errno)
      return NULL;

   return orig_p; /* p has changed */
}



/* pack_fputs:
 *  Writes a string to a text file, returning zero on success, -1 on error.
 *  The input string is converted from the current text encoding format
 *  to UTF-8 before writing. Newline characters (\n) are written as \r\n
 *  on DOS and Windows platforms.
 */
int pack_fputs(AL_CONST char *p, PACKFILE *f)
{
   char *buf, *s;
   int bufsize;
   ASSERT(f);
   ASSERT(p);

   *allegro_errno = 0;

   bufsize = uconvert_size(p, U_CURRENT, U_UTF8);
   buf = _AL_MALLOC_ATOMIC(bufsize);
   if (!buf)
      return -1;

   s = uconvert(p, U_CURRENT, buf, U_UTF8, bufsize);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
         if (*s == '\n')
            pack_putc('\r', f);
      #endif

      pack_putc(*s, f);
      s++;
   }

   _AL_FREE(buf);

   if (*allegro_errno)
      return -1;
   else
      return 0;
}



/* pack_get_userdata:
 *  Returns the userdata field of packfiles using user-defined vtables.
 */
void *pack_get_userdata(PACKFILE *f)
{
   ASSERT(f);

   return f->userdata;
}



/***************************************************
 ************ "Normal" packfile vtable *************
 ***************************************************

   Ideally this would be the only section which knows about the details
   of "normal" packfiles. However, this ideal is still being violated in
   many places (partly due to the API, and partly because it would be
   quite a lot of work to move the _al_normal_packfile_details field
   into the userdata field of the PACKFILE structure.
*/


static int normal_fclose(void *_f);
static int normal_getc(void *_f);
static int normal_ungetc(int ch, void *_f);
static int normal_putc(int c, void *_f);
static long normal_fread(void *p, long n, void *_f);
static long normal_fwrite(AL_CONST void *p, long n, void *_f);
static int normal_fseek(void *_f, int offset);
static int normal_feof(void *_f);
static int normal_ferror(void *_f);

static int normal_refill_buffer(PACKFILE *f);
static int normal_flush_buffer(PACKFILE *f, int last);



static PACKFILE_VTABLE normal_vtable =
{
   normal_fclose,
   normal_getc,
   normal_ungetc,
   normal_fread,
   normal_putc,
   normal_fwrite,
   normal_fseek,
   normal_feof,
   normal_ferror
};



static int normal_fclose(void *_f)
{
   PACKFILE *f = _f;
   int ret;

   if (f->normal.flags & PACKFILE_FLAG_WRITE) {
      if (f->normal.flags & PACKFILE_FLAG_CHUNK) {
         f = pack_fclose_chunk(_f);
         if (!f)
            return -1;

         return pack_fclose(f);
      }

      normal_flush_buffer(f, TRUE);
   }

   if (f->normal.parent) {
      ret = pack_fclose(f->normal.parent);
   }
   else {
      ret = close(f->normal.hndl);
      if (ret != 0)
         *allegro_errno = errno;
   }

   if (f->normal.pack_data) {
      free_lzss_pack_data(f->normal.pack_data);
      f->normal.pack_data = NULL;
   }

   if (f->normal.unpack_data) {
      free_lzss_unpack_data(f->normal.unpack_data);
      f->normal.unpack_data = NULL;
   }

   if (f->normal.passdata) {
      _AL_FREE(f->normal.passdata);
      f->normal.passdata = NULL;
      f->normal.passpos = NULL;
   }

   return ret;
}



/* normal_no_more_input:
 *  Return non-zero if the number of bytes remaining in the file (todo) is
 *  less than or equal to zero.
 *
 *  However, there is a special case.  If we are reading from a LZSS
 *  compressed file, the latest call to lzss_read() may have suspended while
 *  writing out a sequence of bytes due to the output buffer being too small.
 *  In that case the `todo' count would be decremented (possibly to zero),
 *  but the output isn't yet completely written out.
 */
static INLINE int normal_no_more_input(PACKFILE *f)
{
   /* see normal_refill_buffer() to see when lzss_read() is called */
   if (f->normal.parent && (f->normal.flags & PACKFILE_FLAG_PACK) &&
       _al_lzss_incomplete_state(f->normal.unpack_data))
      return 0;

   return (f->normal.todo <= 0);
}



static int normal_getc(void *_f)
{
   PACKFILE *f = _f;

   f->normal.buf_size--;
   if (f->normal.buf_size > 0)
      return *(f->normal.buf_pos++);

   if (f->normal.buf_size == 0) {
      if (normal_no_more_input(f))
         f->normal.flags |= PACKFILE_FLAG_EOF;
      return *(f->normal.buf_pos++);
   }

   return normal_refill_buffer(f);
}



static int normal_ungetc(int c, void *_f)
{
   PACKFILE *f = _f;

   if (f->normal.buf_pos == f->normal.buf) {
      return EOF;
   }
   else {
      *(--f->normal.buf_pos) = (unsigned char)c;
      f->normal.buf_size++;
      f->normal.flags &= ~PACKFILE_FLAG_EOF;
      return (unsigned char)c;
   }
}



static long normal_fread(void *p, long n, void *_f)
{
   PACKFILE *f = _f;
   unsigned char *cp = (unsigned char *)p;
   long i;
   int c;

   for (i=0; i<n; i++) {
      if ((c = normal_getc(f)) == EOF)
         break;

      *(cp++) = c;
   }

   return i;
}



static int normal_putc(int c, void *_f)
{
   PACKFILE *f = _f;

   if (f->normal.buf_size + 1 >= F_BUF_SIZE) {
      if (normal_flush_buffer(f, FALSE))
         return EOF;
   }

   f->normal.buf_size++;
   return (*(f->normal.buf_pos++) = c);
}



static long normal_fwrite(AL_CONST void *p, long n, void *_f)
{
   PACKFILE *f = _f;
   AL_CONST unsigned char *cp = (AL_CONST unsigned char *)p;
   long i;

   for (i=0; i<n; i++) {
      if (normal_putc(*cp++, f) == EOF)
         break;
   }

   return i;
}



static int normal_fseek(void *_f, int offset)
{
   PACKFILE *f = _f;
   int i;

   if (f->normal.flags & PACKFILE_FLAG_WRITE)
      return -1;

   *allegro_errno = 0;

   /* skip forward through the buffer */
   if (f->normal.buf_size > 0) {
      i = MIN(offset, f->normal.buf_size);
      f->normal.buf_size -= i;
      f->normal.buf_pos += i;
      offset -= i;
      if ((f->normal.buf_size <= 0) && normal_no_more_input(f))
         f->normal.flags |= PACKFILE_FLAG_EOF;
   }

   /* need to seek some more? */
   if (offset > 0) {
      i = MIN(offset, f->normal.todo);

      if ((f->normal.flags & PACKFILE_FLAG_PACK) || (f->normal.passpos)) {
         /* for compressed or encrypted files, we just have to read through the data */
         while (i > 0) {
            pack_getc(f);
            i--;
         }
      }
      else {
         if (f->normal.parent) {
            /* pass the seek request on to the parent file */
            pack_fseek(f->normal.parent, i);
         }
         else {
            /* do a real seek */
            lseek(f->normal.hndl, i, SEEK_CUR);
         }
         f->normal.todo -= i;
         if (normal_no_more_input(f))
            f->normal.flags |= PACKFILE_FLAG_EOF;
      }
   }

   if (*allegro_errno)
      return -1;
   else
      return 0;
}



static int normal_feof(void *_f)
{
   PACKFILE *f = _f;

   return (f->normal.flags & PACKFILE_FLAG_EOF);
}



static int normal_ferror(void *_f)
{
   PACKFILE *f = _f;

   return (f->normal.flags & PACKFILE_FLAG_ERROR);
}



/* normal_refill_buffer:
 *  Refills the read buffer. The file must have been opened in read mode,
 *  and the buffer must be empty.
 */
static int normal_refill_buffer(PACKFILE *f)
{
   int i, sz, done, offset;

   if (f->normal.flags & PACKFILE_FLAG_EOF)
      return EOF;

   if (normal_no_more_input(f)) {
      f->normal.flags |= PACKFILE_FLAG_EOF;
      return EOF;
   }

   if (f->normal.parent) {
      if (f->normal.flags & PACKFILE_FLAG_PACK) {
         f->normal.buf_size = lzss_read(f->normal.parent, f->normal.unpack_data, MIN(F_BUF_SIZE, f->normal.todo), f->normal.buf);
      }
      else {
         f->normal.buf_size = pack_fread(f->normal.buf, MIN(F_BUF_SIZE, f->normal.todo), f->normal.parent);
      }
      if (f->normal.parent->normal.flags & PACKFILE_FLAG_EOF)
         f->normal.todo = 0;
      if (f->normal.parent->normal.flags & PACKFILE_FLAG_ERROR)
         goto Error;
   }
   else {
      f->normal.buf_size = MIN(F_BUF_SIZE, f->normal.todo);

      offset = lseek(f->normal.hndl, 0, SEEK_CUR);
      done = 0;

      errno = 0;
      sz = read(f->normal.hndl, f->normal.buf, f->normal.buf_size);

      while (sz+done < f->normal.buf_size) {
         if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
            goto Error;

         if (sz > 0)
            done += sz;

         lseek(f->normal.hndl, offset+done, SEEK_SET);
         errno = 0;
         sz = read(f->normal.hndl, f->normal.buf+done, f->normal.buf_size-done);
      }

      if ((f->normal.passpos) && (!(f->normal.flags & PACKFILE_FLAG_OLD_CRYPT))) {
         for (i=0; i<f->normal.buf_size; i++) {
            f->normal.buf[i] ^= *(f->normal.passpos++);
            if (!*f->normal.passpos)
               f->normal.passpos = f->normal.passdata;
         }
      }
   }

   f->normal.todo -= f->normal.buf_size;
   f->normal.buf_pos = f->normal.buf;
   f->normal.buf_size--;
   if (f->normal.buf_size <= 0)
      if (normal_no_more_input(f))
         f->normal.flags |= PACKFILE_FLAG_EOF;

   if (f->normal.buf_size < 0)
      return EOF;
   else
      return *(f->normal.buf_pos++);

 Error:
   *allegro_errno = EFAULT;
   f->normal.flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}



/* normal_flush_buffer:
 *  Flushes a file buffer to the disk. The file must be open in write mode.
 */
static int normal_flush_buffer(PACKFILE *f, int last)
{
   int i, sz, done, offset;

   if (f->normal.buf_size > 0) {
      if (f->normal.flags & PACKFILE_FLAG_PACK) {
         if (lzss_write(f->normal.parent, f->normal.pack_data, f->normal.buf_size, f->normal.buf, last))
            goto Error;
      }
      else {
         if ((f->normal.passpos) && (!(f->normal.flags & PACKFILE_FLAG_OLD_CRYPT))) {
            for (i=0; i<f->normal.buf_size; i++) {
               f->normal.buf[i] ^= *(f->normal.passpos++);
               if (!*f->normal.passpos)
                  f->normal.passpos = f->normal.passdata;
            }
         }

         offset = lseek(f->normal.hndl, 0, SEEK_CUR);
         done = 0;

         errno = 0;
         sz = write(f->normal.hndl, f->normal.buf, f->normal.buf_size);

         while (sz+done < f->normal.buf_size) {
            if ((sz < 0) && ((errno != EINTR) && (errno != EAGAIN)))
               goto Error;

            if (sz > 0)
               done += sz;

            lseek(f->normal.hndl, offset+done, SEEK_SET);
            errno = 0;
            sz = write(f->normal.hndl, f->normal.buf+done, f->normal.buf_size-done);
         }
      }
      f->normal.todo += f->normal.buf_size;
   }

   f->normal.buf_pos = f->normal.buf;
   f->normal.buf_size = 0;
   return 0;

 Error:
   *allegro_errno = EFAULT;
   f->normal.flags |= PACKFILE_FLAG_ERROR;
   return EOF;
}
