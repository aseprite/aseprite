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
 *      Some definitions for internal use by the MacOs library code.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTMAC_H
#define AINTMAC_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifndef ALLEGRO_MPW
   #error bad include
#endif

#include "macalleg.h"
#include "allegro/aintern.h"

#ifdef __cplusplus
   extern "C" {
#endif

/*macsbmp.c*/
extern void _mac_init_system_bitmap(void);
extern BITMAP *_mac_create_system_bitmap(int w, int h);
extern void _mac_destroy_system_bitmap(BITMAP *bmp);
extern void _mac_sys_set_clip(struct BITMAP *dst);
extern void _mac_sys_clear_to_color8(BITMAP *bmp, int color);
extern void _mac_sys_blit8(BITMAP *src, BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h);
extern void _mac_sys_selfblit8(BITMAP *src, BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h);
extern int _mac_sys_triangle(struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int x3, int y3, int color);
extern void _mac_sys_rectfill8(struct BITMAP *bmp, int x1, int y_1, int x2, int y2, int color);
extern void _mac_sys_hline8(struct BITMAP *bmp, int x1, int y, int x2, int color);
extern void _mac_sys_vline8(struct BITMAP *bmp, int x, int y_1, int y2, int color);
extern BITMAP *_CGrafPtr_to_system_bitmap(CGrafPtr cg);

/*macdraw.c*/
extern GDHandle MainGDevice;
extern CTabHandle MainCTable;
extern short dspr_depth;
extern volatile short _sync;
extern const RGBColor ForeDef;
extern const RGBColor BackDef;
extern int _dspr_sys_init();
extern void _dspr_sys_exit();

enum{kRDDNull    =0,
   kRDDStarted   =1,
   kRDDReserved  =2,
   kRDDFadeOut   =4,
   kRDDActive    =8,
   kRDDPaused    =16,
   kRDDUnder     =32,
   kRDDOver      =64,
   kRDDouble     =128,
};

/*macsys.c*/
extern void _mac_get_executable_name(char *output, int size);
extern void _mac_message(const char *msg);
extern int _tm_sys_init();
extern void _tm_sys_exit();

/*macfile.c*/
extern int _al_open(const char *filename, int mode);
extern int _mac_file_sys_init();

/*macallegro.c*/
extern QDGlobals qd;                                 /*The our QuickDraw globals */
extern char *strdup(const char *p);
extern void ptoc(StringPtr pstr, char *cstr);
extern Boolean RTrapAvailable(short tNumber, TrapType tType);
extern void MacEntry();

#ifdef __cplusplus
   }
#endif


#endif          /* ifndef AINTMAC_H */

