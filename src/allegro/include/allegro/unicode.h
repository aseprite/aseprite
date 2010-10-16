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
 *      Unicode support routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_UNICODE__H
#define ALLEGRO_UNICODE__H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define U_ASCII         AL_ID('A','S','C','8')
#define U_ASCII_CP      AL_ID('A','S','C','P')
#define U_UNICODE       AL_ID('U','N','I','C')
#define U_UTF8          AL_ID('U','T','F','8')
#define U_CURRENT       AL_ID('c','u','r','.')

AL_FUNC(void, set_uformat, (int type));
AL_FUNC(int, get_uformat, (void));
AL_FUNC(void, register_uformat, (int type, AL_METHOD(int, u_getc, (AL_CONST char *s)), AL_METHOD(int, u_getx, (char **s)), AL_METHOD(int, u_setc, (char *s, int c)), AL_METHOD(int, u_width, (AL_CONST char *s)), AL_METHOD(int, u_cwidth, (int c)), AL_METHOD(int, u_isok, (int c)), int u_width_max));
AL_FUNC(void, set_ucodepage, (AL_CONST unsigned short *table, AL_CONST unsigned short *extras));

AL_FUNC(int, need_uconvert, (AL_CONST char *s, int type, int newtype));
AL_FUNC(int, uconvert_size, (AL_CONST char *s, int type, int newtype));
AL_FUNC(void, do_uconvert, (AL_CONST char *s, int type, char *buf, int newtype, int size));
AL_FUNC(char *, uconvert, (AL_CONST char *s, int type, char *buf, int newtype, int size));
AL_FUNC(int, uwidth_max, (int type));

#define uconvert_ascii(s, buf)      uconvert(s, U_ASCII, buf, U_CURRENT, sizeof(buf))
#define uconvert_toascii(s, buf)    uconvert(s, U_CURRENT, buf, U_ASCII, sizeof(buf))

#define EMPTY_STRING    "\0\0\0"

AL_ARRAY(char, empty_string);

AL_FUNCPTR(int, ugetc, (AL_CONST char *s));
AL_FUNCPTR(int, ugetx, (char **s));
AL_FUNCPTR(int, ugetxc, (AL_CONST char **s));
AL_FUNCPTR(int, usetc, (char *s, int c));
AL_FUNCPTR(int, uwidth, (AL_CONST char *s));
AL_FUNCPTR(int, ucwidth, (int c));
AL_FUNCPTR(int, uisok, (int c));
AL_FUNC(int, uoffset, (AL_CONST char *s, int idx));
AL_FUNC(int, ugetat, (AL_CONST char *s, int idx));
AL_FUNC(int, usetat, (char *s, int idx, int c));
AL_FUNC(int, uinsert, (char *s, int idx, int c));
AL_FUNC(int, uremove, (char *s, int idx));
AL_FUNC(int, utolower, (int c));
AL_FUNC(int, utoupper, (int c));
AL_FUNC(int, uisspace, (int c));
AL_FUNC(int, uisdigit, (int c));
AL_FUNC(int, ustrsize, (AL_CONST char *s));
AL_FUNC(int, ustrsizez, (AL_CONST char *s));
AL_FUNC(char *, _ustrdup, (AL_CONST char *src, AL_METHOD(void *, malloc_func, (size_t))));
AL_FUNC(char *, ustrzcpy, (char *dest, int size, AL_CONST char *src));
AL_FUNC(char *, ustrzcat, (char *dest, int size, AL_CONST char *src));
AL_FUNC(int, ustrlen, (AL_CONST char *s));
AL_FUNC(int, ustrcmp, (AL_CONST char *s1, AL_CONST char *s2));
AL_FUNC(char *, ustrzncpy, (char *dest, int size, AL_CONST char *src, int n));
AL_FUNC(char *, ustrzncat, (char *dest, int size, AL_CONST char *src, int n));
AL_FUNC(int, ustrncmp, (AL_CONST char *s1, AL_CONST char *s2, int n));
AL_FUNC(int, ustricmp, (AL_CONST char *s1, AL_CONST char *s2));
AL_FUNC(int, ustrnicmp, (AL_CONST char *s1, AL_CONST char *s2, int n));
AL_FUNC(char *, ustrlwr, (char *s));
AL_FUNC(char *, ustrupr, (char *s));
AL_FUNC(char *, ustrchr, (AL_CONST char *s, int c));
AL_FUNC(char *, ustrrchr, (AL_CONST char *s, int c));
AL_FUNC(char *, ustrstr, (AL_CONST char *s1, AL_CONST char *s2));
AL_FUNC(char *, ustrpbrk, (AL_CONST char *s, AL_CONST char *set));
AL_FUNC(char *, ustrtok, (char *s, AL_CONST char *set));
AL_FUNC(char *, ustrtok_r, (char *s, AL_CONST char *set, char **last));
AL_FUNC(double, uatof, (AL_CONST char *s));
AL_FUNC(long, ustrtol, (AL_CONST char *s, char **endp, int base));
AL_FUNC(double, ustrtod, (AL_CONST char *s, char **endp));
AL_FUNC(AL_CONST char *, ustrerror, (int err));
AL_PRINTFUNC(int, uszprintf, (char *buf, int size, AL_CONST char *format, ...), 3, 4);
AL_FUNC(int, uvszprintf, (char *buf, int size, AL_CONST char *format, va_list args));
AL_PRINTFUNC(int, usprintf, (char *buf, AL_CONST char *format, ...), 2, 3);

#ifndef ustrdup
   #ifdef FORTIFY
      #define ustrdup(src)            _ustrdup(src, Fortify_malloc)
   #else
      #define ustrdup(src)            _ustrdup(src, malloc)
   #endif
#endif

#define ustrcpy(dest, src)            ustrzcpy(dest, INT_MAX, src)
#define ustrcat(dest, src)            ustrzcat(dest, INT_MAX, src)
#define ustrncpy(dest, src, n)        ustrzncpy(dest, INT_MAX, src, n)
#define ustrncat(dest, src, n)        ustrzncat(dest, INT_MAX, src, n)
#define uvsprintf(buf, format, args)  uvszprintf(buf, INT_MAX, format, args)

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_UNICODE__H */


