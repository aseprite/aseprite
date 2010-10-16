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
 *      List of DOS joystick drivers, kept in a seperate file so that
 *      they can be overriden by user programs.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



BEGIN_JOYSTICK_DRIVER_LIST
   JOYSTICK_DRIVER_WINGWARRIOR
   JOYSTICK_DRIVER_SIDEWINDER
   JOYSTICK_DRIVER_GAMEPAD_PRO
   JOYSTICK_DRIVER_GRIP
   JOYSTICK_DRIVER_STANDARD
   JOYSTICK_DRIVER_SNESPAD
   JOYSTICK_DRIVER_PSXPAD
   JOYSTICK_DRIVER_N64PAD
   JOYSTICK_DRIVER_DB9
   JOYSTICK_DRIVER_TURBOGRAFX
   JOYSTICK_DRIVER_IFSEGA_ISA
   JOYSTICK_DRIVER_IFSEGA_PCI
   JOYSTICK_DRIVER_IFSEGA_PCI_FAST
END_JOYSTICK_DRIVER_LIST
