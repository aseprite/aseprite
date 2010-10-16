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
 *      List of Unix sound drivers.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



BEGIN_DIGI_DRIVER_LIST
#if (defined ALLEGRO_WITH_JACKDIGI) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_JACK
#endif
#if (defined ALLEGRO_WITH_SGIALDIGI) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_SGIAL
#endif
#if (defined ALLEGRO_WITH_ARTSDIGI) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_ARTS
#endif
#if (defined ALLEGRO_WITH_ESDDIGI) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_ESD
#endif
#if (defined ALLEGRO_WITH_ALSADIGI) && (!defined ALLEGRO_WITH_MODULES)
   DIGI_DRIVER_ALSA
#endif
#if (defined ALLEGRO_WITH_OSSDIGI)
   DIGI_DRIVER_OSS
#endif
END_DIGI_DRIVER_LIST


BEGIN_MIDI_DRIVER_LIST
   MIDI_DRIVER_DIGMID
#if (defined ALLEGRO_WITH_ALSAMIDI) && (!defined ALLEGRO_WITH_MODULES)
   MIDI_DRIVER_ALSA
#endif
#if (defined ALLEGRO_WITH_OSSMIDI)
   MIDI_DRIVER_OSS
#endif
END_MIDI_DRIVER_LIST

