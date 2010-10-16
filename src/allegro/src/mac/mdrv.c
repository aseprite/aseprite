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
 *      Mac drivers list.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include <allegro.h>

_DRIVER_INFO _system_driver_list[] ={
      {SYSTEM_MACOS, &system_macos, TRUE},
      {SYSTEM_NONE, &system_none, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _timer_driver_list[] ={
      {TIMER_MACOS, &timer_macos, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _mouse_driver_list[] ={
      {MOUSE_MACOS, &mouse_macos, TRUE},
      {MOUSE_ADB, &mouse_adb, TRUE},
      {MOUSEDRV_NONE, &mousedrv_none, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _keyboard_driver_list[] ={
      {KEYBOARD_MACOS, &keyboard_macos, TRUE},
      {KEYBOARD_ADB, &keyboard_adb, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _gfx_driver_list[] ={
      {GFX_DRAWSPROCKET, &gfx_drawsprocket, TRUE},
      {0, NULL, 0}
};
_DRIVER_INFO _digi_driver_list[]={
      {DIGI_MACOS,&digi_macos,TRUE},
      {0,NULL,0}
};
_DRIVER_INFO _midi_driver_list[]={
   {MIDI_DIGMID,&midi_digmid,TRUE},
   {MIDI_QUICKTIME,&midi_quicktime,TRUE},
   {0,NULL,0}
};
_DRIVER_INFO _joystick_driver_list[]={
   {JOY_TYPE_NONE,&joystick_none,TRUE},
   {0,NULL,0}
};
