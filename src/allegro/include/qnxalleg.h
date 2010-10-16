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
 *      Header file for the QNX Allegro library port.
 *
 *      This doesn't need to be included; it prototypes functions you
 *      can use to control the library more closely.
 */


#ifndef QNX_ALLEGRO_H
#define QNX_ALLEGRO_H

#ifndef ALLEGRO_H
#error Please include allegro.h before qnxalleg.h!
#endif

#ifndef SCAN_DEPEND
   #include <Pt.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


AL_FUNC(PtWidget_t *, qnx_get_window, (void));


#ifdef __cplusplus
}
#endif

#endif
